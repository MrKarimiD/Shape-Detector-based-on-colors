#ifndef IMAGEPROCESSING_H
#define IMAGEPROCESSING_H
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <QDebug>
#include "QString"
#include "QFileDialog"
#include "filtersettings.h"
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/nonfree/cuda.hpp>
#include <Prototype_Messages/GameGround.pb.h>
#include "Constants.h"
#include "shape.h"
#include "QTimer"

using namespace cv;
using namespace std;

#include <QObject>

class ImageProcessing : public QObject
{
    Q_OBJECT
public:
    explicit ImageProcessing(QObject *parent = 0);

    Mat undistortUSBCam(Mat input);
    Mat undistortFIREWIRECam(Mat input);

    Mat applyFilters(Mat input);

    void updateFilterSettings(filterSettings *fs);

    outputPacket result;

private:
    filterSettings *filterSetting;
    cv::Size imSize;
    //Mat cameraMatrix, distCoeffs;
    QString color;
    vector<vector<Point> > contours;

signals:

public slots:

};

#endif // IMAGEPROCESSING_H
