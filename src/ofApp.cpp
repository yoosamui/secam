#include "ofApp.h"

#include <iostream>
#include <thread>

#include "common.h"
#include "constants.h"
//--------------------------------------------------------------
void ofApp::check_connection()
{
    string status;
    while (m_thread_processing) {
        auto path = string(CHECK_CONNECTION_SCRIPT);

        status = common::exec(path.c_str());
        m_network = status == "1" ? true : false;

        if (!m_network) common::log("disconnected", OF_LOG_WARNING);

        this_thread::sleep_for(chrono::milliseconds(10000));
    }
}

//--------------------------------------------------------------
void ofApp::setCamName(const string& camname)
{
    m_camname = camname;

    transform(m_camname.begin(), m_camname.end(), m_camname.begin(),
              [](unsigned char s) { return std::tolower(s); });

    common::setCamName(m_camname);
}

//--------------------------------------------------------------
void ofApp::setServerMode(int mode)
{
    m_server_mode = mode == 1;
}

void ofApp::setCamWidth(int width)
{
    m_cam_width = width;
}
void ofApp::setCamHeight(int height)
{
    m_cam_height = height;
}
//--------------------------------------------------------------
void ofApp::setup()
{
    ofLog::setAutoSpace(false);

    common::log("start check connection thread.");
    m_checknetwork_thread = this->spawn();

    // set frame rate.
    ofSetFrameRate(FRAME_RATE);
    ofSetVerticalSync(true);

    m_config.load("data/" + m_camname + ".cfg");

    std::stringstream ss;

    ss << "Configuration:\n"
       << "uri = " << m_config.settings.uri << "\n"
       << "timezone = " << m_config.settings.timezone << "\n"
       << "storage = " << m_config.settings.storage << "\n"
       << "minarearadius = " << m_config.settings.minarearadius << "\n"
       << "minrectwidth = " << m_config.settings.minrectwidth << "\n"
       << "minthreshold = " << m_config.settings.minthreshold << "\n"
       << "mincontousize =  " << m_config.settings.mincontoursize << "\n"
       << "detectionsmaxcount = " << m_config.settings.detectionsmaxcount << endl;

    common::log(ss.str());

    common::log("create default mask");
    m_maskPoints = m_config.mask_points;

    if (m_maskPoints.size()) {
        for (const auto p : m_maskPoints) {
            m_polyline.lineTo(p.x, p.y);
        }
    } else {
        // define default mask size
        int h = m_cam_height - 4;
        int w = m_cam_width - 4;

        Point start_point(m_cam_width / 2 - w / 2, (m_cam_height / 2) - h / 2);

        m_polyline.lineTo(start_point.x, start_point.y);
        m_polyline.lineTo(start_point.x + w, start_point.y);
        m_polyline.lineTo(start_point.x + w, start_point.y + h);
        m_polyline.lineTo(start_point.x, start_point.y + h);
        m_polyline.lineTo(start_point.x, start_point.y);

        for (const auto v : m_polyline.getVertices()) {
            m_maskPoints.push_back(Point(v.x, v.y));
        }
    }

    m_contour_finder.setMinAreaRadius(m_config.settings.minarearadius);
    m_contour_finder.setMaxAreaRadius(100);
    m_contour_finder.setThreshold(10);

    ofSetWindowTitle("CAM-" + m_camname);

    m_processing = true;
}

//--------------------------------------------------------------
void ofApp::update()
{
    if (!m_network || !m_cam.isOpened()) {
        common::log("connecting...");
        m_cam.connect();

        return;
    }

    // frames counter. 5 frames time to stabilization.
    if (m_frame_number++ < FRAME_RATE || !m_processing) return;

    m_cam >> m_frame;

    if (!m_frame.empty()) {
        m_boxes.clear();
        //        common::bgrtorgb(m_frame);
        //
        m_lowframerate = static_cast<uint8_t>(ofGetFrameRate()) < FRAME_RATE - 4;

        if (m_lowframerate) {
            common::log("LOW FRAME RATE " + to_string(FRAME_RATE), OF_LOG_WARNING);
        }

        m_timestamp = common::getTimestamp(m_config.settings.timezone);
        this->drawTimestamp();
    }
}

//--------------------------------------------------------------
void ofApp::draw()
{
    if (m_server_mode) return;

    drawMat(m_frame, 0, 0);
}

//--------------------------------------------------------------
void ofApp::drawTimestamp()
{
    if (m_frame.empty()) return;

    int fontface = cv::FONT_HERSHEY_SIMPLEX;
    double scale = 0.4;
    int thickness = 1;
    int x = m_frame.cols;
    int y = 0;

    cv::rectangle(m_frame, Point(x - 3, y + 2), Point(x - 140, 14), CV_RGB(0, 255, 0), CV_FILLED);
    cv::putText(m_frame, m_timestamp, cv::Point(x - 138, y + 12), fontface, scale,
                cv::Scalar(0, 0, 0), thickness, false);
}
//--------------------------------------------------------------
void ofApp::keyPressed(int key) {}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y) {}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y) {}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {}
