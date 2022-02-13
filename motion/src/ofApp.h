#pragma once

#include "camera.hpp"
#include "config.h"
#include "dbusclient.h"
#include "motion.hpp"
#include "ofMain.h"
#include "ofxCv.h"
#include "ofxOpenCv.h"
#include "videowriter.hpp"

using namespace ofxCv;
using namespace cv;

class ofApp : public ofBaseApp
{
    enum input_mode_t { none, mask, motion };
    const int VIDEODURATION = 30;

  public:
    void setup();
    void update();
    void draw();
    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y);
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void mouseEntered(int x, int y);
    void mouseExited(int x, int y);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);

  private:
    void drawTimestamp();
    void on_motion(Rect& r);
    void on_motion_detected(Rect& r);
    void saveDetectionImage();

    string& getStatusInfo();

    ofPolyline m_detected;

    string m_timestamp;

    Mat m_frame;
    Mat m_resized;

    string m_statusinfo;
    string m_ticket;

    unsigned long m_frame_number = 0;

    bool m_thread_processing = true;
    bool m_processing = false;
    bool m_lowframerate = false;
    bool m_reconnect = false;
    bool m_motion_detected = false;
    bool m_recording = false;
    bool m_manual_recording = false;
    bool m_draw_mask_poly = false;
    bool m_connected = false;

    int m_recording_duration = VIDEODURATION;
    int m_cam_width = 640;
    int m_cam_height = 360;
    int m_view = 1;

    common::Timex m_timex_stoprecording;
    common::Timex m_timex_recording_point;
    common::Timex m_timex_second;

    input_mode_t m_input_mode = input_mode_t::none;

    Config& m_config = m_config.getInstance();
    Camera m_cam;
    Motion m_motion;
    Videowriter m_writer;
    ofTrueTypeFont m_font;
    Dbusclient m_dbusclient;
};
