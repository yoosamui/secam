#include "ofApp.h"
#include "ofAppNoWindow.h"
#include "ofMain.h"

int main(int argc, char* argv[])

{
    if (argc < 2) {
        cout << "enter parameters:  camname" << endl;
        exit(1);
    };

    ofAppNoWindow window;

    ofSetupOpenGL(640, 380, OF_WINDOW);
    auto app = std::make_shared<ofApp>();

    //   app->setCamName(argv[1]);
    ofRunApp(app);
}
