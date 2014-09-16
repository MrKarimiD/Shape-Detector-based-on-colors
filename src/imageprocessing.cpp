#include "imageprocessing.h"

ImageProcessing::ImageProcessing(QObject *parent) :
    QObject(parent)
{
    filterSetting=new filterSettings();
}

Mat ImageProcessing::applyFilters(Mat input)
{
    Mat src = input.clone();

    // Convert to grayscale
    Mat gray;
    cvtColor(src, gray, COLOR_BGR2GRAY);
    if(filterSetting->getUseMedianBlur())
    {
        medianBlur(gray,gray,filterSetting->getKernelSize());
    }

    Mat threshold_mat = Mat::zeros(gray.cols,gray.rows,CV_8UC1);
    if(filterSetting->getUseAdaptiveThresh())
    {
        adaptiveThreshold(gray,threshold_mat,255,ADAPTIVE_THRESH_GAUSSIAN_C,THRESH_BINARY_INV,
        filterSetting->getBlockSize(),
        filterSetting->getC());
    }
    else
    {
        if(filterSetting->getUseThreshold())
        {
            threshold( gray, threshold_mat,filterSetting->getThreshValue(),255,THRESH_BINARY_INV);
        }
        else
        {
            threshold_mat=gray.clone();
        }
    }

    if(filterSetting->getUseDilate())
    {
        Mat structure=getStructuringElement(MORPH_RECT,Size(filterSetting->getDilateSize(),filterSetting->getDilateSize()));
        dilate(threshold_mat,threshold_mat,structure);
    }

    // Use Canny instead of threshold to catch squares with gradient shading
    Mat bw;
    if(filterSetting->getUseCanny())
    {
        Canny(threshold_mat, bw,filterSetting->getFirstThresh(),filterSetting->getSecondThresh()
        , filterSetting->getApertureSize());
    }
    else
    {
        bw=threshold_mat.clone();
    }

    return bw;
}

void ImageProcessing::updateFilterSettings(filterSettings *fs)
{
    this->filterSetting = fs;
}

Mat ImageProcessing::undistortImage(Mat input)
{
    Mat cameraMatrix = Mat::eye(3, 3, CV_64F);

    cameraMatrix.at<double>(0,0) = 6.7055726712006776e+02;
    cameraMatrix.at<double>(0,1) = 0;
    cameraMatrix.at<double>(0,2) = 3.8950000000000000e+02;

    cameraMatrix.at<double>(1,0) = 0;
    cameraMatrix.at<double>(1,1) = 6.7055726712006776e+02;
    cameraMatrix.at<double>(1,2) = 2.8950000000000000e+02;

    cameraMatrix.at<double>(2,0) = 0;
    cameraMatrix.at<double>(2,1) = 0;
    cameraMatrix.at<double>(2,2) = 1;

    Mat distCoeffs = Mat::zeros(8, 1, CV_64F);

    distCoeffs.at<double>(0,0) = -3.7087470577837894e-01;
    distCoeffs.at<double>(1,0) = 1.8508542088322785e-01;
    distCoeffs.at<double>(2,0) = 0;
    distCoeffs.at<double>(3,0) = 0;
    distCoeffs.at<double>(4,0) = -3.4799226907590207e-02;

    Mat inputFrame;
    input.copyTo(inputFrame);

//    bitwise_not(inputFrame, inputFrame);

    Mat outputFrame;
//    undistort(input,outputFrame,cameraMatrix,distCoeffs);
    Size imageSize = input.size();
    Mat view, rview, map1, map2;

    initUndistortRectifyMap(cameraMatrix, distCoeffs, Mat(),
        getOptimalNewCameraMatrix(cameraMatrix, distCoeffs, imageSize, 0, imageSize, 0),
        imageSize, CV_16SC2, map1, map2);
    view = inputFrame;
    remap(view, rview, map1, map2, INTER_LINEAR);

//    //blur(outputFrame,outputFrame,Size(3,3));
//    //imshow("dis",input);
////    imshow("undis",rview);
    return rview;
}
