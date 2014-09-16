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

    connect(redProc,SIGNAL(afterFilter(Mat)),this,SLOT(addRedImage(Mat)));
    connect(blueProc,SIGNAL(afterFilter(Mat)),this,SLOT(addBlueImage(Mat)));
    connect(greenProc,SIGNAL(afterFilter(Mat)),this,SLOT(addGreenImage(Mat)));
    connect(yellowProc,SIGNAL(afterFilter(Mat)),this,SLOT(addYellowImage(Mat)));
    connect(violetProc,SIGNAL(afterFilter(Mat)),this,SLOT(addVioletImage(Mat)));
    connect(cyanProc,SIGNAL(afterFilter(Mat)),this,SLOT(addCyanImage(Mat)));
    connect(blackProc,SIGNAL(afterFilter(Mat)),this,SLOT(addBlackImage(Mat)));

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
    items<<"0"<<"1"<<"Network";
    ui->cam_comboBox->addItems(items);

    QStringList fps_items;
    fps_items<<"60"<<"15"<<"30";
    ui->fps_comboBox->addItems(fps_items);

    QStringList output_items;
    output_items<<"Red"<<"Blue"<<"Green"<<"Yellow"<<"Violet"<<"Cyan"<<"Black"<<"Final";
    ui->out_comboBox->addItems(output_items);
    ui->out_comboBox->setCurrentIndex(4);

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

    //openSetting("/home/kn2c/setting.txt");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_open_button_clicked()
{
    sendingSocket->configure(ui->ip_lineEdit->text(),ui->port_lineEdit->text().toInt());

    Mat frame;

    if(ui->cam_comboBox->currentText() == "Network")
    {
    }
    else
    {
        if(ui->cam_comboBox->currentText()=="0")
        {
            cameraIsOpened=cap.open(CAP_FIREWIRE+0);
        }
        else
        {
            cameraIsOpened=cap.open(CAP_FIREWIRE+1);
        }

        setCameraSetting();

        cap.read(frame);

        redThread->start();
        redProc->Start();

        blueThread->start();
        blueProc->Start();

        greenThread->start();
        greenProc->Start();

        yellowThread->start();
        yellowProc->Start();

        violetThread->start();
        violetProc->Start();

        cyanThread->start();
        cyanProc->Start();

        blackThread->start();
        blackProc->Start();

        checkTimer->start(15);
        cam_timer->start(1000*(1/ui->fps_comboBox->currentText().toInt()));
        connect(cam_timer,SIGNAL(timeout()),this,SLOT(cam_timeout()));
    }

    emit imageReady(frame);
}

void MainWindow::cam_timeout()
{
    Mat frame;
    cap.read(frame);
    emit imageReady(frame);
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
}

void MainWindow::disableSecondMission()
{
    ui->sMendX_lineEdit->setDisabled(true);
    ui->sMendY_lineEdit->setDisabled(true);
    ui->sMend_label->setDisabled(true);
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

        outputPacket_line *line1=imageProcessor->result.add_mission2_lines();
        line1->set_start_x(1300);
        line1->set_start_y(-1500);
        line1->set_end_x(1700);
        line1->set_end_y(0);

        outputPacket_line *line2=imageProcessor->result.add_mission2_lines();
        line2->set_start_x(1700);
        line2->set_start_y(0);
        line2->set_end_x(1300);
        line2->set_end_y(1500);
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
        imageProcessor->result.set_mission3_goal2_x(ui->goal2_Y_lineEdit->text().toFloat());

        break;
    }
    }
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

        //        //Add red Colors
        //        for(int i=0;i<setting.red_instances_size();i++)
        //        {
        //            Vec3f colorSample;
        //            colorSample.val[0] = setting.red_instances(i).hue();
        //            colorSample.val[1] = setting.red_instances(i).sat();
        //            colorSample.val[2] = setting.red_instances(i).val();
        //            imageProcessor->red_samples.push_back(colorSample);
        //        }
        //        ui->red_status_label->setText(QString::number(imageProcessor->red_samples.size())+" Color at list.");

        //        //Add blue colors
        //        for(int i=0;i<setting.blue_instances_size();i++)
        //        {
        //            Vec2f colorSample;
        //            colorSample.val[0] = setting.blue_instances(i).min_hue();
        //            colorSample.val[1] = setting.blue_instances(i).max_hue();
        //            //colorSample.val[2] = setting.blue_instances(i).val();
        //            imageProcessor->blue_samples.push_back(colorSample);
        //        }
        //        ui->blue_status_label->setText(QString::number(imageProcessor->blue_samples.size())+" Color at list.");

        //       //Add green colors
        //        for(int i=0;i<setting.green_instances_size();i++)
        //        {
        //            Vec2f colorSample;
        //            colorSample.val[0] = setting.green_instances(i).min_hue();
        //            colorSample.val[1] = setting.green_instances(i).max_hue();
        //            //colorSample.val[2] = setting.green_instances(i).val();
        //            imageProcessor->green_samples.push_back(colorSample);
        //        }
        //        ui->green_status_label->setText(QString::number(imageProcessor->green_samples.size())+" Color at list.");

        //        //Add yellow colors
        //        for(int i=0;i<setting.yellow_instances_size();i++)
        //        {
        //            Vec2f colorSample;
        //            colorSample.val[0] = setting.yellow_instances(i).min_hue();
        //            colorSample.val[1] = setting.yellow_instances(i).max_hue();
        //            //colorSample.val[2] = setting.yellow_instances(i).val();
        //            imageProcessor->yellow_samples.push_back(colorSample);
        //        }
        //        ui->yellow_status_label->setText(QString::number(imageProcessor->yellow_samples.size())+" Color at list.");

        //        //Add violet colors
        //        for(int i=0;i<setting.violet_instances_size();i++)
        //        {
        //            Vec2f colorSample;
        //            colorSample.val[0] = setting.violet_instances(i).min_hue();
        //            colorSample.val[1] = setting.violet_instances(i).max_hue();
        //            //colorSample.val[2] = setting.violet_instances(i).val();
        //            imageProcessor->violet_samples.push_back(colorSample);
        //        }
        //        ui->violet_status_label->setText(QString::number(imageProcessor->violet_samples.size())+" Color at list.");

        //        //Add cyan colors
        //        for(int i=0;i<setting.cyan_instances_size();i++)
        //        {
        //            Vec2f colorSample;
        //            colorSample.val[0] = setting.cyan_instances(i).min_hue();
        //            colorSample.val[1] = setting.cyan_instances(i).max_hue();
        //            //colorSample.val[2] = setting.cyan_instances(i).val();
        //            imageProcessor->cyan_samples.push_back(colorSample);
        //        }
        //        ui->cyan_status_label->setText(QString::number(imageProcessor->cyan_samples.size())+" Color at list.");

        //        //Add black colors
        //        for(int i=0;i<setting.black_instances_size();i++)
        //        {
        //            Vec2f colorSample;
        //            colorSample.val[0] = setting.black_instances(i).min_hue();
        //            colorSample.val[1] = setting.black_instances(i).max_hue();
        //            //colorSample.val[2] = setting.black_instances(i).val();
        //            imageProcessor->black_samples.push_back(colorSample);
        //        }
        //        ui->black_status_label->setText(QString::number(imageProcessor->black_samples.size())+" Color at list.");
    }
}

void MainWindow::addHSVSettings()
{
    Vec3f min,max;

    min.val[0] = ui->red_min_hue_slider->value();
    min.val[1] = ui->red_min_sat_slider->value();
    min.val[2] = ui->red_min_val_slider->value();
    max.val[0] = ui->red_max_hue_slider->value();
    max.val[1] = ui->red_max_sat_slider->value();
    max.val[2] = ui->red_max_val_slider->value();
    redProc->setRanges(min,max);

    min.val[0] = ui->blue_min_hue_slider->value();
    min.val[1] = ui->blue_min_sat_slider->value();
    min.val[2] = ui->blue_min_val_slider->value();
    max.val[0] = ui->blue_max_hue_slider->value();
    max.val[1] = ui->blue_max_sat_slider->value();
    max.val[2] = ui->blue_max_val_slider->value();
    blueProc->setRanges(min,max);

    min.val[0] = ui->green_min_hue_slider->value();
    min.val[1] = ui->green_min_sat_slider->value();
    min.val[2] = ui->green_min_val_slider->value();
    max.val[0] = ui->green_max_hue_slider->value();
    max.val[1] = ui->green_max_sat_slider->value();
    max.val[2] = ui->green_max_val_slider->value();
    greenProc->setRanges(min,max);

    min.val[0] = ui->yellow_min_hue_slider->value();
    min.val[1] = ui->yellow_min_sat_slider->value();
    min.val[2] = ui->yellow_min_val_slider->value();
    max.val[0] = ui->yellow_max_hue_slider->value();
    max.val[1] = ui->yellow_max_sat_slider->value();
    max.val[2] = ui->yellow_max_val_slider->value();
    yellowProc->setRanges(min,max);

    min.val[0] = ui->violet_min_hue_slider->value();
    min.val[1] = ui->violet_min_sat_slider->value();
    min.val[2] = ui->violet_min_val_slider->value();
    max.val[0] = ui->violet_max_hue_slider->value();
    max.val[1] = ui->violet_max_sat_slider->value();
    max.val[2] = ui->violet_max_val_slider->value();
    violetProc->setRanges(min,max);

    min.val[0] = ui->cyan_min_hue_slider->value();
    min.val[1] = ui->cyan_min_sat_slider->value();
    min.val[2] = ui->cyan_min_val_slider->value();
    max.val[0] = ui->cyan_max_hue_slider->value();
    max.val[1] = ui->cyan_max_sat_slider->value();
    max.val[2] = ui->cyan_max_val_slider->value();
    cyanProc->setRanges(min,max);

    min.val[0] = ui->black_min_hue_slider->value();
    min.val[1] = ui->black_min_sat_slider->value();
    min.val[2] = ui->black_min_val_slider->value();
    max.val[0] = ui->black_max_hue_slider->value();
    max.val[1] = ui->black_max_sat_slider->value();
    max.val[2] = ui->black_max_val_slider->value();
    blackProc->setRanges(min,max);
}

void MainWindow::addRedImage(Mat out)
{
    redSem->tryAcquire(1,30);
    filterColor[0] = out;
    imshow("red",filterColor[0]);
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
    Mat inputFrame;

    imageProcessor->undistortImage(input_mat).copyTo(inputFrame);

    //croped image for better performance
    Mat CropFrame;
    Rect cropedRect;
    if(ui->crop_checkBox->isChecked())
    {
        cropedRect.width = ui->sX_lineEdit->text().toInt()-ui->fX_lineEdit->text().toInt();
        cropedRect.height = ui->sY_lineEdit->text().toInt()-ui->fY_lineEdit->text().toInt();
        cropedRect.x = ui->fX_lineEdit->text().toInt();
        cropedRect.y = ui->fY_lineEdit->text().toInt();

        Mat crop(inputFrame,cropedRect);
        crop.copyTo(CropFrame);
    }
    else
    {
        cropedRect.width = inputFrame.rows;
        cropedRect.height = inputFrame.cols;
        cropedRect.x = 0;
        cropedRect.y = 0;
        inputFrame.copyTo(CropFrame);
    }

    Mat HSV;
    cvtColor(CropFrame,HSV,COLOR_RGB2HSV);

    redProc->setImage(HSV);
    blueProc->setImage(HSV);
    greenProc->setImage(HSV);
    yellowProc->setImage(HSV);
    cyanProc->setImage(HSV);
    violetProc->setImage(HSV);
    blackProc->setImage(HSV);
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
        if(colorMode || mouseButtonClicked)
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
        qDebug()<<"--------------";
        qDebug()<<"mission:"<<imageProcessor->result.mission();
        qDebug()<<"number:"<<imageProcessor->result.numberofshape();
        qDebug()<<"type:"<<imageProcessor->result.type();
        for(int i=0;i<imageProcessor->result.shapes_size();i++)
        {
            qDebug()<<"shape "<<i<<" seen at:"<<imageProcessor->result.shapes(i).position_x()<<","<<imageProcessor->result.shapes(i).position_y();
            qDebug()<<"color:"<<QString::fromStdString(imageProcessor->result.shapes(i).color());
        }
        qDebug()<<"--------------";
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

    SystemSettings setting;

    setting.set_input_edit_camera_setting(ui->camSet_checkBox->isChecked());
    setting.set_input_white_balance_blue_u(ui->blue_slider->value());
    setting.set_input_white_balance_red_v(ui->red_slider->value());
    setting.set_input_exposure(ui->exposure_slider->value());
    setting.set_input_brightness(ui->brightness_slider->value());
    setting.set_input_sharpness(ui->sharpness_slider->value());
    setting.set_input_gain(ui->gain_slider->value());
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

    //    //Add Colors!
    //    for(int i=0;i<imageProcessor->red_samples.size();i++)
    //    {
    //        SystemSettings_color *color = setting.add_red_instances();
    //        color->set_hue(imageProcessor->red_samples.at(i).val[0]);
    //        color->set_sat(imageProcessor->red_samples.at(i).val[1]);
    //        color->set_val(imageProcessor->red_samples.at(i).val[2]);
    //    }

    //    for(int i=0;i<imageProcessor->blue_samples.size();i++)
    //    {
    //        SystemSettings_color *color = setting.add_blue_instances();
    //        color->set_hue(imageProcessor->blue_samples.at(i).val[0]);
    //        color->set_sat(imageProcessor->blue_samples.at(i).val[1]);
    //        color->set_val(imageProcessor->blue_samples.at(i).val[2]);
    //    }

    //    for(int i=0;i<imageProcessor->green_samples.size();i++)
    //    {
    //        SystemSettings_color *color = setting.add_green_instances();
    //        color->set_hue(imageProcessor->green_samples.at(i).val[0]);
    //        color->set_sat(imageProcessor->green_samples.at(i).val[1]);
    //        color->set_val(imageProcessor->green_samples.at(i).val[2]);
    //    }

    //    for(int i=0;i<imageProcessor->yellow_samples.size();i++)
    //    {
    //        SystemSettings_color *color = setting.add_yellow_instances();
    //        color->set_hue(imageProcessor->yellow_samples.at(i).val[0]);
    //        color->set_sat(imageProcessor->yellow_samples.at(i).val[1]);
    //        color->set_val(imageProcessor->yellow_samples.at(i).val[2]);
    //    }

    //    for(int i=0;i<imageProcessor->violet_samples.size();i++)
    //    {
    //        SystemSettings_color *color = setting.add_violet_instances();
    //        color->set_hue(imageProcessor->violet_samples.at(i).val[0]);
    //        color->set_sat(imageProcessor->violet_samples.at(i).val[1]);
    //        color->set_val(imageProcessor->violet_samples.at(i).val[2]);
    //    }

    //    for(int i=0;i<imageProcessor->cyan_samples.size();i++)
    //    {
    //        SystemSettings_color *color = setting.add_cyan_instances();
    //        color->set_hue(imageProcessor->cyan_samples.at(i).val[0]);
    //        color->set_sat(imageProcessor->cyan_samples.at(i).val[1]);
    //        color->set_val(imageProcessor->cyan_samples.at(i).val[2]);
    //    }

    //    for(int i=0;i<imageProcessor->black_samples.size();i++)
    //    {
    //        SystemSettings_color *color = setting.add_black_instances();
    //        color->set_hue(imageProcessor->black_samples.at(i).val[0]);
    //        color->set_sat(imageProcessor->black_samples.at(i).val[1]);
    //        color->set_val(imageProcessor->black_samples.at(i).val[2]);
    //    }

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
}

void MainWindow::on_open_set_button_clicked()
{
    disconnect(cam_timer,SIGNAL(timeout()),this,SLOT(cam_timeout()));

    QString fileAddress = QFileDialog::getOpenFileName(this,tr("Select Setting File"), "/home", tr("Text File (*.txt)"));

    openSetting(fileAddress);

    connect(cam_timer,SIGNAL(timeout()),this,SLOT(cam_timeout()));
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
    RecievedData[4] = true;
    semaphoreForDataPlussing->release(1);
}

void MainWindow::resposibleForVioletOutput()
{
    semaphoreForDataPlussing->acquire(1);
    violet_shapes = violetProc->detectedShapes;
    RecievedData[5] = true;
    semaphoreForDataPlussing->release(1);
}

void MainWindow::resposibleForBlackOutput()
{
    RecievedData[6] = true;
}

void MainWindow::checkAllOfRecieved()
{
    semaphoreForDataPlussing->acquire(7);

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
    }
    semaphoreForDataPlussing->release(7);

   if(!filterColor[0].empty())
       imshow("redi",filterColor[0]);

   if(permissionForSending)
    {
        imageProcessor->result.set_mission(mission);
        imageProcessor->result.set_type(1);
        //sendDataPacket();
        emit dataReadyForSend();
    }

    Mat outputFrame;

    if(ui->out_comboBox->currentText() == "Red")
    {
        redSem->tryAcquire(1,30);
        filterColor[0].copyTo(outputFrame);
        redSem->release();
    }
    else if(ui->out_comboBox->currentText() == "Blue")
    {
        blueSem->tryAcquire(1,30);
        filterColor[1].copyTo(outputFrame);
        blueSem->release();
    }
    else if(ui->out_comboBox->currentText() == "Green")
    {
        greenSem->tryAcquire(1,30);
        filterColor[2].copyTo(outputFrame);
        greenSem->release();
    }
    else if(ui->out_comboBox->currentText() == "Yellow")
    {
        yellowSem->tryAcquire(1,30);
        filterColor[3].copyTo(outputFrame);
        yellowSem->release();
    }

    else if(ui->out_comboBox->currentText() == "Violet")
    {
        violetSem->tryAcquire(1,30);
        filterColor[4].copyTo(outputFrame);
        violetSem->release();
    }

    else if(ui->out_comboBox->currentText() == "Cyan")
    {
        cyanSem->tryAcquire(1,30);
        filterColor[5].copyTo(outputFrame);
        cyanSem->release();
    }

    else if(ui->out_comboBox->currentText() == "Black")
    {
        blackSem->tryAcquire(1,30);
        filterColor[6].copyTo(outputFrame);
        blackSem->release();
    }

    else if(ui->out_comboBox->currentText() == "Final")
    {
        //filterColor[6].copyTo(outputFrame);
    }

    if(!outputFrame.empty())
    {
        imshow("out",outputFrame);
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