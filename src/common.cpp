#include "common.h"

#include <boost/format.hpp>

namespace common
{
    string camname;
    int fps = 15;

    bool server_mode = false;

    string getElapsedTimeString()
    {
        ostringstream result;
        int et = static_cast<int>(ofGetElapsedTimef());
        auto h = et / 3600;
        auto m = (et % 3600) / 60;
        auto s = et % 60;

        result << boost::format("%02u:%02u:%02u") % h % m % s;

        return result.str();
    }

    void setFps(int f) { fps = f; }
    int getFps() { return fps; }

    void setMode(int mode) { server_mode = mode == 1; }

    bool isServerMode() { return server_mode; }

    void setCamName(const string& cam)
    {
        // TODO create an object

        camname = cam;
    }

    const string& getCamName()
    {
        //
        return camname;
    }

    // Log levels are (in order of priority):
    //
    // OF_LOG_VERBOSE
    // OF_LOG_NOTICE
    // OF_LOG_WARNING
    // OF_LOG_ERROR
    // OF_LOG_FATAL_ERROR
    // OF_LOG_SILENT
    //
    void log(const string& message, ofLogLevel level)
    {
        ofLogToFile("data/logs/" + camname + ".log", true);
        ofLog(level) << message << endl;

        ofLogToConsole();
        ofLog(level) << message << endl;
    }

    string getTimestamp(const string& time_zone, const string& format_string)
    {
        time_t t = time(nullptr);
        char buf[64];

        sprintf(buf, "TZ=%s", time_zone.c_str());
        putenv(buf);

        strftime(buf, sizeof buf, format_string.c_str(), localtime(&t));
        string timestamp(buf);

        return timestamp;
    }

    /*
     *  ausume t hast yyyy.mm.dd hh:mm:ss format
     *  and the length must be 19 characters.
     */
    int getHours(const string& t)
    {
        size_t l = t.length();
        if (l != 19) return 0;

        size_t idx = t.find(" ");
        if (idx == string::npos) return 0;

        string cumulator{};
        for (size_t i = idx; i < l; ++i) {
            if (t[i] == ':') {
                return stoi(cumulator);
            }

            cumulator += t[i];
        }

        return 0;
    }

    /*
     *  ausume t hast yyyy.mm.dd hh:mm:ss format
     *  and the length must be 19 characters.
     */
    int getSeconds(const string& t)
    {
        size_t l = t.length();
        if (l != 19) return 0;

        size_t idx = t.find(":");
        if (idx == string::npos) return 0;

        string cumulator{};
        for (size_t i = idx + 1; i < l; ++i) {
            if (t[i] == ':') {
                return stoi(cumulator);
            }

            cumulator += t[i];
        }

        return 0;
    }

    // OpenCV uses BGR as its default colour order for images,
    // OF and VideoCapture uses RGB.
    // When you display an image loaded with OpenCv in VideoCapture the channels will
    // be back to front.
    // avoid cvtColor(img, frame, COLOR_BGR2RGB) High CPU Usage.
    void bgr2rgb(cv::Mat& img)
    {
        uchar r, g, b;
        // loop inside the image matrix
        for (int y = 0; y < img.rows; y++) {
            uchar* color = img.ptr<uchar>(y);  // point to first color in row
            uchar* pixel = color;
            for (int x = 0; x < img.cols; x++) {
                // get the BGR color
                b = *color++;
                g = *color++;
                r = *color++;

                // set the RGB color
                *pixel++ = r;
                *pixel++ = g;
                *pixel++ = b;
            }
        }
    }

    Timex::Timex() {}
    Timex::Timex(uint64_t limit) { this->m_limit = limit; }

    void Timex::setLimit(uint64_t limit) { this->m_limit = limit; }
    bool Timex::elapsed()
    {
        m_currentMillis = ofGetElapsedTimeMillis();
        return (m_currentMillis - m_previousMillis >= m_limit);
    }
    void Timex::set() { m_previousMillis = m_currentMillis; }
    void Timex::reset() { m_previousMillis = ofGetElapsedTimeMillis(); }
    string trim(const string& s)
    {
        string::const_iterator it = s.begin();
        while (it != s.end() && isspace(*it)) it++;

        string::const_reverse_iterator rit = s.rbegin();
        while (rit.base() != it && isspace(*rit)) rit++;

        return string(it, rit.base());
    }

    string exec(const char* cmd)
    {
        char buffer[NAME_MAX];
        string result = "";
        FILE* pipe = popen(cmd, "r");
        if (!pipe) throw runtime_error("popen() failed!");
        try {
            while (!feof(pipe)) {
                if (fgets(buffer, 128, pipe) != NULL) result += buffer;
            }
        } catch (...) {
            pclose(pipe);
            throw;
        }
        pclose(pipe);

        return trim(result);
    }
}  // namespace common
