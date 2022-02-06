#include "config.h"

#include "common.h"

#define CONFIG_MASKPOINTS "config:maskpoints"
#define CONFIG_MASKPOINTS_POINT "maskpoints:point"

void Config::save()
{
    string filename = common::getCamName() + ".xml";
    this->save(filename);
}

bool Config::load()
{
    string filename = common::getCamName() + ".xml";
    return load(filename);
}

bool Config::load(const string& filename)
{
    if (!XML.loadFile(filename)) {
        common::log("Configuration File not found. Set default settings.", OF_LOG_WARNING);
        return false;
    }

    settings.uri = XML.getValue("config:uri", settings.uri);
    settings.host = XML.getValue("config:host", settings.host);
    settings.port = XML.getValue("config:port", settings.port);

    settings.minrectwidth = XML.getValue("config:minrectwidth", settings.minrectwidth);
    settings.minarearadius = XML.getValue("config:minarearadius", settings.minarearadius);
    settings.mincontoursize = XML.getValue("config:mincontoursize", settings.mincontoursize);
    settings.detectionsmaxcount =
        XML.getValue("config:detectionsmaxcount", settings.detectionsmaxcount);
    settings.minthreshold = XML.getValue("config:minthreshold", settings.minthreshold);
    settings.timezone = XML.getValue("config:timezone", settings.timezone);
    settings.storage = XML.getValue("config:storage", settings.storage);

    XML.pushTag(CONFIG_MASKPOINTS);
    XML.pushTag(CONFIG_MASKPOINTS_POINT);

    int nbPoints = XML.getNumTags("point");

    for (int i = 0; i < nbPoints; i++) {
        int x, y;
        x = XML.getAttribute("point", "x", 0, i);
        y = XML.getAttribute("point", "y", 0, i);

        mask_points.push_back(cv::Point(x, y));
    }

    XML.popTag();
    XML.popTag();

    cout << "Configuration loaded." << endl;
    return true;
}

void Config::save(const string& filename)
{
    ofxXmlSettings xml;

    xml.addTag("config");
    xml.pushTag("config");

    xml.setValue("uri", settings.uri);
    xml.setValue("host", settings.host);
    xml.setValue("port", settings.port);
    xml.setValue("minrectwidth", settings.minrectwidth);
    xml.setValue("minarearadius", settings.minarearadius);
    xml.setValue("mincontoursize", settings.mincontoursize);
    xml.setValue("minthreshold", settings.minthreshold);
    xml.setValue("detectionsmaxcount", settings.detectionsmaxcount);
    xml.setValue("timezone", settings.timezone);
    xml.setValue("storage", settings.storage);

    const string tag = "point";
    const string atrx = "x";
    const string atry = "y";

    for (int i = 0; i < (int)mask_points.size(); i++) {
        cv::Point point = mask_points[i];
        if (i == 0) {
            xml.addTag(CONFIG_MASKPOINTS_POINT);
            xml.pushTag(CONFIG_MASKPOINTS_POINT, 0);

            xml.setAttribute(tag, atrx, point.x, 0);
            xml.setAttribute(tag, atry, point.y, 0);

            continue;
        }

        xml.addTag("point");
        xml.setAttribute(tag, atrx, point.x, i);
        xml.setAttribute(tag, atry, point.y, i);
        xml.pushTag("point", i);
        xml.popTag();  // pop position
    }

    xml.popTag();  // pop position
    xml.saveFile(filename);
    //
}
