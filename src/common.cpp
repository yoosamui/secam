#include "common.h"

namespace common
{
    string getTimestamp(const string& time_zone, const string& format_string)
    {
        time_t t = time(nullptr);
        char buf[64];

        sprintf(buf, "TZ=%s", time_zone.c_str());
        putenv(buf);

        strftime(buf, sizeof buf, format_string.c_str(), std::localtime(&t));
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
    //
    // The easiest way of fixing this is to use OpenCV to explicitly convert it back
    // to RGB, much like you do when creating the greyscale image.m_cam >> m_frame;
    // Mat frame;
    // cvtColor(img, frame, COLOR_BGR2RGB);  // Large CPU usage
    void bgrtorgb2(cv::Mat& img)
    {
        struct CV_BGR_COLOR {
            uchar blue;
            uchar green;
            uchar red;
        };

        // loop inside the image matrix
        for (int y = 0; y < img.rows; y++) {
            for (int x = 0; x < img.cols; x++) {
                CV_BGR_COLOR& color = img.ptr<CV_BGR_COLOR>(y)[x];

                // get the BGR color
                auto b = color.blue;
                auto g = color.green;
                auto r = color.red;

                // set the RGB color
                color.red = b;
                color.green = g;
                color.blue = r;
            }
        }
    }

    // OpenCV uses BGR as its default colour order for images,
    // OF and VideoCapture uses RGB.
    // When you display an image loaded with OpenCv in VideoCapture the channels will
    // be back to front.
    // cvtColor(img, frame, COLOR_BGR2RGB);  // Large CPU usage
    void bgrtorgb(cv::Mat& img)
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
    std::string trim(const std::string& s)
    {
        std::string::const_iterator it = s.begin();
        while (it != s.end() && isspace(*it)) it++;

        std::string::const_reverse_iterator rit = s.rbegin();
        while (rit.base() != it && isspace(*rit)) rit++;

        return std::string(it, rit.base());
    }
    std::string exec(const char* cmd)
    {
        char buffer[NAME_MAX];
        std::string result = "";
        FILE* pipe = popen(cmd, "r");
        if (!pipe) throw std::runtime_error("popen() failed!");
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
