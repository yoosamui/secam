#pragma once
#include <queue>

#include "config.h"
#include "ofMain.h"
#include "ofThread.h"
#include "ofxCv.h"
#include "ofxOpenCv.h"

using namespace ofxCv;
using namespace cv;
using namespace std;

class Videowriter : public ofThread
{
  public:
    Videowriter();
    ofEvent<uint32_t> m_on_detected;

    const queue<cv::Mat>& get_queue() const;
    void add(const cv::Mat& img);
    string set_processing(bool processing, const cv::Mat& src);
    string set_processing(bool processing, const cv::Mat& src, const string& camname);
    std::string get_filepath(const string& prefix, const string& extension, bool latest = false);
    const string& getDateTime() const;

  private:
    cv::Mat m_src;
    VideoWriter writer;
    queue<cv::Mat> m_queue;
    queue<cv::Mat> m_queue_cache;
    bool m_processing = false;
    string m_datetime;

    Config& m_config = m_config.getInstance();

    void threadedFunction();
};

