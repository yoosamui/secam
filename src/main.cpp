#include <iomanip>
#include <iostream>
#include <opencv2/highgui.hpp>

#include "constants.h"
#include "ofApp.h"
#include "ofAppNoWindow.h"
#include "ofMain.h"

namespace bfs = boost::filesystem;

static const string keys =
    "{ help h   |       | print help message. }"
    "{ camera c |       | load the camera config.cfg file and starts streaming. }"
    "{ mode m   | 0     | start secam as client = 0 or server = 1  instance.}"
    "{ width w  | 640   | stream width. }"
    "{ height h | 360   | stream height. }";

int main(int argc, char* argv[])
{
    cv::CommandLineParser parser(argc, argv, keys);
    parser.about("This is the secam client server system.");

    if (parser.has("help")) {
        parser.printMessage();
        return 0;
    }

    if (!parser.check()) {
        parser.printErrors();
        return 1;
    }

    auto camera = parser.get<std::string>("camera");
    auto width = parser.get<int>("width");
    auto height = parser.get<int>("height");
    auto mode = parser.get<int>("mode");

    if (height < 50 || width < 50) {
        cout << "invalid width, height values." << endl;
        return 1;
    }

    if (camera.empty() || camera == "true") {
        cout << "Enter the camera name. -c=mycamera" << endl;
        return 1;
    }

    if (!bfs::exists("data/" + camera + ".xml")) {
        std::cout << "Can't find " + camera + " config file." << std::endl;
        return 1;
    }

    auto logfile = "data/logs/" + camera + ".log";
    if (bfs::exists(logfile)) {
        bfs::remove(logfile);
    }

    if (!bfs::exists(CHECK_CONNECTION_SCRIPT)) {
        cout << "Can't find " << string(CHECK_CONNECTION_SCRIPT) << " script file." << endl;
        return 1;
    }

    ofAppNoWindow window;

    if (mode) {
        ofSetupOpenGL(&window, 10, 10, OF_WINDOW);
        cout << "start secam as server." << endl;
    } else {
        ofSetupOpenGL(width, height, OF_WINDOW);
    }

    auto app = std::make_shared<ofApp>();

    // TODO create object/struct
    app->setCamName(camera);
    app->setServerMode(mode);
    app->setCamWidth(width);
    app->setCamHeight(height);

    ofRunApp(app);

    return 0;
}
