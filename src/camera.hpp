
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

        if (m_uri.empty()) {
            m_uri = m_config.settings.uri;
        }

        char stream[512];
        int apiID = cv::CAP_FFMPEG;
        int count = 0;

        while (count++ < 10) {
            // get hostname or ip
            ifstream infile(HOST_ADDRESS_HOLDER);
            if (infile.good()) {
                getline(infile, m_host);
            }

            if (m_host.empty()) {
                common::log("host or ip address could not be found.", OF_LOG_ERROR);
                return false;
            }

            std::size_t found = m_uri.find("%s");
            if (found != std::string::npos) {
                sprintf(stream, m_uri.c_str(), m_host.c_str());

                // ofSetWindowTitle(m_cam_name + " " + m_ip + " " +
                //                 m_config.settings.timezone.c_str());
            } else {
                //
                sprintf(stream, "%s", m_uri.c_str());
                // ofSetWindowTitle(stream);
            }

            common::log("open stream :" + string(stream));

            // The open method first calls VideoCapture::release to close
            // the already opened file or camera.
            if (!open(stream, apiID)) {
                this_thread::sleep_for(chrono::milliseconds(3000));
                continue;
            } else {
                //// m_cam.set(CAP_PROP_MODE, CAP_MODE_RGB);
                set(CAP_PROP_MODE, 1);
                //// m_reconnect = false;
                break;
            }
        }

        return true;
    }

  private:
    string m_uri;
    string m_host;

    Config& m_config = m_config.getInstance();
};
