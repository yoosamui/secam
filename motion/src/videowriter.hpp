#pragma once

#include <queue>

#include "common.h"
#include "config.h"
#include "constants.h"
#include "ofMain.h"
#include "ofThread.h"
#include "ofxCv.h"
#include "ofxOpenCv.h"

using namespace ofxCv;
using namespace cv;
using namespace std;

class Videowriter : public ofThread, public VideoWriter
{
    const uint16_t QUEUE_MAX_SIZE = 128;

  public:
    const queue<Mat>& get_queue() const
    {  //
        return m_queue;
    }

    void add(const cv::Mat& img)
    {
        if (m_first_frame.empty()) {
            img.copyTo(m_first_frame);
            return;
        }

        Mat rgb;
        img.copyTo(rgb);
        common::bgr2rgb(rgb);

        if (!m_processing) {
            if (m_queue.size() >= QUEUE_MAX_SIZE) {
                do {
                    Mat prior = m_queue.front();
                    m_queue.pop();
                    prior.release();
                } while (!m_queue.empty());
            }

            m_queue.push(rgb);
            return;
        }

        if (m_processing) {
            write(rgb);
        }
    }

    void close()
    {
        // Closes the video writer.
        ofSleepMillis(10);
        release();
    }

    void stop()
    {
        /*if (boost::filesystem::exists(string(TMPDIR))) {
            const string s = m_source_dir + m_file;
            const string d = m_destination_dir + m_file;

            try {
                if (boost::filesystem::exists(s)) {
                    boost::filesystem::copy(s, d);
                    //            common::log("copy " + s + " to " + d);

                    boost::filesystem::remove(s);
                }

                // boost::filesystem::recursive_directory_iterator it(m_source_dir);
                // boost::filesystem::recursive_directory_iterator itEnd;

                // remove empty folder.
                if (boost::filesystem::is_empty(m_source_dir)) {
                    boost::filesystem::remove_all(m_source_dir);
                }

            } catch (boost::filesystem::filesystem_error& e) {
                common::log(e.what(), OF_LOG_ERROR);
            }
        }*/

        m_processing = false;
    }

    void start()
    {
        if (!m_first_frame.empty()) {
            m_create_video_file = true;
        }
    }

    string get_filepath(const string& prefix, const string& extension, int ret = 0)
    {
        const string timestamp = common::getTimestamp(m_config.settings.timezone, "%T");
        m_source_dir = m_destination_dir =
            m_config.settings.storage + common::getTimestamp(m_config.settings.timezone, "%F");

        /*if (boost::filesystem::exists(string(TMPDIR))) {
            m_source_dir = string(TMPDIR) + common::getTimestamp(m_config.settings.timezone, "%F");
        }*/

        try {
            boost::filesystem::create_directory(m_source_dir);
            boost::filesystem::create_directory(m_destination_dir);

        } catch (boost::filesystem::filesystem_error& e) {
            common::log(e.what(), OF_LOG_ERROR);
        }

        m_file = "/" + timestamp + "_" + prefix + extension;

        return ret == 0 ? m_source_dir + m_file : m_destination_dir + m_file;
    }

  private:
    Mat m_first_frame;

    bool m_processing = false;
    bool m_create_video_file = false;

    string m_source_dir;
    string m_destination_dir;
    string m_file;

    Config& m_config = m_config.getInstance();
    queue<Mat> m_queue;

    // override
    void threadedFunction()
    {
        while (isThreadRunning()) {
            while (m_create_video_file) {
                int apiID = cv::CAP_FFMPEG;
                int codec = VideoWriter::fourcc('X', '2', '6', '4');

                double fps = 25.0;

                bool isColor = (m_first_frame.type() == CV_8UC3);

                // TODO factory
                Config& m_config = m_config.getInstance();
                string filename = get_filepath("motion_" + m_config.parameters.camname, ".mkv");
                // result = filename;

                common::log("Create video file = " + filename);
                open(filename, apiID, codec, fps, m_first_frame.size(), isColor);

                // Current quality (0..100%) of the encoded videostream.
                // Can be adjusted dynamically in some codecs.
                set(VIDEOWRITER_PROP_QUALITY, 100);
                m_create_video_file = false;

                do {
                    Mat prior = m_queue.front();
                    write(prior);
                    m_queue.pop();
                    prior.release();

                } while (!m_queue.empty());

                m_processing = true;
                break;
            }

            ofSleepMillis(10);
        }
    }
};

