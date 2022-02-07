#include "ofApp.h"

#include <iostream>
#include <thread>

#include "common.h"
#include "constants.h"
//--------------------------------------------------------------
void ofApp::check_connection()
{
    string status, path;
    while (m_thread_processing) {
        // TODO avoid running a separate process
        // make it cross platform
        path = string(CHECK_CONNECTION_SCRIPT);

        string host = m_config.settings.host;
        string port = to_string(m_config.settings.port);
        string cmd = path + " " + host + " " + port;

        status = common::exec(cmd.c_str());

        m_network = status == "1" ? true : false;

        if (!m_network) {
            common::log(ERROR_LOSSCON, OF_LOG_WARNING);
            m_reconnect = true;
        }

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

    // set frame rate.
    ofSetFrameRate(FRAME_RATE);
    ofSetVerticalSync(true);

    if (!m_config.load()) {
        terminate();
    }

    common::log("start check connection thread.");
    m_checknetwork_thread = this->spawn();

    std::stringstream ss;

    ss << "Configuration:\n"
       << "uri = " << m_config.settings.uri << "\n"
       << "host = " << m_config.settings.host << "\n"
       << "port = " << m_config.settings.port << "\n"
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
    ofAddListener(m_motion.on_motion_detected, this, &ofApp::on_motion_detected);

    // recording stop timex
    m_timex_stoprecording.setLimit(30000);

    m_writer.startThread();

    ofSetWindowTitle("CAM-" + m_camname);
    m_processing = true;
}

//--------------------------------------------------------------
void ofApp::update()
{
    if (!m_network || !m_cam.isOpened() || m_reconnect) {
        m_frame_number = 0;
        while (!m_cam.connect()) {
            ;
        }
        if (m_network) m_reconnect = false;
        return;
    }

    // 12 frames time to stabilization.
    if (m_frame_number++ < 12 || !m_processing) return;

    m_cam >> m_frame;

    // TODO config setting
    common::bgr2rgb(m_frame);

    if (!m_frame.empty() && m_network) {
        m_timestamp = common::getTimestamp(m_config.settings.timezone);
        this->drawTimestamp();

        // add frame to writer
        m_writer.add(m_frame);

        m_lowframerate = static_cast<uint8_t>(ofGetFrameRate()) < FRAME_RATE - 4;
        if (m_lowframerate) {
            common::log(string(ERROR_FRAMELOW) + " " + to_string(ofGetFrameRate()), OF_LOG_WARNING);
            return;
        }

        if (m_frame.size().width != m_cam_width || m_frame.size().height != m_cam_height) {
            cv::resize(m_frame, m_resized, cv::Size(m_cam_width, m_cam_height));
        } else {
            m_frame.copyTo(m_resized);
        }

        // process motion
        m_detected.clear();
        m_motion.update(m_frame);

        if (m_motion_detected) {
            // create video
            if (!m_recording) {
                auto m_videofilename = m_writer.start("motion_");
                common::log("recording: " + m_videofilename);

                ofResetElapsedTimeCounter();
                m_recording = true;
            }

            m_timex_stoprecording.reset();
            m_motion_detected = false;

        } else if (m_manual_recording && !m_recording) {
            m_manual_recording = false;

            // start the writer
            auto m_videofilename = m_writer.start("recording_");
            common::log("recording: " + m_videofilename);

            ofResetElapsedTimeCounter();
            m_recording = true;

            m_timex_stoprecording.reset();
        }

        // stop recording
        if ((m_recording && m_timex_stoprecording.elapsed())) {
            // stop recording
            m_writer.stop();
            common::log("Recording finish.");
            //  m_recording_count++;
            //   m_recording_time = 0;
            m_recording = false;

            m_timex_stoprecording.set();
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
        ofDrawBitmapStringHighlight(ERROR_FRAMELOW, 2, m_cam_height - 10);
    }

    char buffer[128];
    // clang-format off
    sprintf(buffer, "FPS/Frame: %2.2f/%.lu queue:%ld ",
        ofGetFrameRate(),
        m_frame_number, m_writer.get_queue().size());

    // clang-format on
    //
    ofPushStyle();
    ofDrawBitmapStringHighlight(buffer, 1, m_cam_height + 15);
    ofPopStyle();

    ofPushStyle();
    if (m_recording) {
        char buffer[24];

        int et = static_cast<int>(ofGetElapsedTimef());
        auto h = et / 3600;
        auto m = (et % 3600) / 60;
        auto s = et % 60;

        sprintf(buffer, "REC: %02u:%02u:%02u", h, m, s);
        ofSetColor(ofColor::white);
        ofDrawBitmapStringHighlight(buffer, m_cam_width - 116, m_cam_height + 14);

        if (m_frame_number % 10 == 0) {
            ofSetColor(ofColor::red);
            ofDrawCircle(m_cam_width - 126, m_cam_height + 9, 6);
        }
    }
    ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::on_motion_detected(Rect& r)
{
    m_motion_detected = true;
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

    cv::rectangle(m_frame, Point(x - 3, y + 2), Point(x - 140, 14), CV_RGB(0, 255, 0), CV_FILLED);
    cv::putText(m_frame, m_timestamp, cv::Point(x - 138, y + 12), fontface, scale,
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

    if (OF_KEY_F12 == key) {
        m_manual_recording = true;
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
        if (button == 0) {
            if (x < m_motion.getWidth() && y < m_motion.getHeight()) {
                m_motion.getMaskPolyLine().lineTo(x, y);
            }

        } else if (button == 2) {
            if (m_motion.getMaskPolyLine().size() > 1) {
                // get the fisrt and last vertices
                auto v1 = m_motion.getMaskPolyLine().getVertices()[0];
                auto v2 = m_motion.getMaskPolyLine().getVertices().back();

                Point p1(v1.x, v1.y);
                Point p2(v2.x, v2.y);

                if (p1 != p2) m_motion.getMaskPolyLine().lineTo(p1.x, p1.y);

                m_motion.getMaskPoints().clear();

                for (const auto& v : m_motion.getMaskPolyLine()) {
                    m_motion.getMaskPoints().push_back(Point(v.x, v.y));
                }

                // create new mask;
                m_motion.updateMask();

                m_input_mode = input_mode_t::none;

                m_config.mask_points = m_motion.getMaskPointsCopy();
                m_config.save();
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
