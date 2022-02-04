#pragma once

#include "camera.hpp"
#include "config.h"
#include "ofMain.h"
#include "ofxCv.h"
#include "ofxOpenCv.h"

using namespace ofxCv;
using namespace cv;

enum input_mode_t { none, mask, motion };

class ofApp : public ofBaseApp
{
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

    void setCamName(const string& camname);
    void setServerMode(int mode);
    void setCamWidth(int width);
    void setCamHeight(int height);

  private:
    void check_connection();
    void drawTimestamp();

    thread spawn()
    {
        return thread([this] { this->check_connection(); });
    }

    thread m_checknetwork_thread;

    string m_camname;
    string m_timestamp;

    Mat m_frame;

    unsigned long m_frame_number = 0;

    bool m_server_mode = false;
    bool m_thread_processing = true;
    bool m_network = false;
    bool m_processing = false;
    bool m_lowframerate = false;

    int m_cam_width = 640;
    int m_cam_height = 360;

    ofPolyline m_polyline;

    vector<Point> m_maskPoints;
    vector<Rect> m_boxes;

    ContourFinder m_contour_finder;

    input_mode_t m_input_mode = input_mode_t::none;

    Config& m_config = m_config.getInstance();

    Camera m_cam;
};
