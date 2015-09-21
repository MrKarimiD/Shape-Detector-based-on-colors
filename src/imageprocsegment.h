#ifndef IMAGEPROCSEGMENT_H
#define IMAGEPROCSEGMENT_H

#include <QObject>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "shape.h"
#include "QTimer"
#include "Constants.h"
#include "QSemaphore"
#include <QDebug>

using namespace cv;
using namespace std;

//        Mat threshold_output;
//        vector<vector<Point> > contours;
//        vector<Vec4i> hierarchy;
//        RNG rng(12345);

class ImageProcSegment : public QObject
{
    Q_OBJECT
public:
    explicit ImageProcSegment(QObject *parent = 0);

    void shapeDetection(Mat input);

    void RobotDetection(Mat input);

    void setImage(Mat input);

    void addShape(float x,float y,double radius,string type,string color);

    //void setRanges(Vec3f minR,Vec3f maxR);

    void Start();

    void setColor(QString input);

    QList<Shape> detectedShapes;
    vector<vector<Point> > recycledShapes;
    vector<vector<Point> > robotList;

private:
    cv::Size imSize;
    //Mat cameraMatrix, distCoeffs;
    QString color;
    Point2f gravCenter;

    //QSemaphore *semaphoreForRanges;

    Mat input;
    QTimer *timer;
    bool newDataRecieved;
    Rect cropR;

    //Vec3f minColorRange,maxColorRange;

    bool checkAspectRatio(vector<Point> contours_poly);

    bool checkAspectRatioForRotatedRect(RotatedRect input);

    void prepareDataForOutput(std::vector<cv::Point>& contour,QString type);

    double angle(cv::Point pt1, cv::Point pt2, cv::Point pt0);

    void doProccess();

public slots:
    void timer_inteval();

signals:
    void dataGenerated();

public slots:

};

#endif // IMAGEPROCSEGMENT_H
