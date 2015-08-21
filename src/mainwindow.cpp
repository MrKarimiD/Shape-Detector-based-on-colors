#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    installEventFilter(this);

    qRegisterMetaType<Mat>("Mat");

    cameraIsOpened=false;
    mouseButtonClicked=false;
    colorMode = false;
    lineDrawing = false;
    firstPointSelectedIsValid=false;

    redProc = new ImageProcSegment();
    redProc->setColor("red");

    blueProc = new ImageProcSegment();
    blueProc->setColor("blue");

    greenProc = new ImageProcSegment();
    greenProc->setColor("green");

    yellowProc = new ImageProcSegment();
    yellowProc->setColor("yellow");

    violetProc = new ImageProcSegment();
    violetProc->setColor("violet");

    cyanProc = new ImageProcSegment();
    cyanProc->setColor("cyan");

    blackProc = new ImageProcSegment();
    blackProc->setColor("black");

    //    connect(redProc,SIGNAL(afterFilter(Mat)),this,SLOT(addRedImage(Mat)));
    //    connect(blueProc,SIGNAL(afterFilter(Mat)),this,SLOT(addBlueImage(Mat)));
    //    connect(greenProc,SIGNAL(afterFilter(Mat)),this,SLOT(addGreenImage(Mat)));
    //    connect(yellowProc,SIGNAL(afterFilter(Mat)),this,SLOT(addYellowImage(Mat)));
    //    connect(violetProc,SIGNAL(afterFilter(Mat)),this,SLOT(addVioletImage(Mat)));
    //    connect(cyanProc,SIGNAL(afterFilter(Mat)),this,SLOT(addCyanImage(Mat)));
    //    connect(blackProc,SIGNAL(afterFilter(Mat)),this,SLOT(addBlackImage(Mat)));

    addHSVSettings();

    redThread = new QThread();
    blueThread = new QThread();
    greenThread = new QThread();
    yellowThread = new QThread();
    cyanThread = new QThread();
    violetThread = new QThread();
    blackThread = new QThread();

    redSem = new QSemaphore();
    blueSem = new QSemaphore();
    greenSem = new QSemaphore();
    yellowSem = new QSemaphore();
    cyanSem = new QSemaphore();
    violetSem = new QSemaphore();
    blackSem = new QSemaphore();
    realSem = new QSemaphore();

    redProc->moveToThread(redThread);
    blueProc->moveToThread(blueThread);
    greenProc->moveToThread(greenThread);
    yellowProc->moveToThread(yellowThread);
    cyanProc->moveToThread(cyanThread);
    violetProc->moveToThread(violetThread);
    blackProc->moveToThread(blackThread);

    stallMode = false;

    permissionForSending = false;

    rubberBand = new QRubberBand(QRubberBand::Rectangle, this);

    for(int i=0;i<7;i++)
    {
        RecievedData[i] = false;
    }
    semaphoreForDataPlussing = new QSemaphore(7);

    sendingSocket = new NetworkSender();
    access2StallMode = new QSemaphore(1);
    semaphoreForOutput = new QSemaphore(1);

    QStringList items;
    items<<"0"<<"1"<<"USB0"<<"USB1"<<"Network"<<"Video";
    ui->cam_comboBox->addItems(items);
    ui->cam_comboBox->setCurrentIndex(4);

    QStringList fps_items;
    fps_items<<"15"<<"30"<<"60";
    ui->fps_comboBox->addItems(fps_items);

    QStringList output_items;
    output_items<<"Red"<<"Blue"<<"Green"<<"Yellow"<<"Violet"<<"Cyan"<<"Black"<<"Final";
    ui->out_comboBox->addItems(output_items);
    ui->out_comboBox->setCurrentIndex(7);

    cam_timer = new QTimer();
    send_timer = new QTimer();
    filterSetting=new filterSettings();
    camSetting = new cameraSetting();

    checkTimer = new QTimer();
    connect(checkTimer,SIGNAL(timeout()),this,SLOT(checkAllOfRecieved()));

    connect(this,SIGNAL(imageReady(Mat)),this,SLOT(callImageProcessingFunctions(Mat)));
    connect(this,SIGNAL(cameraSettingChanged()),this,SLOT(updateCameraSetting()));
    connect(this,SIGNAL(dataReadyForSend()),this,SLOT(sendDataPacket()));
    connect(this,SIGNAL(filterSettingChanged()),this,SLOT(responseForFilterSettingsChanged()));
    //connect(send_timer,SIGNAL(timeout()),this,SLOT(send_timer_interval()));

    connect(redProc,SIGNAL(dataGenerated()),this,SLOT(resposibleForRedOutput()));
    connect(blueProc,SIGNAL(dataGenerated()),this,SLOT(resposibleForBlueOutput()));
    connect(greenProc,SIGNAL(dataGenerated()),this,SLOT(resposibleForGreenOutput()));
    connect(yellowProc,SIGNAL(dataGenerated()),this,SLOT(resposibleForYellowOutput()));
    connect(violetProc,SIGNAL(dataGenerated()),this,SLOT(resposibleForVioletOutput()));
    connect(cyanProc,SIGNAL(dataGenerated()),this,SLOT(resposibleForCyanOutput()));
    connect(blackProc,SIGNAL(dataGenerated()),this,SLOT(resposibleForBlackOutput()));

    imageProcessor=new ImageProcessing();

    udpPort = 6000;

    recSocket= new QUdpSocket(this);

    openSetting("setting-color.txt");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_open_button_clicked()
{
    sendingSocket->configure(ui->ip_lineEdit->text(),ui->port_lineEdit->text().toInt());

    Mat frame;

    imageRecievedFromNetwork = false;

    if(ui->cam_comboBox->currentText() == "Network")
    {
        recSocket->bind(udpPort);
        imageRecievedFromNetwork = true;
        connect(recSocket,SIGNAL(readyRead()),this,SLOT(receiveUDPPacket()));
    }
    else
    {
        if(ui->cam_comboBox->currentText()=="0")
        {
            cameraIsOpened=cap.open(CAP_FIREWIRE+0);
        }
        else if(ui->cam_comboBox->currentText()=="1")
        {
            cameraIsOpened=cap.open(CAP_FIREWIRE+1);
        }
        else if(ui->cam_comboBox->currentText()=="USB0")
        {
            cameraIsOpened=cap.open(0);
        }
        else if(ui->cam_comboBox->currentText()=="USB1")
        {
            cameraIsOpened=cap.open(1);
        }
        else if(ui->cam_comboBox->currentText()=="Video")
        {
            cap.open(ui->videoAdd_lineEdit->text().toStdString());
        }

        if(!imageRecievedFromNetwork)
        {
            setCameraSetting();

            cap.read(frame);
        }

        cam_timer->start(1000*(1/ui->fps_comboBox->currentText().toInt()));
        connect(cam_timer,SIGNAL(timeout()),this,SLOT(cam_timeout()));
        emit imageReady(frame);
    }

    int numberOfColors = 0;

    disableVideoSetting();

    if(ui->use_red_checkBox->isChecked())
    {
        numberOfColors++;
        redThread->start();
        redProc->Start();
    }

    if(ui->use_blue_checkBox->isChecked())
    {
        numberOfColors++;
        blueThread->start();
        blueProc->Start();
    }

    if(ui->use_green_checkBox->isChecked())
    {
        numberOfColors++;
        greenThread->start();
        greenProc->Start();
    }

    if(ui->use_yellow_checkBox->isChecked())
    {
        numberOfColors++;
        yellowThread->start();
        yellowProc->Start();
    }

    if(ui->use_violet_checkBox->isChecked())
    {
        numberOfColors++;
        violetThread->start();
        violetProc->Start();
    }

    if(ui->use_cyan_checkBox->isChecked())
    {
        numberOfColors++;
        cyanThread->start();
        cyanProc->Start();
    }

    if(ui->use_black_checkBox->isChecked())
    {
        numberOfColors++;
        blackThread->start();
        blackProc->Start();
    }
}

void MainWindow::cam_timeout()
{
    Mat frame;
    cap.read(frame);
    if(frame.empty())
    {
        if(ui->cam_comboBox->currentText() == "Video")
        {
            cap.release();
            cap.open(ui->videoAdd_lineEdit->text().toStdString());
        }
    }
    else
    {
        emit imageReady(frame);
    }
}

void MainWindow::enableCameraSetting()
{
    ui->red_slider->setEnabled(true);
    ui->blue_slider->setEnabled(true);
    ui->exposure_slider->setEnabled(true);
    ui->brightness_slider->setEnabled(true);
    ui->gain_slider->setEnabled(true);
    ui->sharpness_slider->setEnabled(true);

    ui->red_label->setEnabled(true);
    ui->blue_label->setEnabled(true);
    ui->expo_label->setEnabled(true);
    ui->brightness_label->setEnabled(true);
    ui->gain_label->setEnabled(true);
    ui->sharpness_label->setEnabled(true);

    ui->redOut_label->setEnabled(true);
    ui->blueOut_label->setEnabled(true);
    ui->expoOut_label->setEnabled(true);
    ui->brightnessOut_label->setEnabled(true);
    ui->gainOut_label->setEnabled(true);
    ui->sharpnessOut_label->setEnabled(true);
}

void MainWindow::disableCameraSetting()
{
    ui->red_slider->setDisabled(true);
    ui->blue_slider->setDisabled(true);
    ui->exposure_slider->setDisabled(true);
    ui->brightness_slider->setDisabled(true);
    ui->gain_slider->setDisabled(true);
    ui->sharpness_slider->setDisabled(true);

    ui->red_label->setDisabled(true);
    ui->blue_label->setDisabled(true);
    ui->expo_label->setDisabled(true);
    ui->brightness_label->setDisabled(true);
    ui->gain_label->setDisabled(true);
    ui->sharpness_label->setDisabled(true);

    ui->redOut_label->setDisabled(true);
    ui->blueOut_label->setDisabled(true);
    ui->expoOut_label->setDisabled(true);
    ui->brightnessOut_label->setDisabled(true);
    ui->gainOut_label->setDisabled(true);
    ui->sharpnessOut_label->setDisabled(true);
}

void MainWindow::enableOpenCamera()
{
    ui->cam_label->setEnabled(true);
    ui->cam_comboBox->setEnabled(true);
    ui->open_button->setEnabled(true);
    ui->camSet_checkBox->setEnabled(true);
    ui->xml_checkBox->setEnabled(true);
}

void MainWindow::disableOpenCamera()
{
    ui->cam_label->setDisabled(true);
    ui->cam_comboBox->setDisabled(true);
    ui->open_button->setDisabled(true);
    ui->camSet_checkBox->setDisabled(true);
    ui->xml_checkBox->setDisabled(true);
}

void MainWindow::enableXML()
{
    ui->xmlAdd_lineEdit->setEnabled(true);
    ui->xml_button->setEnabled(true);
}

void MainWindow::disableXML()
{
    ui->xmlAdd_lineEdit->setDisabled(true);
    ui->xml_button->setDisabled(true);
}

void MainWindow::updateOutputOptions()
{
}

void MainWindow::enableMedianBlur()
{
    ui->kernekSize_label->setEnabled(true);
    ui->kernelSize_lineEdit->setEnabled(true);
}

void MainWindow::disableMedianBlur()
{
    ui->kernekSize_label->setDisabled(true);
    ui->kernelSize_lineEdit->setDisabled(true);
}

void MainWindow::enableAdaptiveThresholdSetting()
{
    ui->blockSizeOut_label->setEnabled(true);
    ui->blockSize_label->setEnabled(true);
    ui->blockSize_slider->setEnabled(true);

    ui->c_label->setEnabled(true);
    ui->cOut_label->setEnabled(true);
    ui->C_slider->setEnabled(true);
}

void MainWindow::disableAdaptiveThresholdSetting()
{
    ui->blockSizeOut_label->setDisabled(true);
    ui->blockSize_label->setDisabled(true);
    ui->blockSize_slider->setDisabled(true);

    ui->c_label->setDisabled(true);
    ui->cOut_label->setDisabled(true);
    ui->C_slider->setDisabled(true);
}

void MainWindow::enableThresholdSetting()
{
    ui->threshOut_label->setEnabled(true);
    ui->thresh_slider->setEnabled(true);
}

void MainWindow::disableThresholdSetting()
{
    ui->threshOut_label->setDisabled(true);
    ui->thresh_slider->setDisabled(true);
}

void MainWindow::enableDilateSetting()
{
    ui->dilateSize_lineEdit->setEnabled(true);
    ui->dilateSize_label->setEnabled(true);
}

void MainWindow::disableDilateSetting()
{
    ui->dilateSize_lineEdit->setDisabled(true);
    ui->dilateSize_label->setDisabled(true);
}

void MainWindow::enableCannySetting()
{
    ui->firstThreshOut_label->setEnabled(true);
    ui->firstThresh_label->setEnabled(true);
    ui->firstThresh_slider->setEnabled(true);

    ui->secondThreshOut_label->setEnabled(true);
    ui->secondThresh_label->setEnabled(true);
    ui->secondThresh_slider->setEnabled(true);

    ui->apertureSize_label->setEnabled(true);
    ui->apertureSize_lineEdit->setEnabled(true);
}

void MainWindow::disableCannySetting()
{
    ui->firstThreshOut_label->setDisabled(true);
    ui->firstThresh_label->setDisabled(true);
    ui->firstThresh_slider->setDisabled(true);

    ui->secondThreshOut_label->setDisabled(true);
    ui->secondThresh_label->setDisabled(true);
    ui->secondThresh_slider->setDisabled(true);

    ui->apertureSize_label->setDisabled(true);
    ui->apertureSize_lineEdit->setDisabled(true);
}

void MainWindow::updateFilterSetting()
{
    filterSetting->setUseUndisort(ui->undisort_checkBox->isChecked());

    filterSetting->setUseCrop(ui->crop_checkBox->isChecked());
    filterSetting->setCrop_firstX(ui->fX_lineEdit->text().toInt());
    filterSetting->setCrop_firstY(ui->fY_lineEdit->text().toInt());
    filterSetting->setCrop_secondX(ui->sX_lineEdit->text().toInt());
    filterSetting->setCrop_secondY(ui->sY_lineEdit->text().toInt());

    filterSetting->setUseMedianBlur(ui->medianBlur_checkBox->isChecked());
    filterSetting->setKernelSize(ui->kernelSize_lineEdit->text().toInt());

    filterSetting->setUseAdaptiveThresh(ui->adaptiveThreshold_checkBox->isChecked());
    filterSetting->setBlockSize(ui->blockSize_slider->value());
    filterSetting->setC(ui->C_slider->value());

    filterSetting->setUseThreshold(ui->thresh_checkBox->isChecked());
    filterSetting->setThreshValue(ui->thresh_slider->value());

    filterSetting->setUseDilate(ui->dilate_checkBox->isChecked());
    filterSetting->setDilateSize(ui->dilateSize_lineEdit->text().toInt());

    filterSetting->setUseCanny(ui->canny_checkBox->isChecked());
    filterSetting->setFirstThresh(ui->firstThresh_slider->value());
    filterSetting->setSecondThresh(ui->secondThresh_slider->value());
    filterSetting->setApertureSize(ui->apertureSize_lineEdit->text().toInt());
}

void MainWindow::enableCropSetting()
{
    ui->fPoint_label->setEnabled(true);
    ui->fVirgoul_label->setEnabled(true);
    ui->fX_lineEdit->setEnabled(true);
    ui->fY_lineEdit->setEnabled(true);

    ui->sPoint_label->setEnabled(true);
    ui->sVirgoul_label->setEnabled(true);
    ui->sX_lineEdit->setEnabled(true);
    ui->sY_lineEdit->setEnabled(true);

    //ui->mouse_button->setEnabled(true);
}

void MainWindow::disableCropSetting()
{
    ui->fPoint_label->setDisabled(true);
    ui->fVirgoul_label->setDisabled(true);
    ui->fX_lineEdit->setDisabled(true);
    ui->fY_lineEdit->setDisabled(true);

    ui->sPoint_label->setDisabled(true);
    ui->sVirgoul_label->setDisabled(true);
    ui->sX_lineEdit->setDisabled(true);
    ui->sY_lineEdit->setDisabled(true);

    //ui->mouse_button->setDisabled(true);
}

void MainWindow::enableFirstMission()
{
    ui->region1_groupBox->setEnabled(true);

    ui->region2_groupBox->setEnabled(true);

    ui->fMendX_lineEdit->setEnabled(true);
    ui->fMendY_lineEdit->setEnabled(true);
    ui->fMend_label->setEnabled(true);
    ui->fMVirgoul_label->setEnabled(true);
}

void MainWindow::disableFirstMission()
{
    ui->region1_groupBox->setDisabled(true);

    ui->region2_groupBox->setDisabled(true);

    ui->fMendX_lineEdit->setDisabled(true);
    ui->fMendY_lineEdit->setDisabled(true);
    ui->fMend_label->setDisabled(true);
    ui->fMVirgoul_label->setDisabled(true);
}

void MainWindow::enableSecondMission()
{
    ui->sMendX_lineEdit->setEnabled(true);
    ui->sMendY_lineEdit->setEnabled(true);
    ui->sMend_label->setEnabled(true);
    ui->lines_button->setEnabled(true);
    ui->clearLines_button->setEnabled(true);
}

void MainWindow::disableSecondMission()
{
    ui->sMendX_lineEdit->setDisabled(true);
    ui->sMendY_lineEdit->setDisabled(true);
    ui->sMend_label->setDisabled(true);
    ui->lines_button->setDisabled(true);
    ui->clearLines_button->setDisabled(true);
}

bool MainWindow::isValidPlaceForSelect(int x, int y)
{
    bool isValid = false ;

    if((x > ui->outputLabel->x() && x<(ui->outputLabel->x()+ui->outputLabel->width())))
    {
        if(y>ui->outputLabel->y() && y<ui->outputLabel->y()+ui->outputLabel->height())
        {
            isValid = true;
        }
    }

    return isValid;
}

void MainWindow::setInitializeMessage(int mission)
{
    switch(mission)
    {
    case 1 :
    {
        this->mission = 1;

        imageProcessor->result.set_mission(1);
        imageProcessor->result.set_type(0);
        imageProcessor->result.set_numberofshape(0);

        imageProcessor->result.set_mission1_isvalid(true);
        imageProcessor->result.set_mission1_region1_tl_x(ui->region1_tlX_lineEdit->text().toFloat());
        imageProcessor->result.set_mission1_region1_tl_y(ui->region1_tlY_lineEdit->text().toFloat());
        imageProcessor->result.set_mission1_region1_br_x(ui->region1_brX_lineEdit->text().toFloat());
        imageProcessor->result.set_mission1_region1_br_y(ui->region1_brY_lineEdit->text().toFloat());

        imageProcessor->result.set_mission1_region2_tl_x(ui->region2_tlX_lineEdit->text().toFloat());
        imageProcessor->result.set_mission1_region2_tl_y(ui->region2_tlY_lineEdit->text().toFloat());
        imageProcessor->result.set_mission1_region2_br_x(ui->region2_brX_lineEdit->text().toFloat());
        imageProcessor->result.set_mission1_region2_br_y(ui->region2_brY_lineEdit->text().toFloat());

        imageProcessor->result.set_mission1_end_x(ui->fMendX_lineEdit->text().toFloat());
        imageProcessor->result.set_mission1_end_y(ui->fMendY_lineEdit->text().toFloat());

        break;
    }
    case 2 :
    {
        this->mission = 2;

        imageProcessor->result.set_mission(2);
        imageProcessor->result.set_type(0);
        imageProcessor->result.set_numberofshape(0);

        imageProcessor->result.set_mission2_isvalid(true);
        imageProcessor->result.set_mission2_end_x(ui->sMendX_lineEdit->text().toFloat());
        imageProcessor->result.set_mission2_end_y(ui->sMendY_lineEdit->text().toFloat());


        for(int i=0;i<lineBorders.size()-1;i++)
        {
            outputPacket_line *line=imageProcessor->result.add_mission2_lines();
            int startX = Orgin_X + ((float)(lineBorders.at(i).x-cropedRect.x)/imSize.width)*Width;
            int startY = Orgin_Y - ((float)(lineBorders.at(i).y-cropedRect.y)/imSize.height)*Height;

            int endX = Orgin_X + ((float)(lineBorders.at(i+1).x-cropedRect.x)/imSize.width)*Width;
            int endY = Orgin_Y - ((float)(lineBorders.at(i+1).y-cropedRect.y)/imSize.height)*Height;

            qDebug()<<"start:"<<startX<<","<<startY;
            qDebug()<<"end:"<<endX<<","<<endY;
            line->set_start_x(startX);
            line->set_start_y(startY);
            line->set_end_x(endX);
            line->set_end_y(endY);
        }
        break;
    }
    case 3:
    {
        this->mission = 3;

        imageProcessor->result.set_mission(3);
        imageProcessor->result.set_type(0);
        imageProcessor->result.set_numberofshape(0);

        imageProcessor->result.set_mission3_isvalid(true);

        if(ui->attacker_rButton->isChecked())
            imageProcessor->result.set_mission3_isattacker(true);
        else if(ui->defender_rButton->isChecked())
            imageProcessor->result.set_mission3_isattacker(false);

        //Border Center
        imageProcessor->result.set_mission3_circularborde_x(ui->border_X_lineEdit->text().toFloat());
        imageProcessor->result.set_mission3_circularborde_y(ui->border_Y_lineEdit->text().toFloat());

        //goal1 center
        imageProcessor->result.set_mission3_goal1_x(ui->goal1_X_lineEdit->text().toFloat());
        imageProcessor->result.set_mission3_goal1_y(ui->goal1_Y_lineEdit->text().toFloat());

        //goal2 center
        imageProcessor->result.set_mission3_goal2_x(ui->goal2_X_lineEdit->text().toFloat());
        imageProcessor->result.set_mission3_goal2_y(ui->goal2_Y_lineEdit->text().toFloat());

        break;
    }
    }
    emit dataReadyForSend();
}

void MainWindow::preapreDataForSending()
{

}

void MainWindow::openSetting(QString fileAddress)
{
    SystemSettings setting;

    fstream input;
    input.open(fileAddress.toUtf8(), ios::in | ios::binary);
    if (!input)
    {
        qDebug() << fileAddress << ": File not found.  Creating a new file." << endl;

    }
    else if (!setting.ParseFromIstream(&input))
    {
        qDebug() << "Failed";
    }
    else
    {
        ui->camSet_checkBox->setCheckable(setting.input_edit_camera_setting());

        ui->blue_slider->setValue(setting.input_white_balance_blue_u());
        ui->blueOut_label->setText(QString::number(setting.input_white_balance_blue_u()));

        ui->red_slider->setValue(setting.input_white_balance_red_v());
        ui->redOut_label->setText(QString::number(setting.input_white_balance_red_v()));

        ui->exposure_slider->setValue(setting.input_exposure());
        ui->expoOut_label->setText(QString::number(setting.input_exposure()));

        ui->brightness_slider->setValue(setting.input_brightness());
        ui->brightnessOut_label->setText(QString::number(setting.input_brightness()));

        ui->sharpness_slider->setValue(setting.input_sharpness());
        ui->sharpnessOut_label->setText(QString::number(setting.input_sharpness()));

        ui->gain_slider->setValue(setting.input_gain());
        ui->gainOut_label->setText(QString::number(setting.input_gain()));

        ui->hue_slider->setValue(setting.input_hue());
        ui->hueOut_label->setText(QString::number(setting.input_hue()));

        ui->saturation_slider->setValue(setting.input_sat());
        ui->saturationOut_label->setText(QString::number(setting.input_sat()));

        ui->contrast_slider->setValue(setting.input_contrast());
        ui->contrastOut_label->setText(QString::number(setting.input_contrast()));

        ui->ip_lineEdit->setText(QString::fromStdString(setting.input_network_ip()));
        ui->port_lineEdit->setText(QString::fromStdString(setting.input_network_port()));

        ui->crop_checkBox->setChecked(setting.filters_crop_photo());
        ui->fX_lineEdit->setText(QString::fromStdString(setting.filters_crop_firstpoint_x()));
        ui->fY_lineEdit->setText(QString::fromStdString(setting.filters_crop_firstpoint_y()));
        ui->sX_lineEdit->setText(QString::fromStdString(setting.filters_crop_secondpoint_x()));
        ui->sY_lineEdit->setText(QString::fromStdString(setting.filters_crop_secondpoint_y()));

        ui->medianBlur_checkBox->setChecked(setting.filters_median_blur());
        ui->kernelSize_lineEdit->setText(QString::fromStdString(setting.filters_median_blur_kernelsize()));

        ui->adaptiveThreshold_checkBox->setChecked(setting.filters_adaptive_threshold());
        ui->blockSize_slider->setValue(setting.filters_adaptive_threshold_blocksize());
        ui->blockSizeOut_label->setText(QString::number(setting.filters_adaptive_threshold_blocksize()));
        ui->C_slider->setValue(setting.filters_adaptive_threshold_c());
        ui->cOut_label->setText(QString::number(setting.filters_adaptive_threshold_c()));

        ui->thresh_checkBox->setChecked(setting.filters_threshold());
        ui->thresh_slider->setValue(setting.filters_threshold_value());
        ui->threshOut_label->setText(QString::number(setting.filters_threshold_value()));

        ui->dilate_checkBox->setChecked(setting.filters_dilate());
        ui->dilateSize_lineEdit->setText(QString::fromStdString(setting.filters_dilationsize()));

        ui->canny_checkBox->setChecked(setting.filters_canny());
        ui->firstThresh_slider->setValue(setting.filters_canny_first_threshold());
        ui->firstThreshOut_label->setText(QString::number(setting.filters_canny_first_threshold()));
        ui->secondThresh_slider->setValue(setting.filters_canny_second_threshold());
        ui->secondThreshOut_label->setText(QString::number(setting.filters_canny_second_threshold()));
        ui->apertureSize_lineEdit->setText(QString::fromStdString(setting.filters_canny_aperturesize()));

        //Add red Colors
        ui->red_min_hue_slider->setValue(setting.red_instances(0).min_hue());
        ui->red_min_hue_label->setText(QString::number(setting.red_instances(0).min_hue()));

        ui->red_min_sat_slider->setValue(setting.red_instances(0).min_sat());
        ui->red_min_sat_label->setText(QString::number(setting.red_instances(0).min_sat()));

        ui->red_min_val_slider->setValue(setting.red_instances(0).min_val());
        ui->red_min_val_label->setText(QString::number(setting.red_instances(0).min_val()));

        ui->red_max_hue_slider->setValue(setting.red_instances(0).max_hue());
        ui->red_max_hue_label->setText(QString::number(setting.red_instances(0).max_hue()));

        ui->red_max_sat_slider->setValue(setting.red_instances(0).max_sat());
        ui->red_max_sat_label->setText(QString::number(setting.red_instances(0).max_sat()));

        ui->red_max_val_slider->setValue(setting.red_instances(0).max_val());
        ui->red_max_val_label->setText(QString::number(setting.red_instances(0).max_val()));

        //Add blue colors
        ui->blue_min_hue_slider->setValue(setting.blue_instances(0).min_hue());
        ui->blue_min_hue_label->setText(QString::number(setting.blue_instances(0).min_hue()));

        ui->blue_min_sat_slider->setValue(setting.blue_instances(0).min_sat());
        ui->blue_min_sat_label->setText(QString::number(setting.blue_instances(0).min_sat()));

        ui->blue_min_val_slider->setValue(setting.blue_instances(0).min_val());
        ui->blue_min_val_label->setText(QString::number(setting.blue_instances(0).min_val()));

        ui->blue_max_hue_slider->setValue(setting.blue_instances(0).max_hue());
        ui->blue_max_hue_label->setText(QString::number(setting.blue_instances(0).max_hue()));

        ui->blue_max_sat_slider->setValue(setting.blue_instances(0).max_sat());
        ui->blue_max_sat_label->setText(QString::number(setting.blue_instances(0).max_sat()));

        ui->blue_max_val_slider->setValue(setting.blue_instances(0).max_val());
        ui->blue_max_val_label->setText(QString::number(setting.blue_instances(0).max_val()));

        //Add green colors
        ui->green_min_hue_slider->setValue(setting.green_instances(0).min_hue());
        ui->green_min_hue_label->setText(QString::number(setting.green_instances(0).min_hue()));

        ui->green_min_sat_slider->setValue(setting.green_instances(0).min_sat());
        ui->green_min_sat_label->setText(QString::number(setting.green_instances(0).min_sat()));

        ui->green_min_val_slider->setValue(setting.green_instances(0).min_val());
        ui->green_min_val_label->setText(QString::number(setting.green_instances(0).min_val()));

        ui->green_max_hue_slider->setValue(setting.green_instances(0).max_hue());
        ui->green_max_hue_label->setText(QString::number(setting.green_instances(0).max_hue()));

        ui->green_max_sat_slider->setValue(setting.green_instances(0).max_sat());
        ui->green_max_sat_label->setText(QString::number(setting.green_instances(0).max_sat()));

        ui->green_max_val_slider->setValue(setting.green_instances(0).max_val());
        ui->green_max_val_label->setText(QString::number(setting.green_instances(0).max_val()));

        //Add yellow colors
        ui->yellow_min_hue_slider->setValue(setting.yellow_instances(0).min_hue());
        ui->yellow_min_hue_label->setText(QString::number(setting.yellow_instances(0).min_hue()));

        ui->yellow_min_sat_slider->setValue(setting.yellow_instances(0).min_sat());
        ui->yellow_min_sat_label->setText(QString::number(setting.yellow_instances(0).min_sat()));

        ui->yellow_min_val_slider->setValue(setting.yellow_instances(0).min_val());
        ui->yellow_min_val_label->setText(QString::number(setting.yellow_instances(0).min_val()));

        ui->yellow_max_hue_slider->setValue(setting.yellow_instances(0).max_hue());
        ui->yellow_max_hue_label->setText(QString::number(setting.yellow_instances(0).max_hue()));

        ui->yellow_max_sat_slider->setValue(setting.yellow_instances(0).max_sat());
        ui->yellow_max_sat_label->setText(QString::number(setting.yellow_instances(0).max_sat()));

        ui->yellow_max_val_slider->setValue(setting.yellow_instances(0).max_val());
        ui->yellow_max_val_label->setText(QString::number(setting.yellow_instances(0).max_val()));

        //Add violet colors
        ui->violet_min_hue_slider->setValue(setting.violet_instances(0).min_hue());
        ui->violet_min_hue_label->setText(QString::number(setting.violet_instances(0).min_hue()));

        ui->violet_min_sat_slider->setValue(setting.violet_instances(0).min_sat());
        ui->violet_min_sat_label->setText(QString::number(setting.violet_instances(0).min_sat()));

        ui->violet_min_val_slider->setValue(setting.violet_instances(0).min_val());
        ui->violet_min_val_label->setText(QString::number(setting.violet_instances(0).min_val()));

        ui->violet_max_hue_slider->setValue(setting.violet_instances(0).max_hue());
        ui->violet_max_hue_label->setText(QString::number(setting.violet_instances(0).max_hue()));

        ui->violet_max_sat_slider->setValue(setting.violet_instances(0).max_sat());
        ui->violet_max_sat_label->setText(QString::number(setting.violet_instances(0).max_sat()));

        ui->violet_max_val_slider->setValue(setting.violet_instances(0).max_val());
        ui->violet_max_val_label->setText(QString::number(setting.violet_instances(0).max_val()));

        //Add cyan colors
        ui->cyan_min_hue_slider->setValue(setting.cyan_instances(0).min_hue());
        ui->cyan_min_hue_label->setText(QString::number(setting.cyan_instances(0).min_hue()));

        ui->cyan_min_sat_slider->setValue(setting.cyan_instances(0).min_sat());
        ui->cyan_min_sat_label->setText(QString::number(setting.cyan_instances(0).min_sat()));

        ui->cyan_min_val_slider->setValue(setting.cyan_instances(0).min_val());
        ui->cyan_min_val_label->setText(QString::number(setting.cyan_instances(0).min_val()));

        ui->cyan_max_hue_slider->setValue(setting.cyan_instances(0).max_hue());
        ui->cyan_max_hue_label->setText(QString::number(setting.cyan_instances(0).max_hue()));

        ui->cyan_max_sat_slider->setValue(setting.cyan_instances(0).max_sat());
        ui->cyan_max_sat_label->setText(QString::number(setting.cyan_instances(0).max_sat()));

        ui->cyan_max_val_slider->setValue(setting.cyan_instances(0).max_val());
        ui->cyan_max_val_label->setText(QString::number(setting.cyan_instances(0).max_val()));


        //Add black colors
        ui->black_min_hue_slider->setValue(setting.black_instances(0).min_hue());
        ui->black_min_hue_label->setText(QString::number(setting.black_instances(0).min_hue()));

        ui->black_min_sat_slider->setValue(setting.black_instances(0).min_sat());
        ui->black_min_sat_label->setText(QString::number(setting.black_instances(0).min_sat()));

        ui->black_min_val_slider->setValue(setting.black_instances(0).min_val());
        ui->black_min_val_label->setText(QString::number(setting.black_instances(0).min_val()));

        ui->black_max_hue_slider->setValue(setting.black_instances(0).max_hue());
        ui->black_max_hue_label->setText(QString::number(setting.black_instances(0).max_hue()));

        ui->black_max_sat_slider->setValue(setting.black_instances(0).max_sat());
        ui->black_max_sat_label->setText(QString::number(setting.black_instances(0).max_sat()));

        ui->black_max_val_slider->setValue(setting.black_instances(0).max_val());
        ui->black_max_val_label->setText(QString::number(setting.black_instances(0).max_val()));

        addHSVSettings();
    }
}

void MainWindow::addHSVSettings()
{
    //    Vec3f min,max;

    //    min.val[0] = ui->red_min_hue_slider->value();
    //    min.val[1] = ui->red_min_sat_slider->value();
    //    min.val[2] = ui->red_min_val_slider->value();
    //    max.val[0] = ui->red_max_hue_slider->value();
    //    max.val[1] = ui->red_max_sat_slider->value();
    //    max.val[2] = ui->red_max_val_slider->value();
    //    redProc->setRanges(min,max);

    //    min.val[0] = ui->blue_min_hue_slider->value();
    //    min.val[1] = ui->blue_min_sat_slider->value();
    //    min.val[2] = ui->blue_min_val_slider->value();
    //    max.val[0] = ui->blue_max_hue_slider->value();
    //    max.val[1] = ui->blue_max_sat_slider->value();
    //    max.val[2] = ui->blue_max_val_slider->value();
    //    blueProc->setRanges(min,max);

    //    min.val[0] = ui->green_min_hue_slider->value();
    //    min.val[1] = ui->green_min_sat_slider->value();
    //    min.val[2] = ui->green_min_val_slider->value();
    //    max.val[0] = ui->green_max_hue_slider->value();
    //    max.val[1] = ui->green_max_sat_slider->value();
    //    max.val[2] = ui->green_max_val_slider->value();
    //    greenProc->setRanges(min,max);

    //    min.val[0] = ui->yellow_min_hue_slider->value();
    //    min.val[1] = ui->yellow_min_sat_slider->value();
    //    min.val[2] = ui->yellow_min_val_slider->value();
    //    max.val[0] = ui->yellow_max_hue_slider->value();
    //    max.val[1] = ui->yellow_max_sat_slider->value();
    //    max.val[2] = ui->yellow_max_val_slider->value();
    //    yellowProc->setRanges(min,max);

    //    min.val[0] = ui->violet_min_hue_slider->value();
    //    min.val[1] = ui->violet_min_sat_slider->value();
    //    min.val[2] = ui->violet_min_val_slider->value();
    //    max.val[0] = ui->violet_max_hue_slider->value();
    //    max.val[1] = ui->violet_max_sat_slider->value();
    //    max.val[2] = ui->violet_max_val_slider->value();
    //    violetProc->setRanges(min,max);

    //    min.val[0] = ui->cyan_min_hue_slider->value();
    //    min.val[1] = ui->cyan_min_sat_slider->value();
    //    min.val[2] = ui->cyan_min_val_slider->value();
    //    max.val[0] = ui->cyan_max_hue_slider->value();
    //    max.val[1] = ui->cyan_max_sat_slider->value();
    //    max.val[2] = ui->cyan_max_val_slider->value();
    //    cyanProc->setRanges(min,max);

    //    min.val[0] = ui->black_min_hue_slider->value();
    //    min.val[1] = ui->black_min_sat_slider->value();
    //    min.val[2] = ui->black_min_val_slider->value();
    //    max.val[0] = ui->black_max_hue_slider->value();
    //    max.val[1] = ui->black_max_sat_slider->value();
    //    max.val[2] = ui->black_max_val_slider->value();
    //    blackProc->setRanges(min,max);
}

Mat MainWindow::returnFilterImage(Mat input, QString color)
{
    Mat Ranged;
    if(color == "red")
    {
        inRange(input
                ,Scalar(ui->red_min_hue_slider->value(),ui->red_min_sat_slider->value(),ui->red_min_val_slider->value())
                ,Scalar(ui->red_max_hue_slider->value(),ui->red_max_sat_slider->value(),ui->red_max_val_slider->value())
                ,Ranged);
    }
    else if(color == "blue")
    {
        inRange(input
                ,Scalar(ui->blue_min_hue_slider->value(),ui->blue_min_sat_slider->value(),ui->blue_min_val_slider->value())
                ,Scalar(ui->blue_max_hue_slider->value(),ui->blue_max_sat_slider->value(),ui->blue_max_val_slider->value())
                ,Ranged);
    }
    else if(color == "green")
    {
        inRange(input
                ,Scalar(ui->green_min_hue_slider->value(),ui->green_min_sat_slider->value(),ui->green_min_val_slider->value())
                ,Scalar(ui->green_max_hue_slider->value(),ui->green_max_sat_slider->value(),ui->green_max_val_slider->value())
                ,Ranged);
    }
    else if(color == "yellow")
    {
        inRange(input
                ,Scalar(ui->yellow_min_hue_slider->value(),ui->yellow_min_sat_slider->value(),ui->yellow_min_val_slider->value())
                ,Scalar(ui->yellow_max_hue_slider->value(),ui->yellow_max_sat_slider->value(),ui->yellow_max_val_slider->value())
                ,Ranged);
    }
    else if(color == "violet")
    {
        inRange(input
                ,Scalar(ui->violet_min_hue_slider->value(),ui->violet_min_sat_slider->value(),ui->violet_min_val_slider->value())
                ,Scalar(ui->violet_max_hue_slider->value(),ui->violet_max_sat_slider->value(),ui->violet_max_val_slider->value())
                ,Ranged);
    }
    else if(color == "cyan")
    {
        inRange(input
                ,Scalar(ui->cyan_min_hue_slider->value(),ui->cyan_min_sat_slider->value(),ui->cyan_min_val_slider->value())
                ,Scalar(ui->cyan_max_hue_slider->value(),ui->cyan_max_sat_slider->value(),ui->cyan_max_val_slider->value())
                ,Ranged);
    }
    else if(color == "black")
    {
        inRange(input
                ,Scalar(ui->black_min_hue_slider->value(),ui->black_min_sat_slider->value(),ui->black_min_val_slider->value())
                ,Scalar(ui->black_max_hue_slider->value(),ui->black_max_sat_slider->value(),ui->black_max_val_slider->value())
                ,Ranged);
    }

    return Ranged;
}

void MainWindow::disableVideoSetting()
{
    ui->videoAdd_label->setDisabled(true);
    ui->videoAdd_lineEdit->setDisabled(true);
}

void MainWindow::addRedImage(Mat out)
{
    redSem->tryAcquire(1,30);
    filterColor[0] = out;
    redSem->release();
}

void MainWindow::addBlueImage(Mat out)
{
    blueSem->tryAcquire(1,30);
    filterColor[1] = out;
    blueSem->release();
}

void MainWindow::addGreenImage(Mat out)
{
    greenSem->tryAcquire(1,30);
    filterColor[2] = out;
    greenSem->release();
}

void MainWindow::addYellowImage(Mat out)
{
    yellowSem->tryAcquire(1,30);
    filterColor[3] = out;
    yellowSem->release(1);
}

void MainWindow::addCyanImage(Mat out)
{
    cyanSem->tryAcquire(1,30);
    filterColor[5] = out;
    cyanSem->release();
}

void MainWindow::addVioletImage(Mat out)
{
    violetSem->tryAcquire(1,30);
    filterColor[4] = out;
    violetSem->release(1);
}

void MainWindow::addBlackImage(Mat out)
{
    blackSem->tryAcquire(1,30);
    filterColor[6] = out;
    blackSem->release();
}

void MainWindow::callImageProcessingFunctions(Mat input_mat)
{
    //undisort image
    disconnect(cam_timer,SIGNAL(timeout()),this,SLOT(cam_timeout()));
    //disconnect(recSocket,SIGNAL(readyRead()),this,SLOT(receiveUDPPacket()));
    disconnect(this,SIGNAL(imageReady(Mat)),this,SLOT(callImageProcessingFunctions(Mat)));
    //cam_timer->stop();

    Mat inputFrame;

    if(ui->undisort_checkBox->isChecked())
    {
        if( (ui->cam_comboBox->currentText() == "USB0") || (ui->cam_comboBox->currentText() == "USB1") )
            imageProcessor->undistortUSBCam(input_mat).copyTo(inputFrame);
        else if( (ui->cam_comboBox->currentText() == "0") || (ui->cam_comboBox->currentText() == "1") ||  (ui->cam_comboBox->currentText() == "Network"))
            imageProcessor->undistortFIREWIRECam(input_mat).copyTo(inputFrame);
    }
    else
    {
        input_mat.copyTo(inputFrame);
    }

    inputFrame.copyTo(filterColor[7]);

    //croped image for better performance
    Mat CropFrame;

    if(ui->crop_checkBox->isChecked())
    {
        imSize.width = ui->sX_lineEdit->text().toInt()-ui->fX_lineEdit->text().toInt();
        imSize.height = ui->sY_lineEdit->text().toInt()-ui->fY_lineEdit->text().toInt();
        cropedRect.width = ui->sX_lineEdit->text().toInt()-ui->fX_lineEdit->text().toInt();
        cropedRect.height = ui->sY_lineEdit->text().toInt()-ui->fY_lineEdit->text().toInt();
        cropedRect.x = ui->fX_lineEdit->text().toInt();
        cropedRect.y = ui->fY_lineEdit->text().toInt();

        Mat crop(inputFrame,cropedRect);
        crop.copyTo(CropFrame);

//        Point2f srcTri[3];
//        Point2f dstTri[3];
//        srcTri[0] = Point2f( 344,393 );
//        srcTri[1] = Point2f( 333, 92);
//        srcTri[2] = Point2f( 67, 340);

//        dstTri[0] = Point2f( 2730, 480 );
//        dstTri[1] = Point2f( 1220, 372 );
//        dstTri[2] = Point2f( 2530, -1034 );
//        Mat warp_mat( 2, 3, CV_32FC1 );
//        /// Get the Affine Transform
//        warp_mat = getAffineTransform( srcTri, dstTri );

//        Mat rotated = Mat::zeros(CropFrame.rows,CropFrame.cols,CropFrame.type());
//        warpAffine( CropFrame, rotated, warp_mat, rotated.size() );
//        imshow("ro",rotated);
    }
    else
    {
        imSize.width = inputFrame.rows;
        imSize.height = inputFrame.cols;

        cropedRect.width = inputFrame.rows;
        cropedRect.height = inputFrame.cols;
        cropedRect.x = 0;
        cropedRect.y = 0;
        inputFrame.copyTo(CropFrame);
    }

    Mat HSV;
    cvtColor(CropFrame,HSV,COLOR_RGB2HSV);

    if(ui->use_red_checkBox->isChecked())
    {
        filterColor[0] = returnFilterImage(HSV,"red");
        redProc->setImage(filterColor[0]);
    }
    else
    {
        RecievedData[0] = true;
    }

    if(ui->use_blue_checkBox->isChecked())
    {
        filterColor[1] = returnFilterImage(HSV,"blue");
        blueProc->setImage(filterColor[1]);
    }
    else
    {
        RecievedData[1] = true;
    }

    if(ui->use_green_checkBox->isChecked())
    {
        filterColor[2] = returnFilterImage(HSV,"green");
        greenProc->setImage(filterColor[2]);
    }
    else
    {
        RecievedData[2] = true;
    }

    if(ui->use_yellow_checkBox->isChecked())
    {
        filterColor[3] = returnFilterImage(HSV,"yellow");
        yellowProc->setImage(filterColor[3]);
    }
    else
    {
        RecievedData[3] = true;
    }

    if(ui->use_violet_checkBox->isChecked())
    {
        filterColor[4] = returnFilterImage(HSV,"violet");
        violetProc->setImage(filterColor[4]);
    }
    else
    {
        RecievedData[4] = true;
    }

    if(ui->use_cyan_checkBox->isChecked())
    {
        filterColor[5] = returnFilterImage(HSV,"cyan");
        cyanProc->setImage(filterColor[5]);
    }
    else
    {
        RecievedData[5] = true;
    }

    if(ui->use_black_checkBox->isChecked())
    {
        filterColor[6] = returnFilterImage(HSV,"black");
        blackProc->setImage(filterColor[6]);
    }
    else
    {
        RecievedData[6] = true;
    }

    checkTimer->start(15);
}

void MainWindow::on_camSet_checkBox_stateChanged()
{
    if(ui->camSet_checkBox->isChecked())
    {
        enableCameraSetting();
    }
    else
    {
        disableCameraSetting();
    }
}

void MainWindow::on_xml_checkBox_stateChanged()
{
    if(ui->xml_checkBox->isChecked())
    {
        enableXML();
    }
    else
    {
        disableXML();
    }
}

void MainWindow::on_xml_button_clicked()
{
    QString fileAddress = QFileDialog::getOpenFileName(this,tr("Select Your XML File"), "/home", tr("XML Files (*.xml)"));
    ui->xmlAdd_lineEdit->setText(fileAddress);
    QFile inputXMLFile(fileAddress);
    QXmlStreamReader cameraXMLSetting(&inputXMLFile);

    while (!cameraXMLSetting.atEnd() && !cameraXMLSetting.hasError())
    {
        qDebug()<<"reading xml file";
        cameraXMLSetting.readNext();
        if (cameraXMLSetting.isStartElement())
        {

        }
        else if (cameraXMLSetting.hasError())
        {
            qDebug() << "XML error: " << cameraXMLSetting.errorString() << endl;
        }
        else if (cameraXMLSetting.atEnd())
        {
            qDebug() << "Reached end, done" << endl;
        }
    }
}

void MainWindow::on_cont_checkBox_stateChanged()
{
    updateOutputOptions();
}

void MainWindow::on_geom_checkBox_stateChanged()
{
    updateOutputOptions();
}

void MainWindow::on_bound_checkBox_stateChanged()
{
    updateOutputOptions();
}

void MainWindow::on_rotate_checkBox_stateChanged()
{
    updateOutputOptions();
}

void MainWindow::on_medianBlur_checkBox_stateChanged()
{
    if(ui->medianBlur_checkBox->isChecked())
    {
        enableMedianBlur();
    }
    else
    {
        disableMedianBlur();
    }
    emit filterSettingChanged();
}

void MainWindow::on_adaptiveThreshold_checkBox_stateChanged()
{
    if(ui->adaptiveThreshold_checkBox->isChecked())
    {
        enableAdaptiveThresholdSetting();
    }
    else
    {
        disableAdaptiveThresholdSetting();
    }
    emit filterSettingChanged();
}

void MainWindow::on_thresh_checkBox_stateChanged()
{
    if(ui->thresh_checkBox->isChecked())
    {
        enableThresholdSetting();
    }
    else
    {
        disableThresholdSetting();
    }
    emit filterSettingChanged();
}

void MainWindow::on_dilate_checkBox_stateChanged()
{
    if(ui->dilate_checkBox->isChecked())
    {
        enableDilateSetting();
    }
    else
    {
        disableDilateSetting();
    }
    emit filterSettingChanged();
}

void MainWindow::on_canny_checkBox_stateChanged()
{
    if(ui->canny_checkBox->isChecked())
    {
        enableCannySetting();
    }
    else
    {
        disableCannySetting();
    }
    emit filterSettingChanged();
}

void MainWindow::on_blockSize_slider_sliderMoved(int position)
{
    ui->blockSizeOut_label->setText(QString::number(position));
    emit filterSettingChanged();
}

void MainWindow::on_C_slider_sliderMoved(int position)
{
    ui->cOut_label->setText(QString::number(position));
    emit filterSettingChanged();
}

void MainWindow::on_thresh_slider_sliderMoved(int position)
{
    ui->threshOut_label->setText(QString::number(position));
    emit filterSettingChanged();
}

void MainWindow::on_firstThresh_slider_sliderMoved(int position)
{
    ui->firstThreshOut_label->setText(QString::number(position));
    emit filterSettingChanged();
}

void MainWindow::on_secondThresh_slider_sliderMoved(int position)
{
    ui->secondThreshOut_label->setText(QString::number(position));
    emit filterSettingChanged();
}

void MainWindow::on_blue_slider_sliderMoved(int position)
{
    ui->blueOut_label->setText(QString::number(position));
    emit cameraSettingChanged();
}

void MainWindow::on_red_slider_sliderMoved(int position)
{
    ui->redOut_label->setText(QString::number(position));
    emit cameraSettingChanged();
}

void MainWindow::on_exposure_slider_sliderMoved(int position)
{
    ui->expoOut_label->setText(QString::number(position));
    emit cameraSettingChanged();
}

void MainWindow::on_brightness_slider_sliderMoved(int position)
{
    ui->brightnessOut_label->setText(QString::number(position));
    emit cameraSettingChanged();
}

void MainWindow::updateCameraSetting()
{
    setCameraSetting();
}

void MainWindow::setCameraSetting()
{
    if(cameraIsOpened)
    {
        cap.set(CAP_PROP_FPS, ui->fps_comboBox->currentText().toInt());

        if( (ui->cam_comboBox->currentText() == "0") || (ui->cam_comboBox->currentText() == "1") )
        {
            camSetting->set_fps(ui->fps_comboBox->currentText().toInt());

            cap.set(CAP_PROP_WHITE_BALANCE_BLUE_U,ui->blue_slider->value());
            camSetting->set_WHITE_BALANCE_BLUE_U(ui->blue_slider->value());

            cap.set(CAP_PROP_WHITE_BALANCE_RED_V,ui->red_slider->value());
            camSetting->set_WHITE_BALANCE_RED_V(ui->red_slider->value());

            cap.set(CAP_PROP_BRIGHTNESS,ui->brightness_slider->value());
            camSetting->set_BRIGHTNESS(ui->brightness_slider->value());

            cap.set(CAP_PROP_EXPOSURE,ui->exposure_slider->value());
            camSetting->set_EXPOSURE(ui->exposure_slider->value());

            cap.set(CAP_PROP_SHARPNESS,ui->sharpness_slider->value());
            camSetting->set_SHARPNESS(ui->sharpness_slider->value());

            cap.set(CAP_PROP_GAIN,ui->gain_slider->value());
            camSetting->set_GAIN(ui->gain_slider->value());

            cap.set(CAP_PROP_HUE,ui->hue_slider->value());
            camSetting->set_HUE(ui->hue_slider->value());

            cap.set(CAP_PROP_SATURATION,ui->saturation_slider->value());
            camSetting->set_SATURATION(ui->saturation_slider->value());

            cap.set(CAP_PROP_CONTRAST,ui->contrast_slider->value());
            camSetting->set_CONTRAST(ui->contrast_slider->value());
        }
    }
}

void MainWindow::on_sharpness_slider_sliderMoved(int position)
{
    ui->sharpnessOut_label->setText(QString::number(position));
    emit cameraSettingChanged();
}

void MainWindow::on_gain_slider_sliderMoved(int position)
{
    ui->gainOut_label->setText(QString::number(position));
    emit cameraSettingChanged();
}

void MainWindow::on_mouse_button_clicked()
{
    mouseButtonClicked=!mouseButtonClicked;
    setMouseTracking(mouseButtonClicked);
    QString temp=(mouseButtonClicked)?"Turn Off\nMouse\nIntrrupt":"Turn On\nMouse\nIntrrupt";
    ui->mouse_button->setText(temp);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonRelease)
    {
        if(lineDrawing || mouseButtonClicked)
        {
            return QMainWindow::eventFilter(obj, event);
        }
        else
        {
            qDebug()<<"Select a mode!";
        }
    }
    else
    {
        // pass the event on to the parent class
        return QMainWindow::eventFilter(obj, event);
    }
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if(isValidPlaceForSelect(event->x(),event->y()))
    {
        origin = event->pos();
        rubberBand->setGeometry(QRect(origin, QSize()));
        rubberBand->show();
        firstPointSelectedIsValid = true;
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if(isValidPlaceForSelect(event->x(),event->y()))
    {
        rubberBand->setGeometry(QRect(origin, event->pos()).normalized());
    }
    else
    {
        rubberBand->hide();
        firstPointSelectedIsValid = false;
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if(isValidPlaceForSelect(event->x(),event->y()))
    {
        if(firstPointSelectedIsValid)
        {
            if(mouseButtonClicked)
            {
                ui->fX_lineEdit->setText(QString::number(origin.x()-ui->outputLabel->x()));
                ui->fY_lineEdit->setText(QString::number(origin.y()-ui->outputLabel->y()));

                ui->sX_lineEdit->setText(QString::number(event->x()-ui->outputLabel->x()));
                ui->sY_lineEdit->setText(QString::number(event->y()-ui->outputLabel->y()));
            }

        }
    }

    rubberBand->hide();
    firstPointSelectedIsValid = false;
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    if(lineDrawing)
    {
        //        if(firstLinePointIsValid)
        //        {
        //            Point end;
        //            end.x = event->x();
        //            end.y = event->y();
        //            //firstLinePointIsValid = false;
        ////            outputPacket_line *line = imageProcessor->result.add_mission2_lines();
        ////            line->set_start_x(firstLinePoint.x);
        ////            line->set_start_y(firstLinePoint.y);
        ////            line->set_end_x(end.x);
        ////            line->set_end_y(end.y);
        ////            firstLinePoint.x = event->x();
        ////            firstLinePoint.y = event->y();
        //        }
        //        else
        //        {
        //            firstLinePoint.x = event->x();
        //            firstLinePoint.y = event->y();
        //            firstLinePointIsValid = true;
        //        }

        Point end;
        end.x = event->x();
        end.y = event->y();
        lineBorders.push_back(end);
    }
}

void MainWindow::on_drawCrop_checkBox_stateChanged()
{
    updateOutputOptions();
}

void MainWindow::on_firstM_rButton_toggled(bool checked)
{
    if(checked)
    {
        enableFirstMission();
    }
    else
    {
        disableFirstMission();
    }
}

void MainWindow::on_secondM_rButton_toggled(bool checked)
{
    if(checked)
    {
        enableSecondMission();
    }
    else
    {
        disableSecondMission();
    }
}

void MainWindow::on_go_button_clicked()
{
    imageProcessor->result.Clear();

    if(ui->firstM_rButton->isChecked())
    {
        setInitializeMessage(1);
    }
    else if(ui->secondM_rButton->isChecked())
    {
        setInitializeMessage(2);
    }
    else if(ui->thirsM_rButton->isChecked())
    {
        setInitializeMessage(3);
    }
    else
    {
        qDebug()<<"Error : Select a Mission!";
    }

    //sendDataPacket();
    emit dataReadyForSend();
    permissionForSending = true;
    send_timer->start(15);
}

void MainWindow::on_thirsM_rButton_toggled(bool checked)
{
    if(checked)
    {
        ui->role_groupBox->setEnabled(true);
        ui->regions_groupBox->setEnabled(true);
    }
    else
    {
        ui->role_groupBox->setDisabled(true);
        ui->regions_groupBox->setDisabled(true);
    }
}

void MainWindow::sendDataPacket()
{
    access2StallMode->acquire(1);
    if(!stallMode)
    {
        qDebug()<<"mission:"<<imageProcessor->result.mission();
        qDebug()<<"number:"<<imageProcessor->result.numberofshape();
        qDebug()<<"type:"<<imageProcessor->result.type();
        for(int i=0;i<imageProcessor->result.shapes_size();i++)
        {
            qDebug()<<"shape "<<i<<" seen at:"<<imageProcessor->result.shapes(i).position_x()<<","<<imageProcessor->result.shapes(i).position_y();
            qDebug()<<"color:"<<QString::fromStdString(imageProcessor->result.shapes(i).color());
            qDebug()<<"type:"<<QString::fromStdString(imageProcessor->result.shapes(i).type());
        }
    }
    else
    {
        imageProcessor->result.set_mission(0);
    }
    access2StallMode->release(1);

    string data;
    imageProcessor->result.SerializeToString(&data);
    QString s = QString::fromStdString(data);
    QByteArray q_data;
    q_data.append(s);
    sendingSocket->sendData(q_data);
}

void MainWindow::on_save_set_button_clicked()
{
    disconnect(cam_timer,SIGNAL(timeout()),this,SLOT(cam_timeout()));
    //disconnect(recSocket,SIGNAL(readyRead()),this,SLOT(receiveUDPPacket()));
    disconnect(this,SIGNAL(imageReady(Mat)),this,SLOT(callImageProcessingFunctions(Mat)));

    SystemSettings setting;

    setting.set_input_edit_camera_setting(ui->camSet_checkBox->isChecked());
    setting.set_input_white_balance_blue_u(ui->blue_slider->value());
    setting.set_input_white_balance_red_v(ui->red_slider->value());
    setting.set_input_exposure(ui->exposure_slider->value());
    setting.set_input_brightness(ui->brightness_slider->value());
    setting.set_input_sharpness(ui->sharpness_slider->value());
    setting.set_input_gain(ui->gain_slider->value());
    setting.set_input_hue(ui->hue_slider->value());
    setting.set_input_sat(ui->saturation_slider->value());
    setting.set_input_contrast(ui->contrast_slider->value());
    setting.set_input_network_ip(ui->ip_lineEdit->text().toStdString());
    setting.set_input_network_port(ui->port_lineEdit->text().toStdString());

    setting.set_filters_crop_photo(ui->crop_checkBox->isChecked());
    setting.set_filters_crop_firstpoint_x(ui->fX_lineEdit->text().toStdString());
    setting.set_filters_crop_firstpoint_y(ui->fY_lineEdit->text().toStdString());
    setting.set_filters_crop_secondpoint_x(ui->sX_lineEdit->text().toStdString());
    setting.set_filters_crop_secondpoint_y(ui->sY_lineEdit->text().toStdString());

    setting.set_filters_median_blur(ui->medianBlur_checkBox->isChecked());
    setting.set_filters_median_blur_kernelsize(ui->kernelSize_lineEdit->text().toStdString());

    setting.set_filters_adaptive_threshold(ui->adaptiveThreshold_checkBox->isChecked());
    setting.set_filters_adaptive_threshold_blocksize(ui->blockSize_slider->value());
    setting.set_filters_adaptive_threshold_c(ui->C_slider->value());

    setting.set_filters_threshold(ui->thresh_checkBox->isChecked());
    setting.set_filters_threshold_value(ui->thresh_slider->value());

    setting.set_filters_dilate(ui->dilate_checkBox->isChecked());
    setting.set_filters_dilationsize(ui->dilateSize_lineEdit->text().toStdString());

    setting.set_filters_canny(ui->canny_checkBox->isChecked());
    setting.set_filters_canny_aperturesize(ui->apertureSize_lineEdit->text().toStdString());
    setting.set_filters_canny_first_threshold(ui->firstThresh_slider->value());
    setting.set_filters_canny_second_threshold(ui->secondThresh_slider->value());

    //Add Colors!
    //Red...
    SystemSettings_HSV *red_color = setting.add_red_instances();
    red_color->set_min_hue(ui->red_min_hue_slider->value());
    red_color->set_min_sat(ui->red_min_sat_slider->value());
    red_color->set_min_val(ui->red_min_val_slider->value());

    red_color->set_max_hue(ui->red_max_hue_slider->value());
    red_color->set_max_sat(ui->red_max_sat_slider->value());
    red_color->set_max_val(ui->red_max_val_slider->value());

    //Blue...
    SystemSettings_HSV *blue_color = setting.add_blue_instances();
    blue_color->set_min_hue(ui->blue_min_hue_slider->value());
    blue_color->set_min_sat(ui->blue_min_sat_slider->value());
    blue_color->set_min_val(ui->blue_min_val_slider->value());

    blue_color->set_max_hue(ui->blue_max_hue_slider->value());
    blue_color->set_max_sat(ui->blue_max_sat_slider->value());
    blue_color->set_max_val(ui->blue_max_val_slider->value());

    //Green...
    SystemSettings_HSV *green_color = setting.add_green_instances();
    green_color->set_min_hue(ui->green_min_hue_slider->value());
    green_color->set_min_sat(ui->green_min_sat_slider->value());
    green_color->set_min_val(ui->green_min_val_slider->value());

    green_color->set_max_hue(ui->green_max_hue_slider->value());
    green_color->set_max_sat(ui->green_max_sat_slider->value());
    green_color->set_max_val(ui->green_max_val_slider->value());

    //Yellow...
    SystemSettings_HSV *yellow_color = setting.add_yellow_instances();
    yellow_color->set_min_hue(ui->yellow_min_hue_slider->value());
    yellow_color->set_min_sat(ui->yellow_min_sat_slider->value());
    yellow_color->set_min_val(ui->yellow_min_val_slider->value());

    yellow_color->set_max_hue(ui->yellow_max_hue_slider->value());
    yellow_color->set_max_sat(ui->yellow_max_sat_slider->value());
    yellow_color->set_max_val(ui->yellow_max_val_slider->value());

    //Violet...
    SystemSettings_HSV *violet_color = setting.add_violet_instances();
    violet_color->set_min_hue(ui->violet_min_hue_slider->value());
    violet_color->set_min_sat(ui->violet_min_sat_slider->value());
    violet_color->set_min_val(ui->violet_min_val_slider->value());

    violet_color->set_max_hue(ui->violet_max_hue_slider->value());
    violet_color->set_max_sat(ui->violet_max_sat_slider->value());
    violet_color->set_max_val(ui->violet_max_val_slider->value());

    //Cyan...
    SystemSettings_HSV *cyan_color = setting.add_cyan_instances();
    cyan_color->set_min_hue(ui->cyan_min_hue_slider->value());
    cyan_color->set_min_sat(ui->cyan_min_sat_slider->value());
    cyan_color->set_min_val(ui->cyan_min_val_slider->value());

    cyan_color->set_max_hue(ui->cyan_max_hue_slider->value());
    cyan_color->set_max_sat(ui->cyan_max_sat_slider->value());
    cyan_color->set_max_val(ui->cyan_max_val_slider->value());

    //Black...
    SystemSettings_HSV *black_color = setting.add_black_instances();
    black_color->set_min_hue(ui->black_min_hue_slider->value());
    black_color->set_min_sat(ui->black_min_sat_slider->value());
    black_color->set_min_val(ui->black_min_val_slider->value());

    black_color->set_max_hue(ui->black_max_hue_slider->value());
    black_color->set_max_sat(ui->black_max_sat_slider->value());
    black_color->set_max_val(ui->black_max_val_slider->value());

    QString fileAddress = QFileDialog::getSaveFileName(this,tr("Select Directory..."), "/home", tr("Text File (*.txt)"));
    QFile file(fileAddress);

    if(file.open(QIODevice::WriteOnly | QIODevice::Text ))
    {
        qDebug() << "File Has Been Created" << endl;
        fstream output;
        output.open(fileAddress.toUtf8(), fstream::out | fstream::trunc | fstream::binary);;
        if (!setting.SerializeToOstream(&output)) {
            qDebug() << "Failed to write data." << endl;
            //file.close();
        }
    }
    else
    {
        qDebug() << "Failed to Create File" << endl;
    }

    connect(cam_timer,SIGNAL(timeout()),this,SLOT(cam_timeout()));
    //connect(recSocket,SIGNAL(readyRead()),this,SLOT(receiveUDPPacket()));
    connect(this,SIGNAL(imageReady(Mat)),this,SLOT(callImageProcessingFunctions(Mat)));
}

void MainWindow::on_open_set_button_clicked()
{
    disconnect(cam_timer,SIGNAL(timeout()),this,SLOT(cam_timeout()));
    //disconnect(recSocket,SIGNAL(readyRead()),this,SLOT(receiveUDPPacket()));
    disconnect(this,SIGNAL(imageReady(Mat)),this,SLOT(callImageProcessingFunctions(Mat)));

    QString fileAddress = QFileDialog::getOpenFileName(this,tr("Select Setting File"), "/home", tr("Text File (*.txt)"));

    openSetting(fileAddress);

    connect(cam_timer,SIGNAL(timeout()),this,SLOT(cam_timeout()));
    //connect(recSocket,SIGNAL(readyRead()),this,SLOT(receiveUDPPacket()));
    connect(this,SIGNAL(imageReady(Mat)),this,SLOT(callImageProcessingFunctions(Mat)));
}

void MainWindow::on_stall_button_clicked()
{
    access2StallMode->acquire(1);
    stallMode = !stallMode;
    QString temp=(stallMode)?"Resume":"Stall";
    ui->stall_button->setText(temp);
    access2StallMode->release(1);
}

void MainWindow::responseForFilterSettingsChanged()
{
    updateFilterSetting();
    imageProcessor->updateFilterSettings(filterSetting);
}

void MainWindow::on_kernelSize_lineEdit_textChanged()
{
    emit filterSettingChanged();
}

void MainWindow::on_dilateSize_lineEdit_textChanged()
{
    emit filterSettingChanged();
}

void MainWindow::on_apertureSize_lineEdit_textChanged()
{
    emit filterSettingChanged();
}

void MainWindow::on_crop_checkBox_stateChanged()
{
    if(ui->crop_checkBox->isChecked())
    {
        enableCropSetting();
    }
    else
    {
        disableCropSetting();
    }
    emit filterSettingChanged();
}

void MainWindow::on_fX_lineEdit_textChanged()
{
    emit filterSettingChanged();
}

void MainWindow::on_fY_lineEdit_textChanged()
{
    emit filterSettingChanged();
}

void MainWindow::on_sX_lineEdit_textChanged()
{
    emit filterSettingChanged();
}

void MainWindow::on_sY_lineEdit_textChanged()
{
    emit filterSettingChanged();
}

void MainWindow::on_undisort_checkBox_stateChanged()
{
    emit filterSettingChanged();
}

void MainWindow::on_hue_slider_sliderMoved(int position)
{
    ui->hueOut_label->setText(QString::number(position));
    emit cameraSettingChanged();
}

void MainWindow::on_saturation_slider_sliderMoved(int position)
{
    ui->saturationOut_label->setText(QString::number(position));
    emit cameraSettingChanged();
}

void MainWindow::on_contrast_slider_sliderMoved(int position)
{
    ui->contrastOut_label->setText(QString::number(position));
    emit cameraSettingChanged();
}

void MainWindow::on_red_min_hue_slider_sliderMoved(int position)
{
    ui->red_min_hue_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::resposibleForRedOutput()
{
    semaphoreForDataPlussing->acquire(1);
    red_shapes = redProc->detectedShapes;
    RecievedData[0] = true;
    semaphoreForDataPlussing->release(1);
}

void MainWindow::resposibleForBlueOutput()
{
    semaphoreForDataPlussing->acquire(1);
    blue_shapes = blueProc->detectedShapes;
    RecievedData[1] = true;
    semaphoreForDataPlussing->release(1);
}

void MainWindow::resposibleForGreenOutput()
{
    semaphoreForDataPlussing->acquire(1);
    green_shapes = greenProc->detectedShapes;
    RecievedData[2] = true;
    semaphoreForDataPlussing->release(1);
}

void MainWindow::resposibleForYellowOutput()
{
    semaphoreForDataPlussing->acquire(1);
    yellow_shapes = yellowProc->detectedShapes;
    RecievedData[3] = true;
    semaphoreForDataPlussing->release(1);
}

void MainWindow::resposibleForCyanOutput()
{
    semaphoreForDataPlussing->acquire(1);
    cyan_shapes = cyanProc->detectedShapes;
    RecievedData[5] = true;
    semaphoreForDataPlussing->release(1);
}

void MainWindow::resposibleForVioletOutput()
{
    semaphoreForDataPlussing->acquire(1);
    violet_shapes = violetProc->detectedShapes;
    RecievedData[4] = true;
    semaphoreForDataPlussing->release(1);
}

void MainWindow::resposibleForBlackOutput()
{
    semaphoreForDataPlussing->acquire(1);
    black_shapes = blackProc->detectedShapes;
    RecievedData[6] = true;
    semaphoreForDataPlussing->release(1);
}

void MainWindow::checkAllOfRecieved()
{
    semaphoreForDataPlussing->acquire(7);

    bool dataRecievedCompeletly = false;

    if(RecievedData[0] && RecievedData[1] && RecievedData[2] && RecievedData[3] && RecievedData[4] && RecievedData[5] && RecievedData[6])
    {
        for(int i=0;i<7;i++)
            RecievedData[i] = false;

        imageProcessor->result.clear_shapes();

        for(int i=0;i<red_shapes.size();i++)
        {
            outputPacket_shape *shape = imageProcessor->result.add_shapes();
            shape->set_position_x(red_shapes.at(i).position_x);
            shape->set_position_y(red_shapes.at(i).position_y);
            shape->set_radios(red_shapes.at(i).roundedRadios);
            shape->set_type(red_shapes.at(i).type);
            shape->set_color(red_shapes.at(i).color);
        }

        for(int i=0;i<blue_shapes.size();i++)
        {
            outputPacket_shape *shape = imageProcessor->result.add_shapes();
            shape->set_position_x(blue_shapes.at(i).position_x);
            shape->set_position_y(blue_shapes.at(i).position_y);
            shape->set_radios(blue_shapes.at(i).roundedRadios);
            shape->set_type(blue_shapes.at(i).type);
            shape->set_color(blue_shapes.at(i).color);
        }

        for(int i=0;i<green_shapes.size();i++)
        {
            outputPacket_shape *shape = imageProcessor->result.add_shapes();
            shape->set_position_x(green_shapes.at(i).position_x);
            shape->set_position_y(green_shapes.at(i).position_y);
            shape->set_radios(green_shapes.at(i).roundedRadios);
            shape->set_type(green_shapes.at(i).type);
            shape->set_color(green_shapes.at(i).color);
        }

        for(int i=0;i<yellow_shapes.size();i++)
        {
            outputPacket_shape *shape = imageProcessor->result.add_shapes();
            shape->set_position_x(yellow_shapes.at(i).position_x);
            shape->set_position_y(yellow_shapes.at(i).position_y);
            shape->set_radios(yellow_shapes.at(i).roundedRadios);
            shape->set_type(yellow_shapes.at(i).type);
            shape->set_color(yellow_shapes.at(i).color);
        }

        for(int i=0;i<cyan_shapes.size();i++)
        {
            outputPacket_shape *shape = imageProcessor->result.add_shapes();
            shape->set_position_x(cyan_shapes.at(i).position_x);
            shape->set_position_y(cyan_shapes.at(i).position_y);
            shape->set_radios(cyan_shapes.at(i).roundedRadios);
            shape->set_type(cyan_shapes.at(i).type);
            shape->set_color(cyan_shapes.at(i).color);
        }

        for(int i=0;i<violet_shapes.size();i++)
        {
            outputPacket_shape *shape = imageProcessor->result.add_shapes();
            shape->set_position_x(violet_shapes.at(i).position_x);
            shape->set_position_y(violet_shapes.at(i).position_y);
            shape->set_radios(violet_shapes.at(i).roundedRadios);
            shape->set_type(violet_shapes.at(i).type);
            shape->set_color(violet_shapes.at(i).color);
        }

        for(int i=0;i<black_shapes.size();i++)
        {
            outputPacket_shape *shape = imageProcessor->result.add_shapes();
            shape->set_position_x(black_shapes.at(i).position_x);
            shape->set_position_y(black_shapes.at(i).position_y);
            shape->set_radios(black_shapes.at(i).roundedRadios);
            shape->set_type(black_shapes.at(i).type);
            shape->set_color(black_shapes.at(i).color);
        }
        if(permissionForSending)
        {
            imageProcessor->result.set_mission(mission);
            imageProcessor->result.set_type(1);
            //sendDataPacket();
            emit dataReadyForSend();
        }
        dataRecievedCompeletly = true;
    }
    semaphoreForDataPlussing->release(7);

    if(dataRecievedCompeletly)
    {
        dataRecievedCompeletly = false;
        checkTimer->stop();
        if(!imageRecievedFromNetwork)
        {
            connect(cam_timer,SIGNAL(timeout()),this,SLOT(cam_timeout()));
        }
        //        recSocket->flush();
        //        connect(recSocket,SIGNAL(readyRead()),this,SLOT(receiveUDPPacket()));
        connect(this,SIGNAL(imageReady(Mat)),this,SLOT(callImageProcessingFunctions(Mat)));
    }

    Mat outputFrame;

    if(ui->out_comboBox->currentText() == "Red")
    {
        filterColor[0].copyTo(outputFrame);
    }
    else if(ui->out_comboBox->currentText() == "Blue")
    {
        filterColor[1].copyTo(outputFrame);
    }
    else if(ui->out_comboBox->currentText() == "Green")
    {
        filterColor[2].copyTo(outputFrame);
    }
    else if(ui->out_comboBox->currentText() == "Yellow")
    {
        filterColor[3].copyTo(outputFrame);
    }

    else if(ui->out_comboBox->currentText() == "Violet")
    {
        filterColor[4].copyTo(outputFrame);
    }

    else if(ui->out_comboBox->currentText() == "Cyan")
    {
        filterColor[5].copyTo(outputFrame);
    }

    else if(ui->out_comboBox->currentText() == "Black")
    {
//        filterColor[6].copyTo(outputFrame);
        //---------------
        Mat crop;
        Rect cropR;
        filterColor[6].copyTo(crop);
        cropR.x = 20;
        cropR.y = 20;
        cropR.width = crop.cols - 50;
        cropR.height = crop.rows - 50;

        Mat crop2(crop,cropR);
        crop2.copyTo(outputFrame);
        //---------------------------


        medianBlur(outputFrame,outputFrame,3);
       Mat structure=getStructuringElement(MORPH_RECT,Size(5,5));
//        erode(outputFrame,outputFrame,structure);
        dilate(outputFrame,outputFrame,structure);
//        medianBlur(outputFrame,outputFrame,7);
//        Mat structure2=getStructuringElement(MORPH_RECT,Size(3,3));
//        erode(outputFrame,outputFrame,structure2);
        //dilate(outputFrame,outputFrame,structure);
        vector<vector<Point> > contours;
        vector<Vec4i> hierarchy;
        findContours(outputFrame.clone(), contours, hierarchy, RETR_LIST, CHAIN_APPROX_SIMPLE);
        RNG rng(12345);
        for(int i=0;i<contours.size();i++)
        {
           drawContours(outputFrame,contours, i, Scalar(125,255,0), 2, 8, hierarchy, 0);
        }
    }

    else if(ui->out_comboBox->currentText() == "Final")
    {
        filterColor[7].copyTo(outputFrame);
        if(ui->drawCrop_checkBox->isChecked())
        {
            if(mission == 2)
            {
                for(int i=0;i<lineBorders.size()-1;i++)
                {
                    line(outputFrame,Point(lineBorders.at(i).x,lineBorders.at(i).y)
                         ,Point(lineBorders.at(i+1).x,lineBorders.at(i+1).y)
                         ,Scalar(0,0,0));
                }
            }
            if(mission == 1)
            {
                float tl_X = ((Orgin_X - ui->region1_tlX_lineEdit->text().toFloat()-100)/Width)*imSize.width + ui->fX_lineEdit->text().toInt();
                float tl_Y = ((-Orgin_Y + ui->region1_tlY_lineEdit->text().toFloat()+100)/Height)*imSize.height + ui->fY_lineEdit->text().toInt();

                float br_X = ((Orgin_X - ui->region1_brX_lineEdit->text().toFloat()-100)/Width)*imSize.width+ ui->fX_lineEdit->text().toInt();
                float br_Y = ((-Orgin_Y + ui->region1_brY_lineEdit->text().toFloat()+100)/Height)*imSize.height+ ui->fY_lineEdit->text().toInt();


                rectangle(outputFrame,Point(tl_X,tl_Y), Point(br_X,br_Y), Scalar(0,0,0));

                tl_X = ((Orgin_X - ui->region2_tlX_lineEdit->text().toFloat()+100)/Width)*imSize.width + ui->fX_lineEdit->text().toInt();
                tl_Y = ((-Orgin_Y + ui->region2_tlY_lineEdit->text().toFloat()-100)/Height)*imSize.height + ui->fY_lineEdit->text().toInt();

                br_X = ((Orgin_X - ui->region2_brX_lineEdit->text().toFloat()+100)/Width)*imSize.width + ui->fX_lineEdit->text().toInt();
                br_Y = ((-Orgin_Y + ui->region2_brY_lineEdit->text().toFloat()-100)/Height)*imSize.height + ui->fY_lineEdit->text().toInt();

                rectangle(outputFrame,Point(tl_X,tl_Y), Point(br_X,br_Y), Scalar(0,0,0));
            }
        }
    }

    if(!outputFrame.empty())
    {
        cv::resize(outputFrame,outputFrame,Size(640,480),0,0,INTER_CUBIC);
        QImage imgIn;

        if(ui->out_comboBox->currentText() == "Final")
            cvtColor(outputFrame, outputFrame, COLOR_BGR2RGB);
        else
            cvtColor(outputFrame, outputFrame, COLOR_GRAY2RGB);

        imgIn= QImage((uchar*) outputFrame.data, outputFrame.cols, outputFrame.rows, outputFrame.step, QImage::Format_RGB888);
        ui->outputLabel->setPixmap(QPixmap::fromImage(imgIn));
    }
}

void MainWindow::on_red_max_hue_slider_sliderMoved(int position)
{
    ui->red_max_hue_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_red_min_sat_slider_sliderMoved(int position)
{
    ui->red_min_sat_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_red_max_sat_slider_sliderMoved(int position)
{
    ui->red_max_sat_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_red_min_val_slider_sliderMoved(int position)
{
    ui->red_min_val_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_red_max_val_slider_sliderMoved(int position)
{
    ui->red_max_val_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_blue_min_hue_slider_sliderMoved(int position)
{
    ui->blue_min_hue_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_blue_max_hue_slider_sliderMoved(int position)
{
    ui->blue_max_hue_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_blue_min_sat_slider_sliderMoved(int position)
{
    ui->blue_min_sat_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_blue_min_val_slider_sliderMoved(int position)
{
    ui->blue_min_val_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_blue_max_val_slider_sliderMoved(int position)
{
    ui->blue_max_val_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_blue_max_sat_slider_sliderMoved(int position)
{
    ui->blue_max_sat_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_green_min_hue_slider_sliderMoved(int position)
{
    ui->green_min_hue_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_green_max_hue_slider_sliderMoved(int position)
{
    ui->green_max_hue_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_green_min_sat_slider_sliderMoved(int position)
{
    ui->green_min_sat_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_green_max_sat_slider_sliderMoved(int position)
{
    ui->green_max_sat_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_green_min_val_slider_sliderMoved(int position)
{
    ui->green_min_val_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_green_max_val_slider_sliderMoved(int position)
{
    ui->green_max_val_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_yellow_min_hue_slider_sliderMoved(int position)
{
    ui->yellow_min_hue_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_yellow_max_hue_slider_sliderMoved(int position)
{
    ui->yellow_max_hue_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_yellow_min_sat_slider_sliderMoved(int position)
{
    ui->yellow_min_sat_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_yellow_max_sat_slider_sliderMoved(int position)
{
    ui->yellow_max_sat_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_yellow_min_val_slider_sliderMoved(int position)
{
    ui->yellow_min_val_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_yellow_max_val_slider_sliderMoved(int position)
{
    ui->yellow_max_val_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_violet_min_hue_slider_sliderMoved(int position)
{
    ui->violet_min_hue_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_violet_max_hue_slider_sliderMoved(int position)
{
    ui->violet_max_hue_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_violet_min_sat_slider_sliderMoved(int position)
{
    ui->violet_min_sat_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_violet_max_sat_slider_sliderMoved(int position)
{
    ui->violet_max_sat_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_violet_min_val_slider_sliderMoved(int position)
{
    ui->violet_min_val_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_violet_max_val_slider_sliderMoved(int position)
{
    ui->violet_max_val_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_cyan_min_hue_slider_sliderMoved(int position)
{
    ui->cyan_min_hue_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_cyan_max_hue_slider_sliderMoved(int position)
{
    ui->cyan_max_hue_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_cyan_min_sat_slider_sliderMoved(int position)
{
    ui->cyan_min_sat_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_cyan_max_sat_slider_sliderMoved(int position)
{
    ui->cyan_max_sat_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_cyan_min_val_slider_sliderMoved(int position)
{
    ui->cyan_min_val_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_cyan_max_val_slider_sliderMoved(int position)
{
    ui->cyan_max_val_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_black_min_hue_slider_sliderMoved(int position)
{
    ui->black_min_hue_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_black_max_hue_slider_sliderMoved(int position)
{
    ui->black_max_hue_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_black_min_sat_slider_sliderMoved(int position)
{
    ui->black_min_sat_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_black_max_sat_slider_sliderMoved(int position)
{
    ui->black_max_sat_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_black_min_val_slider_sliderMoved(int position)
{
    ui->black_min_val_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_black_max_val_slider_sliderMoved(int position)
{
    ui->black_max_val_label->setText(QString::number(position));
    addHSVSettings();
}

void MainWindow::on_import_button_clicked()
{
    ColorSettings setting;

    QString fileAddress = QFileDialog::getOpenFileName(this,tr("Select Color File"), "/home", tr("Text File (*.txt)"));

    fstream input;
    input.open(fileAddress.toUtf8(), ios::in | ios::binary);
    if (!input)
    {
        qDebug() << fileAddress << ": File not found.  Creating a new file." << endl;

    }
    else if (!setting.ParseFromIstream(&input))
    {
        qDebug() << "Failed";
    }
    else
    {
        if(setting.red_instances_size() >=0)
        {
            float red_min_hue = 100000;
            float red_min_sat = 100000;
            float red_min_val = 100000;

            float red_max_hue = -10000;
            float red_max_sat = -10000;
            float red_max_val = -10000;

            for(int i=0;i<setting.red_instances_size();i++)
            {
                if(setting.red_instances(i).min_hue() < red_min_hue)
                {
                    red_min_hue = setting.red_instances(i).min_hue();
                }

                if(setting.red_instances(i).min_sat() < red_min_sat)
                {
                    red_min_sat = setting.red_instances(i).min_sat();
                }

                if(setting.red_instances(i).min_val() < red_min_val)
                {
                    red_min_val = setting.red_instances(i).min_val();
                }

                if(setting.red_instances(i).max_hue() > red_max_hue)
                {
                    red_max_hue = setting.red_instances(i).max_hue();
                }

                if(setting.red_instances(i).max_sat() > red_max_sat)
                {
                    red_max_sat = setting.red_instances(i).max_sat();
                }

                if(setting.red_instances(i).max_val() > red_max_val)
                {
                    red_max_val = setting.red_instances(i).max_val();
                }
            }

            //Add red Colors
            ui->red_min_hue_slider->setValue(red_min_hue);
            ui->red_min_hue_label->setText(QString::number(red_min_hue));

            ui->red_min_sat_slider->setValue(red_min_sat);
            ui->red_min_sat_label->setText(QString::number(red_min_sat));

            ui->red_min_val_slider->setValue(red_min_val);
            ui->red_min_val_label->setText(QString::number(red_min_val));

            ui->red_max_hue_slider->setValue(red_max_hue);
            ui->red_max_hue_label->setText(QString::number(red_max_hue));

            ui->red_max_sat_slider->setValue(red_max_sat);
            ui->red_max_sat_label->setText(QString::number(red_max_sat));

            ui->red_max_val_slider->setValue(red_max_val);
            ui->red_max_val_label->setText(QString::number(red_max_val));
        }

        if(setting.blue_instances_size() >=0)
        {
            float blue_min_hue = 100000;
            float blue_min_sat = 100000;
            float blue_min_val = 100000;

            float blue_max_hue = -10000;
            float blue_max_sat = -10000;
            float blue_max_val = -10000;

            for(int i=0;i<setting.blue_instances_size();i++)
            {
                if(setting.blue_instances(i).min_hue() < blue_min_hue)
                {
                    blue_min_hue = setting.blue_instances(i).min_hue();
                }

                if(setting.blue_instances(i).min_sat() < blue_min_sat)
                {
                    blue_min_sat = setting.blue_instances(i).min_sat();
                }

                if(setting.blue_instances(i).min_val() < blue_min_val)
                {
                    blue_min_val = setting.blue_instances(i).min_val();
                }

                if(setting.blue_instances(i).max_hue() > blue_max_hue)
                {
                    blue_max_hue = setting.blue_instances(i).max_hue();
                }

                if(setting.blue_instances(i).max_sat() > blue_max_sat)
                {
                    blue_max_sat = setting.blue_instances(i).max_sat();
                }

                if(setting.blue_instances(i).max_val() > blue_max_val)
                {
                    blue_max_val = setting.blue_instances(i).max_val();
                }
            }

            //Add blue Colors
            ui->blue_min_hue_slider->setValue(blue_min_hue);
            ui->blue_min_hue_label->setText(QString::number(blue_min_hue));

            ui->blue_min_sat_slider->setValue(blue_min_sat);
            ui->blue_min_sat_label->setText(QString::number(blue_min_sat));

            ui->blue_min_val_slider->setValue(blue_min_val);
            ui->blue_min_val_label->setText(QString::number(blue_min_val));

            ui->blue_max_hue_slider->setValue(blue_max_hue);
            ui->blue_max_hue_label->setText(QString::number(blue_max_hue));

            ui->blue_max_sat_slider->setValue(blue_max_sat);
            ui->blue_max_sat_label->setText(QString::number(blue_max_sat));

            ui->blue_max_val_slider->setValue(blue_max_val);
            ui->blue_max_val_label->setText(QString::number(blue_max_val));
        }

        if(setting.green_instances_size() >=0)
        {
            float green_min_hue = 100000;
            float green_min_sat = 100000;
            float green_min_val = 100000;

            float green_max_hue = -10000;
            float green_max_sat = -10000;
            float green_max_val = -10000;

            for(int i=0;i<setting.green_instances_size();i++)
            {
                if(setting.green_instances(i).min_hue() < green_min_hue)
                {
                    green_min_hue = setting.green_instances(i).min_hue();
                }

                if(setting.green_instances(i).min_sat() < green_min_sat)
                {
                    green_min_sat = setting.green_instances(i).min_sat();
                }

                if(setting.green_instances(i).min_val() < green_min_val)
                {
                    green_min_val = setting.green_instances(i).min_val();
                }

                if(setting.green_instances(i).max_hue() > green_max_hue)
                {
                    green_max_hue = setting.green_instances(i).max_hue();
                }

                if(setting.green_instances(i).max_sat() > green_max_sat)
                {
                    green_max_sat = setting.green_instances(i).max_sat();
                }

                if(setting.green_instances(i).max_val() > green_max_val)
                {
                    green_max_val = setting.green_instances(i).max_val();
                }
            }

            //Add green Colors
            ui->green_min_hue_slider->setValue(green_min_hue);
            ui->green_min_hue_label->setText(QString::number(green_min_hue));

            ui->green_min_sat_slider->setValue(green_min_sat);
            ui->green_min_sat_label->setText(QString::number(green_min_sat));

            ui->green_min_val_slider->setValue(green_min_val);
            ui->green_min_val_label->setText(QString::number(green_min_val));

            ui->green_max_hue_slider->setValue(green_max_hue);
            ui->green_max_hue_label->setText(QString::number(green_max_hue));

            ui->green_max_sat_slider->setValue(green_max_sat);
            ui->green_max_sat_label->setText(QString::number(green_max_sat));

            ui->green_max_val_slider->setValue(green_max_val);
            ui->green_max_val_label->setText(QString::number(green_max_val));
        }

        if(setting.yellow_instances_size() >=0)
        {
            float yellow_min_hue = 100000;
            float yellow_min_sat = 100000;
            float yellow_min_val = 100000;

            float yellow_max_hue = -10000;
            float yellow_max_sat = -10000;
            float yellow_max_val = -10000;

            for(int i=0;i<setting.yellow_instances_size();i++)
            {
                if(setting.yellow_instances(i).min_hue() < yellow_min_hue)
                {
                    yellow_min_hue = setting.yellow_instances(i).min_hue();
                }

                if(setting.yellow_instances(i).min_sat() < yellow_min_sat)
                {
                    yellow_min_sat = setting.yellow_instances(i).min_sat();
                }

                if(setting.yellow_instances(i).min_val() < yellow_min_val)
                {
                    yellow_min_val = setting.yellow_instances(i).min_val();
                }

                if(setting.yellow_instances(i).max_hue() > yellow_max_hue)
                {
                    yellow_max_hue = setting.yellow_instances(i).max_hue();
                }

                if(setting.yellow_instances(i).max_sat() > yellow_max_sat)
                {
                    yellow_max_sat = setting.yellow_instances(i).max_sat();
                }

                if(setting.yellow_instances(i).max_val() > yellow_max_val)
                {
                    yellow_max_val = setting.yellow_instances(i).max_val();
                }
            }

            //Add yellow Colors
            ui->yellow_min_hue_slider->setValue(yellow_min_hue);
            ui->yellow_min_hue_label->setText(QString::number(yellow_min_hue));

            ui->yellow_min_sat_slider->setValue(yellow_min_sat);
            ui->yellow_min_sat_label->setText(QString::number(yellow_min_sat));

            ui->yellow_min_val_slider->setValue(yellow_min_val);
            ui->yellow_min_val_label->setText(QString::number(yellow_min_val));

            ui->yellow_max_hue_slider->setValue(yellow_max_hue);
            ui->yellow_max_hue_label->setText(QString::number(yellow_max_hue));

            ui->yellow_max_sat_slider->setValue(yellow_max_sat);
            ui->yellow_max_sat_label->setText(QString::number(yellow_max_sat));

            ui->yellow_max_val_slider->setValue(yellow_max_val);
            ui->yellow_max_val_label->setText(QString::number(yellow_max_val));
        }

        if(setting.violet_instances_size() >=0)
        {
            float violet_min_hue = 100000;
            float violet_min_sat = 100000;
            float violet_min_val = 100000;

            float violet_max_hue = -10000;
            float violet_max_sat = -10000;
            float violet_max_val = -10000;

            for(int i=0;i<setting.violet_instances_size();i++)
            {
                if(setting.violet_instances(i).min_hue() < violet_min_hue)
                {
                    violet_min_hue = setting.violet_instances(i).min_hue();
                }

                if(setting.violet_instances(i).min_sat() < violet_min_sat)
                {
                    violet_min_sat = setting.violet_instances(i).min_sat();
                }

                if(setting.violet_instances(i).min_val() < violet_min_val)
                {
                    violet_min_val = setting.violet_instances(i).min_val();
                }

                if(setting.violet_instances(i).max_hue() > violet_max_hue)
                {
                    violet_max_hue = setting.violet_instances(i).max_hue();
                }

                if(setting.violet_instances(i).max_sat() > violet_max_sat)
                {
                    violet_max_sat = setting.violet_instances(i).max_sat();
                }

                if(setting.violet_instances(i).max_val() > violet_max_val)
                {
                    violet_max_val = setting.violet_instances(i).max_val();
                }
            }

            //Add violet Colors
            ui->violet_min_hue_slider->setValue(violet_min_hue);
            ui->violet_min_hue_label->setText(QString::number(violet_min_hue));

            ui->violet_min_sat_slider->setValue(violet_min_sat);
            ui->violet_min_sat_label->setText(QString::number(violet_min_sat));

            ui->violet_min_val_slider->setValue(violet_min_val);
            ui->violet_min_val_label->setText(QString::number(violet_min_val));

            ui->violet_max_hue_slider->setValue(violet_max_hue);
            ui->violet_max_hue_label->setText(QString::number(violet_max_hue));

            ui->violet_max_sat_slider->setValue(violet_max_sat);
            ui->violet_max_sat_label->setText(QString::number(violet_max_sat));

            ui->violet_max_val_slider->setValue(violet_max_val);
            ui->violet_max_val_label->setText(QString::number(violet_max_val));
        }

        if(setting.cyan_instances_size() >=0)
        {
            float cyan_min_hue = 100000;
            float cyan_min_sat = 100000;
            float cyan_min_val = 100000;

            float cyan_max_hue = -10000;
            float cyan_max_sat = -10000;
            float cyan_max_val = -10000;

            for(int i=0;i<setting.cyan_instances_size();i++)
            {
                if(setting.cyan_instances(i).min_hue() < cyan_min_hue)
                {
                    cyan_min_hue = setting.cyan_instances(i).min_hue();
                }

                if(setting.cyan_instances(i).min_sat() < cyan_min_sat)
                {
                    cyan_min_sat = setting.cyan_instances(i).min_sat();
                }

                if(setting.cyan_instances(i).min_val() < cyan_min_val)
                {
                    cyan_min_val = setting.cyan_instances(i).min_val();
                }

                if(setting.cyan_instances(i).max_hue() > cyan_max_hue)
                {
                    cyan_max_hue = setting.cyan_instances(i).max_hue();
                }

                if(setting.cyan_instances(i).max_sat() > cyan_max_sat)
                {
                    cyan_max_sat = setting.cyan_instances(i).max_sat();
                }

                if(setting.cyan_instances(i).max_val() > cyan_max_val)
                {
                    cyan_max_val = setting.cyan_instances(i).max_val();
                }
            }

            //Add cyan Colors
            ui->cyan_min_hue_slider->setValue(cyan_min_hue);
            ui->cyan_min_hue_label->setText(QString::number(cyan_min_hue));

            ui->cyan_min_sat_slider->setValue(cyan_min_sat);
            ui->cyan_min_sat_label->setText(QString::number(cyan_min_sat));

            ui->cyan_min_val_slider->setValue(cyan_min_val);
            ui->cyan_min_val_label->setText(QString::number(cyan_min_val));

            ui->cyan_max_hue_slider->setValue(cyan_max_hue);
            ui->cyan_max_hue_label->setText(QString::number(cyan_max_hue));

            ui->cyan_max_sat_slider->setValue(cyan_max_sat);
            ui->cyan_max_sat_label->setText(QString::number(cyan_max_sat));

            ui->cyan_max_val_slider->setValue(cyan_max_val);
            ui->cyan_max_val_label->setText(QString::number(cyan_max_val));
        }

        if(setting.black_instances_size() >=0)
        {
            float black_min_hue = 100000;
            float black_min_sat = 100000;
            float black_min_val = 100000;

            float black_max_hue = -10000;
            float black_max_sat = -10000;
            float black_max_val = -10000;

            for(int i=0;i<setting.black_instances_size();i++)
            {
                if(setting.black_instances(i).min_hue() < black_min_hue)
                {
                    black_min_hue = setting.black_instances(i).min_hue();
                }

                if(setting.black_instances(i).min_sat() < black_min_sat)
                {
                    black_min_sat = setting.black_instances(i).min_sat();
                }

                if(setting.black_instances(i).min_val() < black_min_val)
                {
                    black_min_val = setting.black_instances(i).min_val();
                }

                if(setting.black_instances(i).max_hue() > black_max_hue)
                {
                    black_max_hue = setting.black_instances(i).max_hue();
                }

                if(setting.black_instances(i).max_sat() > black_max_sat)
                {
                    black_max_sat = setting.black_instances(i).max_sat();
                }

                if(setting.black_instances(i).max_val() > black_max_val)
                {
                    black_max_val = setting.black_instances(i).max_val();
                }
            }

            //Add black Colors
            ui->black_min_hue_slider->setValue(black_min_hue);
            ui->black_min_hue_label->setText(QString::number(black_min_hue));

            ui->black_min_sat_slider->setValue(black_min_sat);
            ui->black_min_sat_label->setText(QString::number(black_min_sat));

            ui->black_min_val_slider->setValue(black_min_val);
            ui->black_min_val_label->setText(QString::number(black_min_val));

            ui->black_max_hue_slider->setValue(black_max_hue);
            ui->black_max_hue_label->setText(QString::number(black_max_hue));

            ui->black_max_sat_slider->setValue(black_max_sat);
            ui->black_max_sat_label->setText(QString::number(black_max_sat));

            ui->black_max_val_slider->setValue(black_max_val);
            ui->black_max_val_label->setText(QString::number(black_max_val));
        }

        addHSVSettings();
    }
}

Mat MainWindow::QImage2Mat(QImage src)
{
    QImage myImage=src;
    Mat tmp(src.height(),src.width(),CV_8UC4,src.scanLine(0)); //RGB32 has 8 bits of R, 8 bits of G, 8 bits of B and 8 bits of Alpha. It's essentially RGBA.
    Mat MatOut(src.height(),src.width(),CV_8UC3); //RGB32 has 8 bits of R, 8 bits of G, 8 bits of B and 8 bits of Alpha. It's essentially RGBA.

    cvtColor(tmp, MatOut, COLOR_RGBA2RGB);
    return MatOut;
}

QImage MainWindow::Mat2QImage(const Mat &src)
{
    Mat temp; // make the same cv::Mat
    cvtColor(src, temp, COLOR_BGR2RGB); // cvtColor Makes a copt, that what i need
    QImage dest((uchar*) temp.data, temp.cols, temp.rows, temp.step, QImage::Format_RGB888);
    QImage dest2(dest);
    dest2.detach(); // enforce deep copy
    return dest2;
}

void MainWindow::receiveUDPPacket()
{
    udp_datagram.resize(recSocket->pendingDatagramSize());
    recSocket->readDatagram(udp_datagram.data(), udp_datagram.size());

    // show received image......................
    udpImage1.loadFromData(udp_datagram);
    QImage2Mat(udpImage1).copyTo(udpFrame);
    udp_datagram.clear();
    emit imageReady(udpFrame);
}

void MainWindow::on_lines_button_clicked()
{
    lineDrawing = !lineDrawing;
    setMouseTracking(lineDrawing);
    QString temp=(lineDrawing)?"Turn Off\nMouse\nIntrrupt":"Draw\nLines";
    if(!lineDrawing)
    {
        firstLinePointIsValid = false;
    }
    ui->lines_button->setText(temp);
}

void MainWindow::on_clearLines_button_clicked()
{
    lineBorders.clear();
    firstLinePointIsValid = false;
}
