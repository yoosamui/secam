#include "ofApp.h"

#include <iostream>
#include <thread>

#include "common.h"
#include "constants.h"
//--------------------------------------------------------------
void ofApp::check_connection()
{
    while (m_thread_processing) {
        string status = common::exec(CHECK_CONNECTION_SCRIPT);
        m_connected = status == "1" ? true : false;

        if (!m_connected) common::log("disconnected", OF_LOG_WARNING);

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
    m_thread = this->spawn();

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
}

//--------------------------------------------------------------
void ofApp::update() {}

//--------------------------------------------------------------
void ofApp::draw() {}

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
