#pragma once

#include "common.h"
#include "config.h"
#include "constants.h"
#include "ofxOpenCv.h"

#define MOG2

class Motion
{
    const int width = 320;
    const int height = 240;

  public:
    ofEvent<Rect> on_motion;

    Motion()
    {
        // update backgound after 5 frames
        int frames = 1000 / FRAME_RATE * 5;
        m_timex_background.setLimit(frames);
    }

    void init()
    {
        m_maskPoints = m_config.mask_points;

        if (m_maskPoints.size()) {
            for (const auto p : m_maskPoints) {
                m_polyline.lineTo(p.x, p.y);
            }
        } else {
            // define default mask size
            int w = width - 4;
            int h = height - 4;

            Point start_point(width / 2 - w / 2, (height / 2) - h / 2);

            m_polyline.lineTo(start_point.x, start_point.y);
            m_polyline.lineTo(start_point.x + w, start_point.y);
            m_polyline.lineTo(start_point.x + w, start_point.y + h);
            m_polyline.lineTo(start_point.x, start_point.y + h);
            m_polyline.lineTo(start_point.x, start_point.y);

            for (const auto v : m_polyline.getVertices()) {
                m_maskPoints.push_back(Point(v.x, v.y));
            }
        }

        common::log("setup countour finder.");
        m_contour_finder.setMinAreaRadius(m_config.settings.minarearadius);
        m_contour_finder.setMaxAreaRadius(100);
        m_contour_finder.setThreshold(10);

        this->create_mask();
    }

    bool update(const Mat& frame)
    {
        m_frame_view = frame;

        convertColor(frame, m_gray, CV_RGB2GRAY);
        resize(m_gray, m_gray, Size(width, height));

#ifndef MOG2
        if (!m_first_set) {
            m_gray.copyTo(m_first);
            m_first_set = true;
        }

        m_gray.copyTo(m_second);

        // compute the absolute difference between frames
        absdiff(m_first, m_second, m_difference);

        threshold(m_difference, m_threshold, m_config.settings.minthreshold, 255, CV_THRESH_BINARY);

        blur(m_threshold, 20);
        dilate(m_threshold, 8);

        // copy and add mask
        m_threshold.copyTo(m_output, m_mask);
        m_gray.copyTo(m_mask_image, m_mask);
#else
        mog2->apply(m_gray, m_difference);

        blur(m_difference, 20);
        dilate(m_difference, 8);

        threshold(m_difference, m_threshold, m_config.settings.minthreshold, 255, CV_THRESH_BINARY);

        // copy and add mask
        m_threshold.copyTo(m_output, m_mask);
        m_gray.copyTo(m_mask_image, m_mask);
#endif

        bool found = this->find();

#ifndef MOG2
        if (m_timex_background.elapsed()) {
            m_gray.copyTo(m_first);
            m_timex_background.set();
        }
#endif

        return found;
    }

    ofPolyline& getMaskPolyLine()
    {
        //
        return m_polyline;
    }

    ofPolyline& getMaskPolyLineScaled()
    {
        if (!m_polyline_scaled.size()) this->polygonScaleUp();

        return m_polyline_scaled;
    }

    vector<Point>& getMaskPoints()
    {
        //
        return m_maskPoints;
    }

    vector<Point> getMaskPointsCopy()
    {
        //
        return m_maskPoints;
    }

    const Mat& getFrame()
    {
        //
        return m_gray;
    }

    const Mat& getMaskImage()
    {
        //
        return m_mask_image;
    }

    const Mat& getOutput()
    {
        //
        return m_output;
    }

    const int getWidth()
    {
        //
        return width;
    }

    const int getHeight()
    {
        //
        return height;
    }

    void updateMask()
    {
        //
        this->create_mask();
    }

    void resetMask()
    {
        m_maskPoints.clear();
        m_polyline.clear();
        m_polyline_scaled.clear();
    }

    void draw()
    {
        //
    }

  private:
    Ptr<cv::BackgroundSubtractorMOG2> mog2 = createBackgroundSubtractorMOG2(100, 16, false);
    bool m_first_set = false;

    Mat m_frame_view;
    Mat m_gray;
    Mat m_first;
    Mat m_second;
    Mat m_difference;
    Mat m_threshold;
    Mat m_mask;
    Mat m_output;
    Mat m_mask_image;

    Config& m_config = m_config.getInstance();

    ofPolyline m_polyline;
    ofPolyline m_polyline_scaled;

    vector<Point> m_maskPoints;

    ContourFinder m_contour_finder;

    common::Timex m_timex_background;

    bool find()
    {
        // start contour detection
        m_contour_finder.findContours(m_output);

        Rect m_max_rect = Rect(0, 0, 0, 0);
        bool found = false;

        int w = 0;
        int h = 0;

        // fast interation. filter the bigest rectangle
        for (auto& r : m_contour_finder.getBoundingRects()) {
            if ((r.width > w || r.height > h) && r.width > m_config.settings.minrectwidth &&
                r.height > m_config.settings.minrectwidth) {
                m_max_rect = Rect(r);

                w = r.width;
                h = r.height;

                found = true;
            }
        }

        if (found) ofNotifyEvent(on_motion, m_max_rect, this);

        return found;
    }

    void create_mask()
    {
        if (m_maskPoints.size() == 0) {
            m_maskPoints.push_back(cv::Point(2, 2));
            m_maskPoints.push_back(cv::Point(width - 2, 2));
            m_maskPoints.push_back(cv::Point(width - 2, height - 2));
            m_maskPoints.push_back(cv::Point(2, height - 2));
            m_maskPoints.push_back(cv::Point(2, 2));
        }

        CvMat* matrix = cvCreateMat(height, width, CV_8UC1);
        m_mask = cvarrToMat(matrix);

        for (int x = 0; x < m_mask.cols; x++) {
            for (int y = 0; y < m_mask.rows; y++) m_mask.at<uchar>(cv::Point(x, y)) = 0;
        }

        fillPoly(m_mask, m_maskPoints, 255);
    }

    void polygonScaleUp()
    {
        m_polyline_scaled = m_polyline;

        float scalex = static_cast<float>(m_frame_view.cols * 100 / width) / 100;
        float scaley = static_cast<float>(m_frame_view.rows * 100 / height) / 100;

        m_polyline_scaled.scale(scalex, scaley);
    }
};

