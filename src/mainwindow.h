#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "opencv2/core/core.hpp"
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <QDebug>
#include "QString"
#include "QFileDialog"
#include "imageprocessing.h"
#include "QTimer"
#include <QXmlStreamReader>
#include <QFile>
#include "filtersettings.h"
#include "QMouseEvent"
#include "QString"
#include "QSemaphore"
#include "QMessageBox"
#include "networksender.h"
#include "Prototype_Messages/GameGround.pb.h"
#include "System_Protobuf/SystemSettings.pb.h"
#include "Color_Protobuf/ColorSettings.pb.h"
#include <iostream>
#include <fstream>
#include "QRubberBand"
#include "camerasetting.h"
#include "imageprocsegment.h"
#include "QThread"

using namespace cv;
using namespace std;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_open_button_clicked();

    void cam_timeout();

    void on_camSet_checkBox_stateChanged();

    void callImageProcessingFunctions(Mat input_mat);

    void on_xml_checkBox_stateChanged();

    void on_xml_button_clicked();

    void on_cont_checkBox_stateChanged();

    void on_geom_checkBox_stateChanged();

    void on_bound_checkBox_stateChanged();

    void on_rotate_checkBox_stateChanged();

    //-----filter Setting Changed-----
    void on_undisort_checkBox_stateChanged();

    void on_crop_checkBox_stateChanged();

    void on_fX_lineEdit_textChanged();

    void on_fY_lineEdit_textChanged();

    void on_sX_lineEdit_textChanged();

    void on_sY_lineEdit_textChanged();

    void on_medianBlur_checkBox_stateChanged();

    void on_kernelSize_lineEdit_textChanged();

    void on_adaptiveThreshold_checkBox_stateChanged();

    void on_thresh_checkBox_stateChanged();

    void on_dilate_checkBox_stateChanged();

    void on_dilateSize_lineEdit_textChanged();

    void on_canny_checkBox_stateChanged();

    void on_blockSize_slider_sliderMoved(int position);

    void on_C_slider_sliderMoved(int position);

    void on_thresh_slider_sliderMoved(int position);

    void on_firstThresh_slider_sliderMoved(int position);

    void on_secondThresh_slider_sliderMoved(int position);

    void on_apertureSize_lineEdit_textChanged();

    //-----camera setting changed----------------
    void on_blue_slider_sliderMoved(int position);

    void on_red_slider_sliderMoved(int position);

    void on_exposure_slider_sliderMoved(int position);

    void on_brightness_slider_sliderMoved(int position);

    void updateCameraSetting();

    void setCameraSetting();

    void on_sharpness_slider_sliderMoved(int position);

    void on_gain_slider_sliderMoved(int position);
    //---------------------------------------------

    void on_mouse_button_clicked();

    //-------Mouse Functions---------
    bool eventFilter(QObject *obj, QEvent *event);

    void mousePressEvent(QMouseEvent *event);

    void mouseMoveEvent(QMouseEvent *event);

    void mouseReleaseEvent(QMouseEvent *event);

    void mouseDoubleClickEvent(QMouseEvent *event);

    //-------------------------------

    void on_drawCrop_checkBox_stateChanged();

    void on_firstM_rButton_toggled(bool checked);

    void on_secondM_rButton_toggled(bool checked);

    void on_go_button_clicked();

    void on_thirsM_rButton_toggled(bool checked);

    void sendDataPacket();

    void on_save_set_button_clicked();

    void on_open_set_button_clicked();

    void on_stall_button_clicked();

    void responseForFilterSettingsChanged();

    void on_hue_slider_sliderMoved(int position);

    void on_saturation_slider_sliderMoved(int position);

    void on_contrast_slider_sliderMoved(int position);

    void resposibleForRedOutput();

    void resposibleForBlueOutput();

    void resposibleForGreenOutput();

    void resposibleForYellowOutput();

    void resposibleForVioletOutput();

    void resposibleForBlackOutput();

    void checkAllOfRecieved();

    void addRedImage(Mat out);

    void addBlueImage(Mat out);

    void addGreenImage(Mat out);

    void addYellowImage(Mat out);

    void addVioletImage(Mat out);

    void addBlackImage(Mat out);

    void on_red_min_hue_slider_sliderMoved(int position);

    void on_red_max_hue_slider_sliderMoved(int position);

    void on_red_min_sat_slider_sliderMoved(int position);

    void on_red_max_sat_slider_sliderMoved(int position);

    void on_red_min_val_slider_sliderMoved(int position);

    void on_red_max_val_slider_sliderMoved(int position);

    void on_blue_min_hue_slider_sliderMoved(int position);

    void on_blue_max_hue_slider_sliderMoved(int position);

    void on_blue_min_sat_slider_sliderMoved(int position);

    void on_blue_max_sat_slider_sliderMoved(int position);

    void on_blue_min_val_slider_sliderMoved(int position);

    void on_blue_max_val_slider_sliderMoved(int position);

    void on_green_min_hue_slider_sliderMoved(int position);

    void on_green_max_hue_slider_sliderMoved(int position);

    void on_green_min_sat_slider_sliderMoved(int position);

    void on_green_max_sat_slider_sliderMoved(int position);

    void on_green_min_val_slider_sliderMoved(int position);

    void on_green_max_val_slider_sliderMoved(int position);

    void on_yellow_min_hue_slider_sliderMoved(int position);

    void on_yellow_max_hue_slider_sliderMoved(int position);

    void on_yellow_min_sat_slider_sliderMoved(int position);

    void on_yellow_max_sat_slider_sliderMoved(int position);

    void on_yellow_min_val_slider_sliderMoved(int position);

    void on_yellow_max_val_slider_sliderMoved(int position);

    void on_violet_min_hue_slider_sliderMoved(int position);

    void on_violet_max_hue_slider_sliderMoved(int position);

    void on_violet_min_sat_slider_sliderMoved(int position);

    void on_violet_max_sat_slider_sliderMoved(int position);

    void on_violet_min_val_slider_sliderMoved(int position);

    void on_violet_max_val_slider_sliderMoved(int position);

    void on_black_min_hue_slider_sliderMoved(int position);

    void on_black_max_hue_slider_sliderMoved(int position);

    void on_black_min_sat_slider_sliderMoved(int position);

    void on_black_max_sat_slider_sliderMoved(int position);

    void on_black_min_val_slider_sliderMoved(int position);

    void on_black_max_val_slider_sliderMoved(int position);

    void on_import_button_clicked();

    Mat QImage2Mat(QImage src);

    QImage Mat2QImage(Mat const& src);

    void receiveUDPPacket();

    void on_lines_button_clicked();

    void on_clearLines_button_clicked();

private:
    Ui::MainWindow *ui;
    ImageProcessing *imageProcessor;
    QTimer *cam_timer,*send_timer;
    VideoCapture cap;
    bool cameraIsOpened,mouseButtonClicked,firstPointSelectedIsValid;
    bool permissionForSending;
    filterSettings *filterSetting;
    cameraSetting *camSetting;

    NetworkSender *sendingSocket;
    int mission;
    bool colorMode;

    //------
    bool lineDrawing;
    bool firstLinePointIsValid;
    cv::Point firstLinePoint;
    //--------

    bool stallMode;
    QString whichColor;
    QPoint origin;
    QRubberBand *rubberBand;
    QSemaphore *access2StallMode;
    QSemaphore *semaphoreForOutput;
    QTimer *checkTimer;

    ImageProcSegment *redProc,*blueProc,*greenProc
    ,*yellowProc,*violetProc,*blackProc;

    QThread *redThread,*blueThread,*greenThread
    ,*yellowThread,*violetThread,*blackThread;

    QSemaphore *redSem,*blueSem,*greenSem
    ,*yellowSem,*violetSem,*blackSem,*realSem;

    QList<Shape> red_shapes;
    QList<Shape> blue_shapes;
    QList<Shape> green_shapes;
    QList<Shape> yellow_shapes;
    QList<Shape> violet_shapes;
    QList<Shape> black_shapes;

    bool RecievedData[7];
    QSemaphore *semaphoreForDataPlussing;

    Mat filterColor[7];

    //------UDP Image Recieving
    QUdpSocket *recSocket;
    QByteArray udp_datagram;
    QImage udpImage1;
    Mat udpFrame;
    int udpPort;
    bool imageRecievedFromNetwork;

    //-------------------

    QList<Point> lineBorders;

    Size imSize;
    Rect cropedRect;

    void enableCameraSetting();

    void disableCameraSetting();

    void enableOpenCamera();

    void disableOpenCamera();

    void enableXML();

    void disableXML();

    void updateOutputOptions();

    void enableMedianBlur();

    void disableMedianBlur();

    void enableAdaptiveThresholdSetting();

    void disableAdaptiveThresholdSetting();

    void enableThresholdSetting();

    void disableThresholdSetting();

    void enableDilateSetting();

    void disableDilateSetting();

    void enableCannySetting();

    void disableCannySetting();

    void updateFilterSetting();

    void enableCropSetting();

    void disableCropSetting();

    void enableFirstMission();

    void disableFirstMission();

    void enableSecondMission();

    void disableSecondMission();

    bool isValidPlaceForSelect(int x,int y);

    void setInitializeMessage(int mission);

    void preapreDataForSending();

    void openSetting(QString fileAddress);

    void addHSVSettings();

    Mat returnFilterImage(Mat input,QString color);

    void disableVideoSetting();

signals:
    void imageReady(Mat image);
    void cameraSettingChanged();
    void dataReadyForSend();
    void filterSettingChanged();

};

#endif // MAINWINDOW_H
