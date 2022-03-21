
#pragma once

#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <string>

#include "ofxOpenCv.h"

using namespace std;
using namespace cv;

class Camera : public cv::VideoCapture
{
  public:
    bool connect(const string& uri)
    {
        cout << "open stream " << uri << endl;

        return open(uri, CAP_FFMPEG);
    }

    bool get_Frame(Mat& frame)
    {
        try {
            return read(frame);
        } catch (const std::runtime_error& r) {
            std::cout << r.what();
        } catch (const std::exception& e) {
            std::cout << e.what();
        }

        return false;
    }

    Size getSize()
    {
        //
        return Size((int)get(CAP_PROP_FRAME_WIDTH), (int)get(CAP_PROP_FRAME_HEIGHT));
    }

    int getCodeType()
    {
        //
        return static_cast<int>(get(CAP_PROP_FOURCC));
    }

    string getCodeName()
    {
        int ex = getCodeType();
        char EXT[] = {(char)(ex & 0XFF), (char)((ex & 0XFF00) >> 8), (char)((ex & 0XFF0000) >> 16),
                      (char)((ex & 0XFF000000) >> 24), 0};

        return EXT;
    }

    float getFPS()
    {
        //
        return get(CAP_PROP_FPS);
    }

    string toString()
    {
        stringstream ss;

        Size s = getSize();
        string codec = getCodeName();
        ss << "codec " << codec << "size " << s.width << " x " << s.height << endl;

        return ss.str();
    }

  private:
    Mat m_frame;
};
