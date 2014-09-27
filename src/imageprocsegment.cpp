#include "imageprocsegment.h"

ImageProcSegment::ImageProcSegment(QObject *parent) :
    QObject(parent)
{
    newDataRecieved = false;

    //semaphoreForRanges = new QSemaphore(1);
    timer = new QTimer();
    connect(timer,SIGNAL(timeout()),this,SLOT(timer_inteval()));
}

void ImageProcSegment::shapeDetection(Mat input)
{
    imSize.width = input.cols;
    imSize.height = input.rows;

    detectedShapes.clear();
    recycledShapes.clear();

    // Find contours
    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;
    findContours(input.clone(), contours, hierarchy, RETR_LIST, CHAIN_APPROX_SIMPLE);

    vector<Point> approx;

    for (int i = 0; i < contours.size(); i++)
    {
        // Approximate contour with accuracy proportional
        // to the contour perimeter
        approxPolyDP(Mat(contours[i]), approx,arcLength(Mat(contours[i]), true)*0.0382, true);

        // Skip small or non-convex objects
        if (fabs(contourArea(contours[i])) < 50 || !isContourConvex(approx))
        {
            if(fabs(contourArea(contours[i])) > 200 && !isContourConvex(approx))
            {
                recycledShapes.push_back(contours[i]);
            }
            continue;
        }

        if(!checkAspectRatio(contours[i]))
            continue;

        RotatedRect rotatetBoundRect=minAreaRect(Mat(contours[i]));
        if(!checkAspectRatioForRotatedRect(rotatetBoundRect))
        {
            continue;
        }

        //        if(contourArea(contours[i]) >(0.005)*imSize.width*imSize.height)
        //        {
        //           prepareDataForOutput(contours[i],"Chasbideh");
        //           continue;
        //        }
        prepareDataForOutput(contours[i],"Ball");
    }
}

void ImageProcSegment::RobotDetection(Mat input)
{
    imSize.width = input.cols;
    imSize.height = input.rows;

    detectedShapes.clear();

    // Find contours
    //    vector<vector<Point> > contours1;
    //    vector<Vec4i> hierarchy1;
    //    findContours(input.clone(), contours1, hierarchy1, RETR_LIST, CHAIN_APPROX_SIMPLE);

    //    Mat fil = input.clone();

    //    for (int i = 0; i < contours1.size(); i++)
    //    {
    //        Rect boundedRect=boundingRect( Mat(contours1[i]) );
    //        rectangle( fil, boundedRect.tl(), boundedRect.br(), Scalar(255,255,255), 2, 8, 0 );
    //    }

    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;
    Point tl;
    tl.x = -cropR.tl().x;
    tl.y = -cropR.tl().y;
    findContours(input.clone(), contours, hierarchy, RETR_LIST, CHAIN_APPROX_SIMPLE,tl);

    //qDebug()<<"--------------";
    for (int i = 0; i < contours.size(); i++)
    {
        //qDebug()<<"contoursArea:"<<fabs(contourArea(contours[i]));
        //Skip small or non-convex objects
        //qDebug()<<"contour:"<<contourArea(contours[i]);

        if (fabs(contourArea(contours[i])) < 300 || fabs(contourArea(contours[i]))  > 1000 )
            continue;

        if(!checkAspectRatio(contours[i]))
            continue;

        RotatedRect rotatetBoundRect=minAreaRect(Mat(contours[i]));
        if(!checkAspectRatioForRotatedRect(rotatetBoundRect))
        {
            continue;
        }

        //        Point2f center;
        //        float radius;
        //        minEnclosingCircle( (Mat)contours[i], center, radius );

        //        if(radius < 5)
        //            continue;

        //prepareDataForOutput(contours[i],"Robot");
        robotList.push_back(contours[i]);
    }

//    for(int i=0;i<robotList.size();i++)
//    {
//        Point2f center;
//        float radius;
//        minEnclosingCircle( (Mat)contours[i], center, radius );

//        if(sqrt(pow( (center.x-221),2) + pow( (center.y-176),2) < 50))
//        {
//            robotList.erase(robotList.begin()+i);
//        }

//        if(sqrt(pow( (center.x-221),2) + pow( (center.y-244),2) < 50))
//        {
//            robotList.erase(robotList.begin()+i);
//        }
//    }

    for(int i=0;i<robotList.size();i++)
    {
        for(int j=i+1;j<robotList.size();j++)
        {
            Point2f centerI,centerJ;
            float radiusI,radiusJ;
            minEnclosingCircle( (Mat)robotList[i], centerI, radiusI );
            minEnclosingCircle( (Mat)robotList[j], centerJ, radiusJ );

            qDebug()<<"i:"<<i<<"     "<<centerI.x<<","<<centerI.y<<"     "<<radiusI;

            if( sqrt( pow((centerI.x-centerJ.x),2)+ pow((centerI.y-centerJ.y),2)) < min(radiusI,radiusJ) )
            {
                if( min(radiusI,radiusJ) == radiusI)
                {
                    robotList.erase(robotList.begin()+i);
                }
                else
                {
                    robotList.erase(robotList.begin()+j);
                }
            }
        }
    }


//    for(int i=0;i<robotList.size();i++)
//    {
//        if(fabs(contourArea(robotList[i]))  > 1000)
//            robotList.erase(robotList.begin()+i);
//    }

    for(int i=0;i<robotList.size();i++)
    {
        for(int j=i+1;j<robotList.size();j++)
        {
            if( fabs(contourArea(robotList[i])) < fabs(contourArea(robotList[j])) )
            {
                vector<Point> temp;
                temp = robotList.at(i);
                robotList.at(i) = robotList.at(j);
                robotList.at(j) = temp;
            }
        }
    }

    for(int i=0;i<robotList.size();i++)
    {
        qDebug()<<"size"<<i<<" "<<contourArea(robotList[i]);
        prepareDataForOutput(robotList[i],"Robot");
    }

    //prepareDataForOutput(robotList[1],"Robot");
}

void ImageProcSegment::setImage(Mat input)
{
    newDataRecieved = true;
    this->input = input;
}

void ImageProcSegment::addShape(float x, float y, double radius, string type, string color)
{
    Shape shape;
    shape.set(x,y,radius,color,type);
    detectedShapes.push_back(shape);
}

//void ImageProcSegment::setRanges(Vec3f minR, Vec3f maxR)
//{
//    semaphoreForRanges->tryAcquire(1,30);
//    minColorRange = minR;
//    maxColorRange = maxR;
//    semaphoreForRanges->release(1);
//}

void ImageProcSegment::Start()
{
    timer->start(15);
}

void ImageProcSegment::setColor(QString input)
{
    this->color = input;
}

bool ImageProcSegment::checkAspectRatio(vector<Point> contours_poly)
{
    Rect boundedRect=boundingRect( Mat(contours_poly) );
    double aspect_ratio = float(boundedRect.width)/boundedRect.height;
    bool out;

    if(  (aspect_ratio>ASPECT_RATIO_TRESH) || (aspect_ratio<(1/ASPECT_RATIO_TRESH)))
        out=false;
    else
        out=true;

    return out;
}

bool ImageProcSegment::checkAspectRatioForRotatedRect(RotatedRect input)
{
    double aspect_ratio = float(input.size.width)/input.size.height;
    bool out=true;

    if(  (aspect_ratio>ASPECT_RATIO_TRESH) || (aspect_ratio<(1/ASPECT_RATIO_TRESH)))
        out=false;

    return out;
}

void ImageProcSegment::prepareDataForOutput(std::vector<Point> &contour, QString type)
{
    Point2f center;
    float radius;
    minEnclosingCircle( (Mat)contour, center, radius );

    //--------mohsen khare---------------

    radius *= max((Width/imSize.width),(Height/imSize.height));
    //radius *= 1.5;
    float Xman,Yman;
    //    Mat src(1,1,CV_32FC2);
    //    Mat warp_dst(1,1,CV_32FC2);
    if(type == "TRI")
    {
        Xman = Orgin_X + (gravCenter.x/imSize.width)*Width;
        Yman = Orgin_Y - (gravCenter.y/imSize.height)*Height;
    }
    else
    {
        Xman = Orgin_X + (center.x/imSize.width)*Width;
        Yman = Orgin_Y - (center.y/imSize.height)*Height;
    }
    //---------------------------------

    //-------Tales------
    //    Xman *= 60/57;
    //    Yman *= 60/57;

    addShape(Xman,Yman,radius,type.toStdString(),color.toStdString());
}

double ImageProcSegment::angle(Point pt1, Point pt2, Point pt0)
{
    double dx1 = pt1.x - pt0.x;
    double dy1 = pt1.y - pt0.y;
    double dx2 = pt2.x - pt0.x;
    double dy2 = pt2.y - pt0.y;
    return (dx1*dx2 + dy1*dy2)/sqrt((dx1*dx1 + dy1*dy1)*(dx2*dx2 + dy2*dy2) + 1e-10);
}

void ImageProcSegment::doProccess()
{
    disconnect(timer,SIGNAL(timeout()),this,SLOT(timer_inteval()));

    newDataRecieved = false;
    Mat ranged;
    //    semaphoreForRanges->tryAcquire(1,30);
    //    inRange(input
    //            ,Scalar(minColorRange.val[0],minColorRange.val[1],minColorRange.val[2])
    //            ,Scalar(maxColorRange.val[0],maxColorRange.val[1],maxColorRange.val[2])
    //            ,ranged);

    //    emit afterFilter(ranged);

    //    semaphoreForRanges->release(1);
    input.copyTo(ranged);

    if(this->color == "black")
    {


        cropR.x = 20;
        cropR.y = 20;
        cropR.width = ranged.cols - 50;
        cropR.height = ranged.rows - 50;

        Mat crop(ranged,cropR);

        medianBlur(crop,crop,3);
        Mat structure=getStructuringElement(MORPH_RECT,Size(5,5));
        //        dilate(crop,crop,structure);
        //        Mat structure=getStructuringElement(MORPH_RECT,Size(5,5));
        erode(crop,crop,structure);
        //        dilate(ranged,ranged,structure);
        //        medianBlur(ranged,ranged,7);
        //        Mat structure2=getStructuringElement(MORPH_RECT,Size(5,5));
        //        erode(ranged,ranged,structure2);
        Canny( crop, crop, 80, 180, 3 );
        RobotDetection(crop);
    }
    else
    {
        medianBlur(ranged,ranged,7);
        Canny( ranged, ranged, 80, 180, 3 );
        shapeDetection(ranged);
    }

    emit dataGenerated();

    connect(timer,SIGNAL(timeout()),this,SLOT(timer_inteval()));
}

void ImageProcSegment::timer_inteval()
{
    if(newDataRecieved)
    {
        doProccess();
    }
}
