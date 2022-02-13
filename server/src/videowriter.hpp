#pragma once

#include <queue>

#include "ofThread.h"
#include "ofxCv.h"
#include "ofxOpenCv.h"

using namespace ofxCv;
using namespace cv;
using namespace std;

const uint16_t QUEUE_MAX_SIZE = 200;

class Videowriter : public ofThread, public VideoWriter
{
  public:
    ofEvent<Glib::ustring> on_completed;

    Glib::ustring m_ticket;

    string m_uri;
    string m_time_zone = "Asia/Bangkok";
    string m_title = "motion_gate";
    string m_temp_dir = "/var/tmp/";
    string m_storage = "/home/yoo/Dropbox/survillance/";

    ~Videowriter() { cout << "video writer deleted." << endl; }

#ifdef CLEAN
    const queue<Mat>& get_queue() const
    {  //
        return m_queue;
    }
    void add(const cv::Mat& img)
    {
        if (!m_processing) {
            // make space for new frames
            if (m_queue.size() > QUEUE_MAX_SIZE) {
                for (int i = 0; i < 60 /*m_config.parameters.fps*/; i++) {
                    m_queue.pop();
                }
                return;
            }

            // release video writer to avoid lookahead thread error.
            release();
        }

        Mat rgb;
        img.copyTo(rgb);
        //     common::bgr2rgb(rgb);

        m_queue.push(rgb);
    }

    string start(const string& prefix)
    {
        string result;
        m_processing = true;

        if (m_queue.size()) {
            //  This returns a reference to the first element of the queue,
            //  without removing the element.
            Mat src = m_queue.front();

            int apiID = cv::CAP_FFMPEG;
            int codec = VideoWriter::fourcc('X', '2', '6', '4');

            double fps = 25;

            bool isColor = (src.type() == CV_8UC3);

            // TODO factory
            Config& m_config = m_config.getInstance();
            string filename = get_filepath(prefix + m_config.parameters.camname, ".mkv");
            result = filename;

            open(filename, apiID, codec, fps, src.size(), isColor);

            // Current quality (0..100%) of the encoded videostream.
            // Can be adjusted dynamically in some codecs.
            set(VIDEOWRITER_PROP_QUALITY, 75);
        }

        return result;
    }

    string get_filepath(const string& prefix, const string& extension, int ret = 0)
    {
        const string timestamp = common::getTimestamp(m_config.settings.timezone, "%T");
        m_source_dir = m_destination_dir =
            m_config.settings.storage + common::getTimestamp(m_config.settings.timezone, "%F");

        try {
            if (boost::filesystem::exists(string(TMPDIR))) {
                m_source_dir =
                    string(TMPDIR) + common::getTimestamp(m_config.settings.timezone, "%F");
            }

            boost::filesystem::create_directory(m_source_dir);
            boost::filesystem::create_directory(m_destination_dir);

        } catch (boost::filesystem::filesystem_error& e) {
            common::log(e.what(), OF_LOG_ERROR);
        }

        m_file = "/" + timestamp + "_" + prefix + extension;

        return ret == 0 ? m_source_dir + m_file : m_destination_dir + m_file;
    }

#endif

    void start()
    {
        //
        startThread();
    }

    bool stop()
    {
        stopThread();
        ofSleepMillis(1000);

        if (boost::filesystem::exists(string(m_temp_dir))) {
            const string s = m_source_dir + m_file;
            const string d = m_destination_dir + m_file;

            try {
                boost::filesystem::copy(s, d);
                boost::filesystem::remove(s);

                // boost::filesystem::recursive_directory_iterator it(m_source_dir);
                // boost::filesystem::recursive_directory_iterator itEnd;

                // remove empty folder.
                if (boost::filesystem::is_empty(m_source_dir)) {
                    boost::filesystem::remove_all(m_source_dir);
                }

            } catch (boost::filesystem::filesystem_error& e) {
                cerr << e.what() << endl;
            }
        }

        // m_processing = false;
        // release();

        return true;
    }

  private:
    bool m_processing = false;

    string m_source_dir;
    string m_destination_dir;
    string m_file;

    string getTimestamp(const string& time_zone, const string& format_string)
    {
        time_t t = time(nullptr);
        char buf[32];

        sprintf(buf, "TZ=%s", time_zone.c_str());
        setenv("TZ", time_zone.c_str(), 1);

        strftime(buf, sizeof buf, format_string.c_str(), localtime(&t));
        unsetenv("TZ");

        return buf;
    }

    string get_filepath(const string& extension, int ret = 0)
    {
        const string timestampt = getTimestamp(m_time_zone, "%T");
        const string timestampf = getTimestamp(m_time_zone, "%F");

        m_source_dir = m_destination_dir = m_storage + timestampf;

        try {
            if (boost::filesystem::exists(string(m_temp_dir))) {
                m_source_dir = m_temp_dir + timestampf;
            }

            boost::filesystem::create_directory(m_source_dir);
            boost::filesystem::create_directory(m_destination_dir);

        } catch (boost::filesystem::filesystem_error& e) {
            cerr << e.what() << endl;
            return "";
        }

        m_file = "/" + timestampt + "_" + m_title + extension;

        return ret == 0 ? m_source_dir + m_file : m_destination_dir + m_file;
    }

    void threadedFunction()
    {
        Glib::ustring m_msg;
        cv::Mat frame;
        VideoCapture cap;

        cout << "writer ticket#: " << m_ticket << endl;
        cout << "uri: " << m_uri << endl;
        cout << "temp: " << m_temp_dir << endl;
        cout << "storage: " << m_storage << endl;
        cout << "tmp-filename: " << get_filepath(".mkv") << endl;
        cout << "dest-filename: " << get_filepath(".mkv", 1) << endl;

        bool connected = false;
        bool first_frame = true;

        auto filename = get_filepath(".mkv", 0);

        uint64_t begin_millis;

        while (isThreadRunning()) {
            if (!connected) {
                connected = cap.open(m_uri, CAP_FFMPEG);
                begin_millis = static_cast<uint64_t>(ofGetElapsedTimeMillis());

                cout << (connected ? "connected" : "lost connection.") << endl;

                if (!connected) {
                    ofSleepMillis(1000);
                    continue;
                }
            }

            cap >> frame;

            if (frame.empty()) cout << "empty........" << endl;

            if (!cap.isOpened() || frame.empty()) {
                connected = false;
                continue;
            }

            if (first_frame) {
                first_frame = false;
                bool isColor = (frame.type() == CV_8UC3);
                open(filename, VideoWriter::fourcc('X', '2', '6', '4'), 25, frame.size(), isColor);
            }

            if (!isOpened()) {
                first_frame = true;
                continue;
            }

            // encode the frame into the videofile stream
            write(frame);

            uint64_t current = static_cast<uint64_t>(ofGetElapsedTimeMillis());
            if ((current - begin_millis) > 60000 * 15) {
                ofNotifyEvent(on_completed, m_ticket, this);
                cout << "recording time out after 15 min." << endl;
                break;
            }

            ofSleepMillis(10);
        }

        //   When everything done, release the capture
        release();
    }
};

