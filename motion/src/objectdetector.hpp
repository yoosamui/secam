//
// https://github.com/doleron/yolov5-opencv-cpp-python
// https://github.com/doleron/yolov5-opencv-cpp-python.git
//

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
    const float CONFIDENCE_THRESHOLD = 0.7;  // 0.5;

    const string m_title = "PERSON_";

    struct Detection {
        int class_id;
        float confidence;
        Rect box;
    };
    // clang-format off
    map<string, Scalar> color_map = {
        {"person", Scalar(0, 255, 255)},
        {"motorbike", Scalar(255, 255, 0)},
        {"car", Scalar(0, 255, 0)}
    };
    // clang-format on

  public:
    ofEvent<int> on_finish_detections;

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

    void add(const Mat &img, const Rect &r)
    {
        if (m_frames.size() > 10 || ++m_frame_number % 3 || img.empty() || m_block_add) {
            return;
        }

        common::log("Add detection frame =  " + to_string(r.width) + " x " + to_string(r.height));

        Mat frame;
        img.copyTo(frame);
        common::bgr2rgb(frame);

        m_frames.push_back(frame);
    }

    void detect()
    {
        m_processing = true;
        m_block_add = true;
        if (m_frames.size()) common::log("Detected frames count :" + to_string(m_frames.size()));

        int i = 1;
        for (auto &frame : m_frames) {
            if (m_detected) break;

            common::log("Add to queue :" + to_string(i));
            m_queue.push(frame.clone());
            frame.release();
            i++;
        }

        reset();
    }

    void start()
    {
        reset();

        m_filename = get_filepath(m_config.parameters.camname + ".jpg");
    }

    bool detected()
    {
        //
        return m_detected;
    }

  private:
    Config &m_config = m_config.getInstance();

    vector<Mat> m_frames;
    vector<string> m_classes;

    int m_frame_number = 1;

    bool m_is_cuda = false;
    bool m_processing = false;
    bool m_block_add = false;
    bool m_detected = false;

    dnn::Net m_net;

    Rect m_detection_rect;

    string m_filename;
    string m_destination_dir;
    string m_file;
    string m_time_zone = m_config.settings.timezone;

    queue<Mat> m_queue;

    void reset()
    {
        m_frames.clear();

        m_block_add = false;
        m_detected = false;
    }

    Scalar get_color(const string &name)
    {
        if (color_map.count(name) == 0) return Scalar(255, 255, 255);

        return color_map[name];
    }

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
        vector<Detection> output;
        int count = 1;
        while (isThreadRunning()) {
            count = 1;
            while (!m_queue.empty() && m_processing) {
                common::log("Start detection => " + to_string(count));

                output.clear();
                m_detected = detect(m_queue.front(), output);

                m_queue.pop();
                count++;

                if (m_detected) {
                    do {
                        m_queue.pop();
                    } while (!m_queue.empty());

                    common::log("!!!Person detected!!!");
                    break;
                }
            }

            if (m_processing) {
                ofNotifyEvent(on_finish_detections, count, this);
                m_processing = false;
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

    int detect(Mat frame, vector<Detection> &output)

    {
        Mat blob;

        auto input_image = format_yolov5(frame);

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

        vector<int> nms_result;
        dnn::NMSBoxes(boxes, confidences, SCORE_THRESHOLD, NMS_THRESHOLD, nms_result);
        for (size_t i = 0; i < nms_result.size(); i++) {
            int idx = nms_result[i];

            Detection result;
            result.class_id = class_ids[idx];
            result.confidence = confidences[idx];
            result.box = boxes[idx];

            output.push_back(result);
        }

        return draw(frame, output) != 0;
    }

    bool draw(const Mat &frame, vector<Detection> &output)
    {
        int detections = output.size();
        if (!detections) return false;

        Mat input;
        frame.copyTo(input);

        bool found = false;

        for (int c = 0; c < detections; ++c) {
            auto detection = output[c];

            auto box = detection.box;
            auto classId = detection.class_id;
            auto color = get_color(m_classes[classId]);

            if (!found) found = m_classes[classId] == "person";

            Rect r = inflate(box, 20, input);

            rectangle(input, r, color, 2);
            rectangle(input, Point(r.x - 1, r.y - 20), cv::Point(r.x + r.width, r.y), color,
                      FILLED);

            float fscale = 0.4;
            int thickness = 1;
            string title = m_classes[classId];

            putText(input, title, Point(r.x + 2, r.y - 5), cv::FONT_HERSHEY_SIMPLEX, fscale,
                    Scalar(0, 0, 0), thickness, LINE_AA, false);
        }

        if (found) imwrite(m_filename, input);
        return found;
    }

    Rect inflate(const Rect &rect, size_t size, const Mat &frame)
    {
        Rect r = rect;

        r.x -= size;
        if (r.x < 0) r.x = 0;

        r.y -= size;
        if (r.y < 0) r.y = 0;

        r.width += size * 2;
        if (r.x + r.width > frame.cols) {
            r.x -= (r.x + r.width) - frame.cols;
        }

        r.height += size * 2;
        if (r.y + r.height > frame.rows) {
            r.y -= (r.y + r.height) - frame.rows;
        }

        return r;
    }
};
