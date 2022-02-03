#pragma once
#include "ofMain.h"
#include "ofThread.h"
#include "ofxCv.h"
#include "ofxOpenCv.h"

using namespace ofxCv;
using namespace cv;
using namespace std;

#define FRAME_RATE 22

namespace common
{
    int getSeconds(const string& t);
    int getHours(const string& t);
    string getTimestamp(const string& time_zone, const string& format_string = "%Y.%m.%d %T");
    void bgrtorgb(cv::Mat& img);
    void bgrtorgb2(cv::Mat& img);
    std::string trim(const std::string& s);
    std::string exec(const char* cmd);

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
        bool elapsed();
        void set();
        void reset();
    };
}  // namespace common
