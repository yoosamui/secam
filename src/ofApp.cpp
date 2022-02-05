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

        //        m_boxes.clear();

        if (m_frame.size().width != m_cam_width || m_frame.size().height != m_cam_height) {
            cv::resize(m_frame, m_resized, cv::Size(m_cam_width, m_cam_height));
        } else {
            m_frame.copyTo(m_resized);
        }

        m_timestamp = common::getTimestamp(m_config.settings.timezone);
        this->drawTimestamp();

        // process motion
        if (m_motion.update(m_frame)) {
            //
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
            //    m_polyline_resized.draw();

        } break;

        case 2:
            drawMat(m_motion.getFrame(), 0, 0);
            // m_polyline.draw();
            break;

        default:
            return;
    }

    ofPushStyle();
    if (m_lowframerate) {
        string lfr = "L O W  F R A M E  R A T E";
        ofDrawBitmapStringHighlight(lfr, 2, m_cam_height - 10);
    }
    ofPopStyle();

    char buffer[128];
    sprintf(buffer, "FPS/Frame: %2.2f/%.10lu ", ofGetFrameRate(), m_frame_number);

    ofDrawBitmapStringHighlight(buffer, 1, m_cam_height + 15);
    ofPopStyle();
}

//OB//--------------------------------------------------------------
// void ofApp::create_mask()
//{
// if (m_maskPoints.size() == 0) {
// m_maskPoints.push_back(cv::Point(2, 2));
// m_maskPoints.push_back(cv::Point(m_proc_width - 2, 2));
// m_maskPoints.push_back(cv::Point(m_proc_width - 2, m_proc_height - 2));
// m_maskPoints.push_back(cv::Point(2, m_proc_height - 2));
// m_maskPoints.push_back(cv::Point(2, 2));
//}

// CvMat* matrix = cvCreateMat(m_proc_height, m_proc_width, CV_8UC1);
// m_mask = cvarrToMat(matrix);

// for (int x = 0; x < m_mask.cols; x++) {
// for (int y = 0; y < m_mask.rows; y++) m_mask.at<uchar>(cv::Point(x, y)) = 0;
//}

// fillPoly(m_mask, m_maskPoints, 255);
//}

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
    /*if (m_input_mode == input_mode_t::mask) {
        if (button == 0) {
            if (x < m_proc_width && y < m_proc_height) {
                m_polyline.lineTo(x, y);
            }

        } else if (button == 2) {
            if (m_polyline.size() > 1) {
                // get the fisrt and last vertices
                auto v1 = m_polyline.getVertices()[0];
                auto v2 = m_polyline.getVertices().back();

                // convert to cv::Point
                Point p1 = Point(v1.x, v1.y);
                Point p2 = Point(v2.x, v2.y);

                if (p1 != p2) m_polyline.lineTo(p1.x, p1.y);

                // copy the points;
                m_maskPoints.clear();

                for (const auto& v : m_polyline) {
                    m_maskPoints.push_back(Point(v.x, v.y));
                }

                // create new mask;
                // m_mask_image.release();
                this->create_mask();
                this->polygonScaleUp();

                m_input_mode = input_mode_t::none;

                m_config.mask_points = m_maskPoints;
                m_config.save(m_camname + ".cfg");
            }
        }
    }*/
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
