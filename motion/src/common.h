#pragma once

#include "ofMain.h"
//#include "ofThread.h"
#include "ofxCv.h"
#include "ofxOpenCv.h"

using namespace ofxCv;
using namespace cv;
using namespace std;

namespace common
{
    void log(const string& message, ofLogLevel level = OF_LOG_NOTICE);
    void bgr2rgb(cv::Mat& img);

    int getSeconds(const string& t);
    int getHours(const string& t);

    string getElapsedTimeString();
    string getTimestampMillis(const string& time_zone, const string& format_string = "%Y.%m.%d %T");
    string getTimestamp(const string& time_zone, const string& format_string = "%Y.%m.%d %T");
    string trim(const string& s);
    string exec(const char* cmd);

    class Timex
    {
      private:
        uint64_t m_limit = 0;
        uint64_t m_previousMillis = 0;
        uint64_t m_currentMillis = 0;

        bool m_result = false;

      public:
        Timex();
        Timex(uint64_t m_limit);

        void setLimit(uint64_t limit);
        void reset();
        void set();

        bool elapsed();
    };
}  // namespace common
