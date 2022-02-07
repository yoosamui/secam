#include "videowriter.h"

#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "boost/date_time/local_time/local_time.hpp"
#include "boost/filesystem.hpp"  // includes all needed Boost.Filesystem declarations
#include "common.h"
#include "constants.h"
//#include "date.h"

const uint16_t QUEUE_MAX_SIZE = FRAME_RATE * 6;
namespace bf = boost::filesystem;  // alias

using namespace boost::posix_time;
using namespace boost::gregorian;
using namespace boost::local_time;
// using namespace boost::locales;
std::mutex m_mutex;

Videowriter::Videowriter() {}

void Videowriter::threadedFunction()
{
    while (isThreadRunning()) {
        while (!m_queue.empty() && m_processing) {
            Mat img = m_queue.front();

            if (!img.empty()) {
                writer.write(img);
            }

            m_queue.pop();
        }

        ofSleepMillis(10);
    }
    // When everything done, release the capture
    writer.release();
}

const queue<cv::Mat>& Videowriter::get_queue() const
{
    return m_queue;
}

string Videowriter::set_processing(bool processing, const cv::Mat& src)
{
    return set_processing(processing, src, "");
}

string Videowriter::get_filepath(const string& prefix, const string& extension, bool latest)
{
    // date +%Y-%d-%m" "%T -d "+6 hours"
    // const string directory = "/home/yoo/Dropbox/survillance/" + ofGetTimestampString("%Y-%m-%d");
    // const string timestamp = ofGetTimestampString("%H:%M:%S");

    // format strings
    // https://en.cppreference.com/w/cpp/chrono/c/strftime
    const string directory =
        m_config.settings.storage + common::getTimestamp(m_config.settings.timezone, "%F");
    const string timestamp = common::getTimestamp(m_config.settings.timezone, "%T");

    bf::create_directory(directory);

    // if (latest) {
    // return "/home/yoo/Dropbox/survillance/latest_" + prefix + extension;
    //}

    return directory + "/" + timestamp + "_" + prefix + extension;
}

string Videowriter::set_processing(bool processing, const cv::Mat& src, const string& camname)
{
    m_processing = processing;
    string result = "";
    // https://stackoverflow.com/questions/54738747/opencv-videowriter-not-writing-to-output-avi
    // http://www.fourcc.org/codecs.php ++++
    //
    // FourCC is a 4-byte code used to specify the video codec.
    // The list of available codes can be found in fourcc.org.
    // It is platform dependent. The following codecs work fine for me.

    // In Fedora: DIVX, XVID, MJPG, X264, WMV1, WMV2. (XVID is more preferable. MJPG results in high
    // size video. X264 gives very small size video) In Windows: DIVX (More to be tested and added)
    // In OSX: MJPG (.mp4), DIVX (.avi), X264 (.mkv).
    //
    if (m_processing) {
        int codec = VideoWriter::fourcc('X', '2', '6', '4');
        // select desired codec (must be available at runtime)
        //'X', 'V', 'I', 'D');  // select desired codec (must be available at runtime)
        double fps = FRAME_RATE;  // framerate of the video stream
        // string filename = get_filepath(camname, ".xvid");
        string filename = get_filepath(camname, ".mkv");
        result = filename;

        bool isColor = (src.type() == CV_8UC3);
        writer.open(filename, codec, fps, src.size(), isColor);

        // Current quality (0..100%) of the encoded videostream.
        // Can be adjusted dynamically in some codecs.
        writer.set(VIDEOWRITER_PROP_QUALITY, 100);
    } else {
        writer.release();
    }
    return result;
}

void Videowriter::add(const cv::Mat& img)
{
    if (!m_processing) {
        // make space for new frames
        if (m_queue.size() > QUEUE_MAX_SIZE) {
            for (int i = 0; i < FRAME_RATE; i++) {
                m_queue.pop();
            }
            return;
        }

        // release video writer to avoid lookahead thread error.
        writer.release();
    }

    Mat rgb;
    img.copyTo(rgb);

    common::bgr2rgb(rgb);
    m_queue.push(rgb);
}
