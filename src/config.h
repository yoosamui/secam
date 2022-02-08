#pragma once

#include <stdio.h>

#include "ofMain.h"
#include "ofxCv.h"
#include "ofxXmlSettings.h"

class Config
{
  private:
    Config() {}

    bool load(const string& filename);
    void save(const string& filename);

    ofxXmlSettings XML;

    struct Settings {
        string uri = "data/notfound.png";
        string host = "";
        int port = 554;

        int minrectwidth = 10;
        int minarearadius = 1;
        int mincontoursize = 1;
        int detectionsmaxcount = 5;
        int minthreshold = 90;

        string timezone = "Asia/Bangkok";
        int daytime_hour = 6;
        int daytime_second = 30;

        int nighttime_hour = 19;
        int nighttime_second = 30;

        string storage = "";
    };

  public:
    Config(Config const&) = delete;
    void operator=(Config const&) = delete;

    bool load();
    void save();

    static Config& getInstance()
    {
        static Config instance;
        return instance;
    }

    vector<cv::Point> mask_points;

    Settings settings;
};
