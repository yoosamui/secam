#pragma once

#include <opencv2/dnn.hpp>
#include <queue>

#include "common.h"
#include "config.h"
#include "constants.h"
#include "ofMain.h"
#include "ofThread.h"
#include "ofxCv.h"
#include "ofxOpenCv.h"
#include "opencv2/imgcodecs.hpp"

using namespace ofxCv;
using namespace cv;
using namespace std;
using namespace dnn;

class Objectdetector : public ofThread
{
    const uint16_t QUEUE_MAX_SIZE = 5;

    const float INPUT_WIDTH = 640.0;
    const float INPUT_HEIGHT = 640.0;
    const float SCORE_THRESHOLD = 0.2;
    const float NMS_THRESHOLD = 0.4;
    const float CONFIDENCE_THRESHOLD = 0.5;

    const int TILE_SIZE = 320;  // 213;
    const string m_title = "PERSON_";

    struct Detection {
        int class_id;
        float confidence;
        Rect box;
    };

    const vector<Scalar> colors = {Scalar(0, 255, 0)};

  public:
    Objectdetector()
    {
        ifstream ifs("data/classes.txt");
        string line;
        while (getline(ifs, line)) {
            m_classes.push_back(line);
        }

        auto result = dnn::readNet("data/yolov5s.onnx");

        if (m_is_cuda) {
            result.setPreferableBackend(dnn::DNN_BACKEND_CUDA);
            result.setPreferableTarget(dnn::DNN_TARGET_CUDA_FP16);
        } else {
            result.setPreferableBackend(dnn::DNN_BACKEND_OPENCV);
            result.setPreferableTarget(dnn::DNN_TARGET_CPU);
        }

        m_net = result;
    }

    bool m_first_set = false;
    float m_sx = 0, m_sy = 0;
    long m_frame_number = 0;
    int m_idx = 0;
    void add(const Mat &img, const Rect &r)
    {
        if (++m_frame_number % 2 || img.empty() || !m_processing ||
            m_frames_map.size() >= QUEUE_MAX_SIZE)
            return;

        if (!m_first_set) {
            m_first_set = true;

            m_sx = static_cast<float>(img.cols * 100 / 320) / 100;
            m_sy = static_cast<float>(img.rows * 100 / 240) / 100;
        }

        Mat rgb;
        img.copyTo(rgb);

        common::bgr2rgb(rgb);

        ofPolyline poly = ofPolyline::fromRectangle(toOf(r));

        poly.scale(m_sx, m_sy);
        Rect scaled_rect(toCv(poly.getBoundingBox()));

        Rect inflated_rec = Rect(scaled_rect.x, scaled_rect.y, TILE_SIZE, TILE_SIZE);

        int move = 40;

        inflated_rec.x -= move;
        if (inflated_rec.x < 0) inflated_rec.x = 0;

        inflated_rec.y -= move;
        if (inflated_rec.y < 0) inflated_rec.y = 0;

        if (inflated_rec.x + inflated_rec.width > rgb.cols) {
            inflated_rec.x -= (scaled_rect.x + inflated_rec.width) - rgb.cols;
        }

        if (inflated_rec.y + inflated_rec.height > rgb.rows) {
            inflated_rec.y -= (scaled_rect.y + inflated_rec.width) - rgb.rows;
        }

        m_frames_map[m_idx] = make_tuple(rgb.clone(), Rect(inflated_rec));

        m_idx++;
    }

    void detect()
    {
        int col = 0, row = 0;
        size_t count = 0;
        bool first_set = false;

        Mat mosaik;
        for (auto &m : m_frames_map) {
            Mat img;
            Rect rect;
            std::tie(img, rect) = m.second;

            if (!first_set) {
                first_set = true;
                row = col = 0;

                mosaik = Mat::zeros(INPUT_WIDTH, INPUT_HEIGHT, CV_8UC3);
            }
            //      cout << "begin add mosaik" << endl;
            img(rect).copyTo(mosaik(Rect(col, row, rect.width, rect.height)));
            //      cout << "end add mosaik" << endl;

            col += rect.width;

            if (col + rect.width > INPUT_WIDTH) {
                col = 0;
                row += rect.height;
            }

            if (m_detected) break;

            if (row + rect.height > INPUT_HEIGHT || count >= m_frames_map.size() - 1) {
                // auto filename = get_filepath(tostr(ofGetSystemTimeMillis()) + ".jpg");
                // imwrite(filename, mosaik);

                //                equalizeHist(mosaik, mosaik);
                m_queue.push(mosaik.clone());
                cout << " mosaik added." << endl;
                first_set = false;
            }
            count++;
        }
        m_processing = true;
    }

    void start()
    {
        m_output.clear();
        m_frames_map.clear();
        m_first_set = false;

        m_filename = get_filepath(m_config.parameters.camname + ".jpg");

        m_detected = false;

        m_processing = true;
    }

    bool detected()
    {
        //
        return m_detected;
    }

  private:
    Config &m_config = m_config.getInstance();

    bool m_is_cuda = false;
    bool m_processing = false;
    bool m_detected = false;

    int m_page = 0;
    map<int, tuple<Mat, Rect>> m_frames_map;

    vector<string> m_classes;
    vector<Detection> m_output;

    dnn::Net m_net;

    Rect m_detection_rect;

    string m_filename;
    string m_destination_dir;
    string m_file;
    string m_time_zone = m_config.settings.timezone;

    queue<Mat> m_queue;

    string get_filepath(const string &extension)
    {
        const string timestampt = common::getTimestamp(m_time_zone, "%T");
        const string timestampf = common::getTimestamp(m_time_zone, "%F");

        m_destination_dir = m_config.settings.storage + timestampf;

        try {
            boost::filesystem::create_directory(m_destination_dir);

        } catch (boost::filesystem::filesystem_error &e) {
            cerr << e.what() << endl;
            return "";
        }

        m_file = "/" + timestampt + "_" + m_title + extension;

        return m_destination_dir + m_file;
    }

    // override
    void threadedFunction()
    {
        while (isThreadRunning()) {
            m_page = 0;
            while (!m_queue.empty() && m_processing) {
                cout << "start detection..." << endl;
                m_detected = detect(m_queue.front());
                cout << "finish detection." << endl;

                m_page++;

                m_queue.pop();

                if (m_detected) {
                    //   m_processing = false;

                    while (!m_queue.empty()) {
                        m_queue.pop();
                    }

                    cout << "person detected" << endl;
                    break;
                }
            }

            ofSleepMillis(10);
        }
    }

    template <typename T>
    string tostr(const T &t, int precision = 2)
    {
        ostringstream ss;
        ss << fixed << setprecision(precision) << t;
        return ss.str();
    }

    Mat format_yolov5(const Mat &source)
    {
        int col = source.cols;
        int row = source.rows;

        int _max = max(col, row);

        Mat result = Mat::zeros(_max, _max, CV_8UC3);

        source.copyTo(result(Rect(0, 0, col, row)));

        return result;
    }

    bool detect(const Mat &mosaik)
    {
        cout << "ENTER DETECT" << endl;
        Mat blob;
        auto input_image = format_yolov5(mosaik);

        dnn::blobFromImage(input_image, blob, 1. / 255., cv::Size(INPUT_WIDTH, INPUT_HEIGHT),
                           Scalar(), true, false);
        m_net.setInput(blob);
        vector<Mat> outputs;
        m_net.forward(outputs, m_net.getUnconnectedOutLayersNames());

        float x_factor = input_image.cols / INPUT_WIDTH;
        float y_factor = input_image.rows / INPUT_HEIGHT;

        float *data = (float *)outputs[0].data;

        const int rows = 25200;

        vector<int> class_ids;
        vector<float> confidences;
        vector<Rect> boxes;

        for (int i = 0; i < rows; ++i) {
            float confidence = data[4];
            if (confidence >= CONFIDENCE_THRESHOLD) {
                float *classes_scores = data + 5;
                Mat scores(1, m_classes.size(), CV_32FC1, classes_scores);
                Point class_id;
                double max_class_score;
                minMaxLoc(scores, 0, &max_class_score, 0, &class_id);
                if (max_class_score > SCORE_THRESHOLD) {
                    confidences.push_back(confidence);

                    class_ids.push_back(class_id.x);

                    float x = data[0];
                    float y = data[1];
                    float w = data[2];
                    float h = data[3];

                    int left = int((x - 0.5 * w) * x_factor);
                    int top = int((y - 0.5 * h) * y_factor);
                    int width = int(w * x_factor);
                    int height = int(h * y_factor);
                    boxes.push_back(Rect(left, top, width, height));
                }
            }

            data += 85;
        }

        bool found = false;
        vector<int> nms_result;
        dnn::NMSBoxes(boxes, confidences, SCORE_THRESHOLD, NMS_THRESHOLD, nms_result);
        for (size_t i = 0; i < nms_result.size(); i++) {
            int idx = nms_result[i];
            Detection result;
            result.class_id = class_ids[idx];
            result.confidence = confidences[idx];
            result.box = boxes[idx];
            m_output.push_back(result);

            found = draw(mosaik);
        }
        cout << "LEAVE DETECT" << endl;
        return found;
    }

    bool draw(const Mat &mosaik)
    {
        Mat frame;

        if (!m_output.size()) return false;

        mosaik.copyTo(frame);

        int detections = m_output.size();
        bool found = false;
        for (int c = 0; c < detections; ++c) {
            auto detection = m_output[c];
            auto box = detection.box;
            auto classId = detection.class_id;
            const auto color = colors[classId % colors.size()];
            found = m_classes[classId] == "person";

            cv::rectangle(frame, box, color, 1.5);

            cv::rectangle(frame, cv::Point(box.x, box.y - 20), cv::Point(box.x + box.width, box.y),
                          color, cv::FILLED);
            cv::putText(frame, m_classes[classId].c_str(), cv::Point(box.x, box.y - 5),
                        cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0));
            /*
                const auto color = Scalar(0, 255, 0);
                auto detection = m_output[c];
                auto box = detection.box;
                auto classId = detection.class_id;

                found = m_classes[classId] == "person";

                ofRectangle found_rect(toOf(box));
                rectangle(input, box, color, 2);

                ofRectangle rect(0, 0, TILE_SIZE, TILE_SIZE);

                // int colmax = INPUT_WIDTH / TILE_SIZE;
                // int tiles = INPUT_WIDTH * colmax / TILE_SIZE;
                // int i = 0;

                float fscale = 0.4;
                int thickness = 1;

                string title = m_classes[classId];  // + " " + tostr(detection.confidence);
                rectangle(input, box, color, 2);
                rectangle(input, Point(box.x - 1, box.y - 20), cv::Point(box.x + box.width, box.y),
                          color, FILLED);

                putText(input, title, Point(box.x + 2, box.y - 5), FONT_HERSHEY_SIMPLEX, fscale,
                        Scalar(0, 0, 0), thickness, LINE_AA, false);

                        */

            // for (; i <= tiles; i++) {
            ////               rectangle(input, toCv(rect), color, 3);

            // rectangle(input, box, color, 3);

            // if (found_rect.intersects(rect)) {
            // rectangle(input, toCv(rect), color, 2);
            // found = true;
            // break;
            //}

            // rect.x += TILE_SIZE;
            // if (rect.x >= colmax * TILE_SIZE) {
            // rect.y += TILE_SIZE;
            // rect.x = 0;
            //}
            //}
            // if (!found) return;

            /*float fscale = 0.4;
            int thickness = 1;

            string title = m_classes[classId] + " " + tostr(detection.confidence);
            rectangle(input, Point(rect.x - 1, rect.y - 20), cv::Point(rect.x + rect.width, rect.y),
                      color, FILLED);

            putText(input, title, Point(rect.x + 2, rect.y - 5), FONT_HERSHEY_SIMPLEX, fscale,
                    Scalar(0, 0, 0), thickness, LINE_AA, false);

            imwrite(m_filename, input);*/

            /*
                        int idx = (m_page * tiles) + i;

                        Mat frame;
                        Rect r;
                        tie(frame, r) = m_frames_map[idx];
                        m_frames_map.erase(idx);

                        rectangle(frame, r, color, 2);
                        rectangle(frame, Point(r.x - 1, r.y - 20), cv::Point(r.x +
               r.width, r.y), color, FILLED);

                        float fscale = 0.4;
                        int thickness = 1;
                        string title = m_classes[classId] + " " +
               tostr(detection.confidence) + " confidence"; putText(frame, title,
               Point(r.x + 2, r.y - 5), cv::FONT_HERSHEY_SIMPLEX, fscale, Scalar(0, 0,
               0), thickness, LINE_AA, false);

                        // finally save the detected frame
                        if (frame.empty()) cout <<
               "-------------------------------------" << endl; imwrite(m_filename,
               frame);*/

            // if (found) break;
        }

        if (found) imwrite(m_filename, frame);
        return found;
    }
};
