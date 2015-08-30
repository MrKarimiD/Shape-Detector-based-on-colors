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

        if(approx.size() < 7)// !!!!!!!!!
        {// !!!!!!!!
            double sum = arcLength(Mat(contours[i]), true);
            Point first,second;
            double avg = sum / approx.size();

            for(int i=0;i<approx.size();i++)
            {
                for(int j=i+1;j<approx.size();j++)
                {
                    first = approx.at(i);
                    second = approx.at(j);

                    if( sqrt(pow(second.x-first.x,2)+pow(second.y-first.y,2))/avg < 0.5)
                    {
                        Point between;
                        between.x = (first.x+second.x)/2;
                        between.y = (first.y+second.y)/2;
                        approx.erase(approx.begin()+i);
                        approx.erase(approx.begin()+j);
                        approx.insert(approx.begin()+i,between);
                    }
                }
            }
        } // !!!!!!!!!

        if (approx.size() == 3)
        {
            gravCenter.x = (approx.at(0).x + approx.at(1).x + approx.at(2).x) / 3;
            gravCenter.y = (approx.at(0).y + approx.at(1).y + approx.at(2).y) / 3;
            prepareDataForOutput(contours[i],"TRI");
        }
        else if (approx.size() >= 4 && approx.size() <= 6)
        {
            // Number of vertices of polygonal curve
            int vtc = approx.size();

            // Get the cosines of all corners
            vector<double> cos;
            for (int j = 2; j < vtc+1; j++)
                cos.push_back(angle(approx[j%vtc], approx[j-2], approx[j-1]));

            // Sort ascending the cosine values
            sort(cos.begin(), cos.end());

            // Get the lowest and the highest cosine
            double mincos = cos.front();
            double maxcos = cos.back();

            // Use the degrees obtained above and the number of vertices
            // to determine the shape of the contour
            if (vtc == 4 && mincos >= -0.3 && maxcos <= 0.3)  //-0.1,0.3
            {
                prepareDataForOutput(contours[i],"RECT");
            }
            else if ( (vtc >= 5 && vtc <=7) && mincos >= -0.7 && maxcos <= 0.5) //-0.34,-0.27
            {
                prepareDataForOutput(contours[i],"PENTA");
            }
            else
            {
                recycledShapes.push_back(contours[i]);
            }
        }
        else
        {
            // Detect and label circles
            double area = contourArea(contours[i]);
            Rect r = boundingRect(contours[i]);
            int radius = r.width / 2;

//            if (abs(1 - ((double)r.width / r.height)) <= 0.3 &&
//                    abs(1 - (area / (CV_PI * pow(radius, 2)))) <= 0.3)
//            {
            if (abs(1 - ((double)r.width / r.height)) <= 0.5 &&
                    abs(1 - (area / (CV_PI * pow(radius, 2)))) <= 0.5)
            {
                prepareDataForOutput(contours[i],"CIR");
            }
            else
            {
                recycledShapes.push_back(contours[i]);
            }
       }
    }

    for(int i=0;i<recycledShapes.size();i++)
    {
            if(contourArea(recycledShapes[i]) >(0.01)*imSize.width*imSize.height)
            {
               prepareDataForOutput(recycledShapes[i],"Chasbideh");
           }
    }
    //    vector<Vec3f> circles;
    //    HoughCircles(input, circles, HOUGH_GRADIENT,3,1, 150, 100 );
    //    qDebug()<<"size:"<<circles.size();
    //    for(int i=0;i<circles.size();i++)
    //    {
    //        Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
    //        int radius = cvRound(circles[i][2]);
    //        circle(dst,center,radius,Scalar(0,124,124),3);
    //    }
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
    findContours(input.clone(), contours, hierarchy, RETR_LIST, CHAIN_APPROX_SIMPLE,cropR.tl());

    //qDebug()<<"--------------";
    for (int i = 0; i < contours.size(); i++)
    {
        //qDebug()<<"contoursArea:"<<fabs(contourArea(contours[i]));
         //Skip small or non-convex objects
        //qDebug()<<"contour:"<<contourArea(contours[i]);

       if (fabs(contourArea(contours[i])) < 50 )
            continue;

        if(!checkAspectRatio(contours[i]))
            continue;

        RotatedRect rotatetBoundRect=minAreaRect(Mat(contours[i]));
        if(!checkAspectRatioForRotatedRect(rotatetBoundRect))
        {
            continue;
        }

        Point2f center;
        float radius;
        minEnclosingCircle( (Mat)contours[i], center, radius );

        if(radius < 5)
            continue;

        prepareDataForOutput(contours[i],"Robot");
        //robotList.push_back(contours[i]);
    }

//    for(int i=0;i<robotList.size();i++)
//    {
//        for(int j=i;j<robotList.size();j++)
//        {
//            if( fabs(contourArea(robotList[i])) < fabs(contourArea(robotList[j])) )
//            {
//                vector<Point> temp;
//                temp = robotList.at(i);
//                robotList.at(i) = robotList.at(j);
//                robotList.at(j) = temp;
//            }
//        }
//    }
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
    Mat src(1,1,CV_32FC2);
    Mat warp_dst(1,1,CV_32FC2);
    if(type == "TRI")
    {
        Yman = -(Orgin_X - (gravCenter.x/imSize.width)*Width);
        Xman = Orgin_Y + (gravCenter.y/imSize.height)*Height;
    }
    else
    {
        Yman = -(Orgin_X - (center.x/imSize.width)*Width);
        Xman = Orgin_Y + (center.y/imSize.height)*Height;
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
//        Mat structure=getStructuringElement(MORPH_RECT,Size(5,5));
//        dilate(crop,crop,structure);
//        Mat structure=getStructuringElement(MORPH_RECT,Size(5,5));
//        erode(outputFrame,outputFrame,structure);
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
