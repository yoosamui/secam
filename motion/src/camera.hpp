
#pragma once

#include "common.h"
#include "config.h"
#include "constants.h"
#include "ofxOpenCv.h"

class Camera : public cv::VideoCapture
{
  public:
    bool connect(const string& uri = "")
    {
        m_uri = uri;
        m_host = m_config.settings.host.empty();

        if (m_uri.empty()) {
            m_uri = m_config.settings.uri;
        }

        if (m_host.empty()) {
            // get hostname or ip from file
            ifstream infile(HOST_ADDRESS_HOLDER);
            if (infile.good()) {
                getline(infile, m_host);
            }
        }

        if (m_host.empty()) {
            common::log("host or ip address could not be found.", OF_LOG_ERROR);
            return false;
        }

        char stream[512];
        int apiID = cv::CAP_FFMPEG;

        std::size_t found = m_uri.find("%s");
        if (found != std::string::npos) {
            sprintf(stream, m_uri.c_str(), m_host.c_str());
        } else {
            sprintf(stream, "%s", m_uri.c_str());
        }

        common::log("open stream :" + string(stream));

        // The open method first calls VideoCapture::release to close
        // the already opened file or camera.
        if (!open(stream, apiID)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            return false;
        }

        set(CAP_PROP_MODE, 1);

        return true;
    }

  private:
    string m_uri;
    string m_host;

    thread m_thread;

    Config& m_config = m_config.getInstance();
};
