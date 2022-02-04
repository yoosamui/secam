#pragma once

#include <stdio.h>

#include "ofMain.h"
#include "ofxCv.h"
#include "ofxXmlSettings.h"

// https://stackoverflow.com/questions/1008019/c-singleton-design-pattern
class Config
{
  private:
    Config() {}
    // Config(Config const&);          // Don't Implement
    // void operator=(Config const&);  // Don't implement

    ofxXmlSettings XML;

    struct Settings {
        string uri = "data/notfound.jpg";
        int minrectwidth = 40;
        int minarearadius = 1;
        int mincontoursize = 1;
        int detectionsmaxcount = 8;
        int minthreshold = 130;

        string timezone = "Asia/Bangkok";
        int daytime_hour = 6;
        int daytime_second = 30;

        int nighttime_hour = 19;
        int nighttime_second = 30;

        string storage = "/home/yoo/Dropbox/survillance/";
    };

  public:
    Config(Config const&) = delete;
    void operator=(Config const&) = delete;

    static Config& getInstance()
    {
        static Config instance;  // Guaranteed to be destroyed.
                                 // Instantiated on first use.
        return instance;
    }

    bool load(const string& filename);
    void save(const string& filename);

    vector<cv::Point> mask_points;

    Settings settings;
};
