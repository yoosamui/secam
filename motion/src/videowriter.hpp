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

const uint16_t QUEUE_MAX_SIZE = 100;

class Videowriter : public ofThread, public VideoWriter
{
  public:
    const queue<Mat>& get_queue() const
    {  //
        return m_queue;
    }

    void add(const cv::Mat& img)
    {
        if (!m_processing) {
            // make space for new frames
            if (m_queue.size() >= QUEUE_MAX_SIZE) {
                for (int i = 0; i < QUEUE_MAX_SIZE / 3; i++) {
                    m_queue.pop();
                }
            }

            //            return;
            // release video writer to avoid lookahead thread error.
            // release();
        }

        Mat rgb;
        img.copyTo(rgb);
        common::bgr2rgb(rgb);

        m_queue.push(rgb);
    }

    bool stop()
    {
        /*if (boost::filesystem::exists(string(TMPDIR))) {
            const string s = m_source_dir + m_file;
            const string d = m_destination_dir + m_file;

            try {
                if (boost::filesystem::exists(s)) {
                    boost::filesystem::copy(s, d);
                    common::log("copy " + s + " to " + d);

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
        release();

        return true;
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

            double fps = 25.0;

            bool isColor = (src.type() == CV_8UC3);

            // TODO factory
            Config& m_config = m_config.getInstance();
            string filename = get_filepath(prefix + m_config.parameters.camname, ".mkv");
            result = filename;

            open(filename, apiID, codec, fps, src.size(), isColor);

            // Current quality (0..100%) of the encoded videostream.
            // Can be adjusted dynamically in some codecs.
            set(VIDEOWRITER_PROP_QUALITY, 100);
        }

        return result;
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
    bool m_processing = false;

    string m_source_dir;
    string m_destination_dir;
    string m_file;

    Config& m_config = m_config.getInstance();
    queue<Mat> m_queue;

    // override
    void threadedFunction()
    {
        while (isThreadRunning()) {
            while (!m_queue.empty() && m_processing) {
                Mat img = m_queue.front();
                if (!img.empty()) {
                    write(img);
                }

                m_queue.pop();
            }

            ofSleepMillis(10);
        }

        //   When everything done, release the capture
        release();
    }
};

