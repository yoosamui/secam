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

        if (!m_network) common::log(ERROR_LOSSCON, OF_LOG_WARNING);

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

    m_motion.init();
    ofAddListener(m_motion.on_motion, this, &ofApp::on_motion);

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

    // TODO config setting
    common::bgrtorgb(m_frame);

    if (!m_frame.empty()) {
        m_lowframerate = static_cast<uint8_t>(ofGetFrameRate()) < FRAME_RATE - 4;
        if (m_lowframerate) {
            common::log("LOW FRAME RATE " + to_string(ofGetFrameRate()), OF_LOG_WARNING);
        }

        if (m_frame.size().width != m_cam_width || m_frame.size().height != m_cam_height) {
            cv::resize(m_frame, m_resized, cv::Size(m_cam_width, m_cam_height));
        } else {
            m_frame.copyTo(m_resized);
        }

        m_timestamp = common::getTimestamp(m_config.settings.timezone);
        this->drawTimestamp();

        // process motion
        m_detected.clear();
        if (m_motion.update(m_frame)) {
            common::log("motion detection");
        }
    }
}

//--------------------------------------------------------------
void ofApp::draw()
{
    if (m_server_mode || m_frame.empty()) return;

    ofBackground(ofColor::black);

    switch (m_view) {
        case 1: {
            drawMat(m_resized, 0, 0);
            m_motion.getMaskPolyLineScaled().draw();

        } break;

        case 2:
            drawMat(m_motion.getFrame(), 0, 0);
            m_motion.getMaskPolyLine().draw();

            break;

        case 3:
            drawMat(m_motion.getMaskImage(), 0, 0);
            m_motion.getMaskPolyLine().draw();

            break;
        case 4:
            drawMat(m_motion.getOutput(), 0, 0);

            break;
        default:
            break;
    }

    // draw detection polyline
    ofPushStyle();
    ofNoFill();
    ofSetLineWidth(1.5);
    ofSetColor(yellowPrint);

    m_detected.draw();

    ofPopStyle();

    if (m_lowframerate) {
        string lfr = "L O W  F R A M E  R A T E";
        ofDrawBitmapStringHighlight(ERROR_FRAMELOW, 2, m_cam_height - 10);
    }

    if (!m_network) {
        string s = "C O N N E C T I O N  L O S S";
        if (m_frame_number % 10 == 0) {
            s = "";
        }
        ofDrawBitmapStringHighlight(s, 2, m_cam_height - 10);
    }

    char buffer[128];
    sprintf(buffer, "FPS/Frame: %2.2f/%.10lu ", ofGetFrameRate(), m_frame_number);

    ofPushStyle();
    ofDrawBitmapStringHighlight(buffer, 1, m_cam_height + 15);
    ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::on_motion(Rect& r)
{
    m_detected = m_detected.fromRectangle(toOf(r));

    if (m_view == 1) {
        float sx = static_cast<float>(m_cam_width * 100 / m_motion.getWidth()) / 100;
        float sy = static_cast<float>(m_cam_height * 100 / m_motion.getHeight()) / 100;

        m_detected.scale(sx, sy);
    }
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

    cv::rectangle(m_resized, Point(x - 3, y + 2), Point(x - 140, 14), CV_RGB(0, 255, 0), CV_FILLED);
    cv::putText(m_resized, m_timestamp, cv::Point(x - 138, y + 12), fontface, scale,
                cv::Scalar(0, 0, 0), thickness, false);
}
//--------------------------------------------------------------
void ofApp::keyPressed(int key)
{
    if (key == '1') {
        m_view = 1;
        return;
    }

    if (key == '2') {
        m_view = 2;
        return;
    }

    if (key == '3') {
        m_view = 3;
        return;
    }

    if (key == '4') {
        m_view = 4;
        return;
    }

    if (OF_KEY_F5 == key) {
        if (m_view == 2) {
            m_input_mode = input_mode_t::mask;
            m_motion.resetMask();
        }
        return;
    }
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button)
{
    if (m_input_mode == input_mode_t::mask) {
        // TODO make it in motion
        if (button == 0) {
            if (x < m_motion.getWidth() && y < m_motion.getHeight()) {
                m_motion.getMaskPolyLine().lineTo(x, y);
            }

        } else if (button == 2) {
            if (m_motion.getMaskPolyLine().size() > 1) {
                // get the fisrt and last vertices
                auto v1 = m_motion.getMaskPolyLine().getVertices()[0];
                auto v2 = m_motion.getMaskPolyLine().getVertices().back();

                // convert to cv::Point
                Point p1 = Point(v1.x, v1.y);
                Point p2 = Point(v2.x, v2.y);

                if (p1 != p2) m_motion.getMaskPolyLine().lineTo(p1.x, p1.y);

                // copy the points;
                m_motion.getMaskPoints().clear();

                for (const auto& v : m_motion.getMaskPolyLine()) {
                    m_motion.getMaskPoints().push_back(Point(v.x, v.y));
                }

                // create new mask;
                // m_mask_image.release();
                m_motion.updateMask();
                //                this->polygonScaleUp();

                m_input_mode = input_mode_t::none;

                m_config.mask_points = m_motion.getMaskPointsCopy();
                m_config.save(m_camname + ".cfg");
            }
        }
    }
}

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
