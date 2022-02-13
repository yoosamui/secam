#include "ofApp.h"

#include <iostream>
#include <thread>

#include "common.h"
#include "constants.h"
#include "factory.hpp"

#define ERROR_LOSSCON "C O N N E C T I O N  L O S T"
#define ERROR_FRAMELOW "F R A M E R A T E  L O W"

//--------------------------------------------------------------
void ofApp::check_connection()
{
    string status, path;
    while (m_thread_processing) {
        path = string(CHECK_CONNECTION_SCRIPT);

        string host = m_config.settings.host;
        string port = to_string(m_config.settings.port);
        string cmd = path + " " + host + " " + port;

        status = common::exec(cmd.c_str());
        //        status = ofSystem(cmd.c_str());

        m_network = status == "1" ? true : false;

        if (!m_network) {
            common::log(ERROR_LOSSCON, OF_LOG_WARNING);
            m_reconnect = true;
        }

        ofSleepMillis(10000);
    }
}

//--------------------------------------------------------------

void ofApp::setup()
{
    ofLog::setAutoSpace(false);

    // set frame rate.
    ofSetFrameRate(m_config.parameters.fps);
    ofSetVerticalSync(true);

    ofLogToFile("data/logs/" + m_config.parameters.camname + ".log", false);

    if (!m_config.load()) {
        common::log("load Configuration error.", OF_LOG_WARNING);
        terminate();
    }

    m_checknetwork_thread = this->spawn();

    stringstream ss;

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
       << "detectionsmaxcount = " << m_config.settings.detectionsmaxcount << "\n"
       << endl;

    common::log(ss.str());

    m_motion.init();
    ofAddListener(m_motion.on_motion, this, &ofApp::on_motion);
    ofAddListener(m_motion.on_motion_detected, this, &ofApp::on_motion_detected);

    // recording stop timex
    m_timex_stoprecording.setLimit(30000);
    m_timex_second.setLimit(1000);
    m_timex_recording_point.setLimit(1000);

    m_writer.startThread();

    ofSetWindowTitle("CAM-" + m_config.parameters.camname + " / " + m_config.settings.timezone);

    if (!m_config.isServer()) m_font.load(OF_TTF_SANS, 9, true, true);

    m_processing = true;
}

//--------------------------------------------------------------
void ofApp::update()
{
    m_cam >> m_frame;

    if (!m_network || !m_cam.isOpened() || m_reconnect || m_frame.empty()) {
        m_frame_number = 0;
        while (!m_cam.connect()) {
            ;
        }
        if (m_network) m_reconnect = false;
        return;
    }

    // 20 frames time to stabilization.
    if (m_frame_number++ < 20 || !m_processing) return;

    // TODO config setting
    common::bgr2rgb(m_frame);

    if (!m_frame.empty() && m_network) {
        m_lowframerate = static_cast<uint8_t>(ofGetFrameRate()) < m_config.parameters.fps - 4;
        if (m_lowframerate) {
            common::log(string(ERROR_FRAMELOW) + " " + to_string(ofGetFrameRate()), OF_LOG_WARNING);
            //            m_reconnect = true;
            m_frame_number = 0;
            return;
        }

        m_timestamp = common::getTimestamp(m_config.settings.timezone);
        this->drawTimestamp();

        // add frame to writer
        m_writer.add(m_frame);

        if (m_frame.size().width != m_cam_width || m_frame.size().height != m_cam_height) {
            cv::resize(m_frame, m_resized, cv::Size(m_cam_width, m_cam_height));
        } else {
            m_frame.copyTo(m_resized);
        }

        // process motion
        m_detected.clear();
        m_motion.update(m_resized);

        if (m_motion_detected) {
            // create video
            if (!m_recording) {
                this->saveDetectionImage();

                // start recording
                if (m_dbusclient.server_alive()) {
                    m_ticket = m_dbusclient.start_recording();
                    common::log("recording: " + m_ticket);

                } else {
                    auto m_videofilename = m_writer.start("motion_");

                    stringstream ss;
                    ss << "recording: " + m_videofilename << "\n" << m_statusinfo << endl;
                    common::log(ss.str());
                }

                ofResetElapsedTimeCounter();
                m_recording = true;
            }

            m_recording_duration = VIDEODURATION;
            m_timex_stoprecording.reset();

            m_motion_detected = false;

        } else if (m_manual_recording && !m_recording) {
            m_manual_recording = false;

            // start recording
            if (m_dbusclient.server_alive()) {
                m_ticket = m_dbusclient.start_recording();
                common::log("recording: " + m_ticket);

            } else {
                auto m_videofilename = m_writer.start("recording_");

                stringstream ss;
                ss << "recording: " + m_videofilename << "\n" << m_statusinfo << endl;
                common::log(ss.str());
            }

            ofResetElapsedTimeCounter();
            m_recording = true;

            m_timex_stoprecording.reset();
        }

        if (m_recording) {
            if (m_timex_second.elapsed()) {
                m_recording_duration--;

                m_timex_second.set();
            }

            if (m_timex_stoprecording.elapsed()) {
                if (m_dbusclient.server_alive()) {
                    // dbus
                    m_dbusclient.stop_recording(m_ticket);
                } else {
                    //
                    m_writer.stop();
                }

                common::log("Recording finish.");

                m_recording_duration = VIDEODURATION;
                m_recording = false;

                m_timex_stoprecording.set();
            }
        }
    }
}

//--------------------------------------------------------------
void ofApp::draw()
{
    if (m_config.isServer() || m_frame.empty()) return;

    ofBackground(ofColor::black);

    switch (m_view) {
        case 1:
            drawMat(m_resized, 0, 0);
            if (m_draw_mask_poly) m_motion.getMaskPolyLineScaled().draw();

            break;

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

    ofPushStyle();
    m_font.drawString(getStatusInfo(), 1, m_cam_height - 6);
    ofPopStyle();

    ofPushStyle();
    if (m_recording) {
        ofSetColor(ofColor::white);
        m_font.drawString("REC: " + common::getElapsedTimeString(), m_cam_width - 100,
                          m_cam_height - 6);

        if (m_timex_recording_point.elapsed()) {
            ofSetColor(ofColor::red);
            ofDrawCircle(m_cam_width - 110, m_cam_height - 11, 6);

            m_timex_recording_point.set();
        }
    }
    ofPopStyle();
}

//--------------------------------------------------------------
string& ofApp::getStatusInfo()
{
    // clang-format off

    char buf[512];
    sprintf(buf,"FPS/Frame: %2.2f/%.9lu q:%.3ld [ %3d, %3d, %3d, %3d ] [vid:%.2d]",
             ofGetFrameRate(),
             m_frame_number,
             m_writer.get_queue().size(),
             m_config.settings.minthreshold,
             m_config.settings.minrectwidth,
             m_config.settings.mincontoursize,
             m_config.settings.detectionsmaxcount,
             m_recording_duration);

    // clang-format on

    m_statusinfo = buf;
    return m_statusinfo;
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
void ofApp::saveDetectionImage()
{
    //    cvtColor(m_frame, m_frame, COLOR_BGR2RGB);
    // common::bgrtorgb(m_frame);

    Mat img;
    m_frame.copyTo(img);
    Rect r = toCv(m_detected.getBoundingBox());

    string text = to_string(r.width) + "x" + to_string(r.height);

    cv::putText(img, text, cv::Point(r.x, r.y - 10), cv::FONT_HERSHEY_DUPLEX, 0.5,
                cv::Scalar(0, 255, 0), 0.5, false);

    string filename = m_writer.get_filepath("motion_" + m_config.parameters.camname, ".jpg", 1);
    cv::rectangle(img, r, cv::Scalar(0, 0, 255), 2);

    imwrite(filename, img);
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

    if (key == 'i') {
        m_draw_mask_poly = !m_draw_mask_poly;
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