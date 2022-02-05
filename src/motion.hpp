#pragma once

#include "common.h"
#include "config.h"
#include "constants.h"
#include "ofxOpenCv.h"

class Motion
{
    const int width = 320;
    const int height = 240;

  public:
    Motion()
    {
        common::log("create default mask.");
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
        //    this->polygonScaleUp();
    }

    bool update(const Mat& frame)
    {
        convertColor(frame, m_gray, CV_RGB2GRAY);
        resize(m_gray, m_gray, Size(width, height));

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
        m_output.copyTo(m_mask_image);

        return true;
    }

    const Mat& getFrame() { return m_gray; }
    const Mat& getMaskImage() { return m_mask_image; }

    void resetMask()
    {
        m_maskPoints.clear();
        m_polyline.clear();
    }

    void draw() {}

  private:
    bool m_first_set = false;

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

    vector<Point> m_maskPoints;

    ContourFinder m_contour_finder;

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
        // m_polyline_resized = m_polyline;

        // float scalex = static_cast<float>(m_cam_width * 100 / m_proc_width) / 100;
        // float scaley = static_cast<float>(m_cam_height * 100 / m_proc_height) / 100;

        // m_polyline_resized.scale(scalex, scaley);
    }
};

