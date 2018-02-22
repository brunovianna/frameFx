#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup()
{



    ofSetEscapeQuitsApp(true);
    ofSetBackgroundAuto(false);
    ofBackground(55);


    gui = new ofxUICanvas();        //Creates a canvas at (0,0) using the default width

    gui->loadSettings("settings.xml");

    gui->addLabel("CV EFFECTS", OFX_UI_FONT_LARGE);

    gui->addSpacer();
    gui->addLabel("drop video here to start", OFX_UI_FONT_SMALL);
    gui->addSpacer();
    gui->addLabelButton("save video", false);
    gui->addLabelButton("save one frame", false);
    gui->addLabelButton("restart", false);
    gui->addSpacer();
    gui->addToggle("export frames", false);
    gui->addSpacer();
    gui->addLabel("background", OFX_UI_FONT_SMALL);
    vector<string> backgroundOptions;
    backgroundOptions.push_back("black");
    backgroundOptions.push_back("video");
    backgroundOptions.push_back("frame");
    ofxUIRadio *backgroundRadio = (ofxUIRadio *) gui->addRadio("background", backgroundOptions, OFX_UI_ORIENTATION_HORIZONTAL);
    backgroundRadio->activateToggle("video");
    backgroundMode = 1;


    gui->addSpacer();
    gui->addLabel("blend", OFX_UI_FONT_SMALL);
    ofxUIMinimalSlider * blendMinSli = gui->addMinimalSlider("blend",0.0,255.0,255.0);
    blendMinSli->setLabelVisible(false);
    blendLevel = 255.0;

    gui->addLabel("scale", OFX_UI_FONT_SMALL);
    vector<string> scales;
    scales.push_back("1/4");
    scales.push_back("1/3");
    scales.push_back("1/2");
    scales.push_back("2/3");
    scales.push_back("3/4");
    scales.push_back("1");

    scaleRadio = (ofxUIRadio *) gui->addRadio("scale", scales, OFX_UI_ORIENTATION_HORIZONTAL);

    gui->addSpacer();
    vector<string> effects;
    effects.push_back("no effects");
    effects.push_back("feature lines");
    effects.push_back("optical flow lines");
    effects.push_back("track optical flow");
    effects.push_back("bg extract");
    effects.push_back("surf waves");
    effects.push_back("gradient motion");
    effects.push_back("contours");
    effects.push_back("glitch");
    ddl = gui->addDropDownList("effects chooser", effects);

    //label is not working
    //ddl->setLabelVisible(true);
    //ddl->setLabelText("effects chooser");
    //ddl->setLabelPosition(OFX_UI_WIDGET_POSITION_UP);
    ddl->checkAndSetTitleLabel();
    ddl->setAllowMultiple(false);
    ddl->setAutoClose(false);
    ddl->setShowCurrentSelected(true);

    ofAddListener(gui->newGUIEvent, this, &ofApp::guiEvent);

    //blendMinSli->setValue(0.0);

    gui->autoSizeToFitWidgets();
    gui->setHeight(HEIGHT-100);


    scaleRadio->activateToggle("1");
    scaleFloat = 1.0;

    ofFile lastVideoName;

    if (lastVideoName.doesFileExist(ofToDataPath("lastVideoName.txt")))
    {

        ofBuffer buff = ofBufferFromFile(ofToDataPath("lastVideoName.txt"));
        fileName = buff.getFirstLine();


    }
    cv::initModule_nonfree();//THIS LINE IS IMPORTANT


    lastPointsOpticalFlow = std::vector<cv::Point2f>(MAX_FEATURES);
    goodFeatureDetector = cv::GoodFeaturesToTrackDetector (MAX_FEATURES);

    siftFinder = new cv::SIFT();
    //surfFeatures = new cv::detail::ImageFeatures();
}

//--------------------------------------------------------------
void ofApp::update()
{

    if (mMovie.isLoaded())
        mMovie.update();


}

//--------------------------------------------------------------
void ofApp::draw()
{

    ofxCvColorImage ofxCvImgTmp;
    cv::Mat frame(movieHeight,movieWidth,CV_32FC1);

    if (!mMovie.isLoaded())
    {
        return;
    }


    if (!mMovie.isFrameNew())
    {
    }
    else
    {

        ofxCvImgTmp.setFromPixels(mMovie.getPixels());
        frame=ofxCvImgTmp.getCvImage();

        resized = myResize (frame, scaleFloat);

        //for glitch
        cv::Mat blank(resized.rows,resized.cols,CV_32F);


        if (blendLevel  == 255.0)
        {
            ofEnableBlendMode(OF_BLENDMODE_DISABLED);
        }
        else
        {
            ofEnableBlendMode(OF_BLENDMODE_ADD);
            ofSetColor(255,255,255,blendLevel);
        }

        if (backgroundMode==0)
        {
            ofSetColor (0,0,0,blendLevel);
            ofRect (moviePosX,moviePosY,movieWidth,movieHeight);
            ofSetColor(255,255,255,blendLevel);
        }
        else if (backgroundMode==1)
        {
            ofIm.setFromPixels(resized.data,(int)scaledWidth,(int)scaledHeight, OF_IMAGE_COLOR);
            ofIm.draw(moviePosX, moviePosY);
        }
        else if (backgroundMode==2)
        {
            firstFrame.draw(moviePosX,moviePosY);
        }
        else if (backgroundMode=-1)
        {
        }

        switch (effectNum)
        {

        case 0:
            //no effect
            //    ofIm.setFromPixels(resized.data,(int)scaledWidth,(int)scaledHeight, OF_IMAGE_COLOR);
            //    ofIm.draw(moviePosX, moviePosY);



            break;

        case 1:

            ofIm = ofImage(effectBgExtract(resized));
            ofIm.draw(moviePosX, moviePosY);

            break;

        case 2:

            effectGoodOpticalFlow(resized);

            break;

        case 3:

            //ofIm.draw(moviePosX, moviePosY);
            effectFeatureLines(resized);

            break;

        case 4:

            //ofIm.draw(moviePosX, moviePosY);
            effectTrackOpticalFlow(resized);

            break;

        case 5:
            //surf waves
            effectSurfWaves(resized);

            break;

        case 6:

            ofIm = ofImage(effectGradientMotion(resized));
            ofIm.draw(moviePosX, moviePosY);

        break;


        case 8:
             // beautiful glitch when initiliaze this way cv::Mat blank(h,w,CV_8UC3);
            cv::cvtColor(resized, blank, CV_RGB2GRAY);
            ofIm.setFromPixels(blank.data,blank.cols,blank.rows, OF_IMAGE_COLOR);
            ofIm.draw(moviePosX, moviePosY);
        break;

        }

        if (exportFramesOn)
        {
            string frameString;
            ostringstream convert;

            convert << setfill('0') << setw (4) << frameNumber << ".jpg";
            frameString = convert.str();

            ofImage screen;
            screen.grabScreen(moviePosX,moviePosY,scaledWidth,scaledHeight);
            screen.saveImage(savePath+frameString);

            frameNumber++;
            mMovie.nextFrame();
            mMovie.update();

        }

    }

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key)
{

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key)
{



}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y )
{

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button)
{

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button)
{

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button)
{

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h)
{

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg)
{

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo)
{
    if( dragInfo.files.size() > 0 ) {
        openMovie(dragInfo.files[0]);
        fileName = dragInfo.files[0];
    }
}


void ofApp::exit()
{
    gui->saveSettings("settings.xml");
    delete gui;

    // create or open a file
    ofFile pFile (ofToDataPath("lastVideoName.txt"),ofFile::WriteOnly);


    // add some lines of text
    pFile << fileName;

    // save file
    pFile.close();

}

void ofApp::guiEvent(ofxUIEventArgs &e)
{

    if(e.getName() == "restart")
    {

        if (!mMovie.isLoaded())
        {
            mMovie.loadMovie(fileName);
        }
        mMovie.firstFrame();
        mMovie.play();


    }
    else if(e.getName() == "scale")
    {
        ofxUIRadio *radio = (ofxUIRadio *) e.widget;
        int scaleChoice = radio->getValue();

        setScale(scaleChoice);

        setupEffect();

        ofClear(55);

    }
    else if(e.getName() == "save video")
    {

        ofxUIButton *labelbutton = e.getButton();

        if (labelbutton->getValue())
            exportMovie();

    }
    else if(e.getName() == "export frames")
    {

        ofxUIButton *labelbutton = e.getButton();

        if (labelbutton->getValue())
        {
            exportFrames();
            mMovie.setPaused(true);
            mMovie.nextFrame();
            mMovie.update();
            exportFramesOn = true;
        }
        else
        {
            mMovie.setPaused(false);
            exportFramesOn = false;
        }
    }
    else if(e.getName() == "effects chooser")
    {

        ofxUIDropDownList *ddlist = (ofxUIDropDownList *) e.widget;
        vector<ofxUIWidget *> &selected = ddlist->getSelected();


        for(vector<ofxUIWidget *>::iterator it = selected.begin(); it != selected.end(); ++it)
        {
            ofxUILabelToggle *lt = (ofxUILabelToggle *) (*it);
            selectEffect(lt->getName());

        }


    }
    else if(e.getName() == "features")
    {

        ofxUIMinimalSlider *minSli = (ofxUIMinimalSlider *) e.widget;
        MAX_FEATURES = (int)minSli->getValue();
        ofClear(55);
        setupEffect();

    }
    else if(e.getName() == "background")
    {

        ofxUIRadio *radio = (ofxUIRadio *) e.widget;
        backgroundMode = radio->getValue();

    }
    else if(e.getName() == "blend")
    {

        ofxUIMinimalSlider *sli = (ofxUIMinimalSlider *) e.widget;
        blendLevel = (int)sli->getValue();


    }
}

void ofApp::setScale(int scaleChoice)
{
    switch (scaleChoice)
    {

    case 0:
        scaleFloat = 0.25;
        break;

    case 1:
        scaleFloat = 0.33333333333333333333;
        break;
    case 2:
        scaleFloat = 0.5;
        break;

    case 3:
        scaleFloat = 0.66666666666666666666;
        break;

    case 4:
        scaleFloat = 0.75;
        break;

    case 5:
        scaleFloat = 1.0;


    }

    scaledHeight = movieHeight * scaleFloat;
    scaledWidth = movieWidth * scaleFloat;
    setupEffect();

}


void ofApp::openMovie (string result) {

    if (mMovie.loadMovie(result)) {
        movieWidth = mMovie.getWidth();
        movieHeight = mMovie.getHeight();
        scaledWidth = (int)movieWidth * scaleFloat;
        scaledHeight = (int)movieHeight * scaleFloat;


        mMovie.play();

        //mMovie.pause();
    } else {
        ofSystemAlertDialog("the movie couldn't be loaded");
    }

}

void ofApp::exportMovie()
{

    float rate;

    if (!mMovie.isLoaded())
    {
        ofSystemAlertDialog("please load a movie first");
        return;
    }

    mMovie.setPaused(true);
    mMovie.firstFrame();

    if ((mMovie.getTotalNumFrames()!=0)&&(mMovie.getDuration()!=0))
    {
        rate =  mMovie.getTotalNumFrames() / mMovie.getDuration() ;

    }
    else
    {

        rate =  24 ;

    }


    cout << "rate " << rate;


    //string saveFileName = "/dados/videos/multidao/testMovie";
    string saveFileName = "exported-";
    string fileExt = ".mov"; // ffmpeg uses the extension to determine the container type. run 'ffmpeg -formats' to see supported formats

    // override the default codecs if you like
    // run 'ffmpeg -codecs' to find out what your implementation supports (or -formats on some older versions)
    //vidRecorder.setVideoCodec("copy");
    //vidRecorder.setVideoCodec("copy");
    //vidRecorder.setVideoCodec("msmpeg4v2");
    vidRecorder.setVideoCodec("libx264");
    //vidRecorder.setPixelFormat("rgb24");
    //vidRecorder.setVideoCodec("mjpeg");
    vidRecorder.setVideoBitrate("50000k");
    vidRecorder.setAudioCodec("libmp3lame");

    vidRecorder.setAudioBitrate("192k");

    vidRecorder.setup(saveFileName+ofGetTimestampString()+fileExt, scaledWidth, scaledHeight, rate,44100,0);

    mMovie.nextFrame();
    mMovie.update();
    while (mMovie.getCurrentFrame()< mMovie.getTotalNumFrames())
    {
        if (mMovie.isFrameNew())
        {

            ofImage ofImTmp;
            ofxCvColorImage ofxCvImgTmp;


            cv::Mat frame(movieHeight,movieWidth,CV_32FC1);
            ofxCvImgTmp.setFromPixels(mMovie.getPixels());
            frame=ofxCvImgTmp.getCvImage();

            cv::Mat resized;
            resized = myResize (frame, scaleFloat);

            ofImTmp.setFromPixels(resized.data,resized.cols,resized.rows, OF_IMAGE_COLOR);
            vidRecorder.addFrame(ofImTmp.getPixelsRef());

            mMovie.nextFrame();
            mMovie.update();
            cout << mMovie.getCurrentFrame() << " "<< mMovie.getTotalNumFrames() << "\n";
        }
        else
        {
            mMovie.update();
            //ofSleepMillis(1);
        }
    }


    vidRecorder.close();
    ofSystemAlertDialog("done");

}

void ofApp::exportFrames()
{

    savePath = "exported_frames_"+ofGetTimestampString()+"/";

    ofDirectory dir;

    dir.createDirectory (savePath, false);

    frameNumber = 0;

    mMovie.setPaused(true);

}



cv::Mat ofApp::myResize (cv::Mat src, float s)
{

    cv::Mat dst;

    if (s!=1.0)
    {
        cv::resize (src, dst, cv::Size(), s, s );
        return dst;

    }
    else
    {

        return src.clone();

    }



}



/* effects */
void ofApp::selectEffect (string effectName)
{

    cleanEffect();

    if (effectName == "no effects" )
    {

        effectNum = 0;
        setupEffect();


    }
    else if (effectName == "bg extract")
    {

        effectNum = 1;
        setupEffect();


    }
    else if (effectName == "optical flow lines")
    {


        effectNum = 2;
        setupEffect();

    }
    else if (effectName == "track optical flow")
    {


        effectNum = 4;
        setupEffect();

    }
    else if (effectName == "feature lines")
    {

        effectNum = 3;
        setupEffect();

    }
    else if (effectName == "surf waves")
    {

        effectNum = 5;
        setupEffect();

    }
    else if (effectName == "gradient motion")
    {

        effectNum = 6;
        setupEffect();

    }
    else if (effectName == "contours")
    {

        effectNum = 7;
        setupEffect();

    }
    else if (effectName == "glitch")
    {

        effectNum = 8;

    }

}


void ofApp::setupEffect()
{

    firstFrame.setFromPixels(resized.data,(int)scaledWidth,(int)scaledHeight, OF_IMAGE_COLOR);

    switch (effectNum)
    {

    case 0:
        //no effect

        break;


    case 1:
        // bg extractor
        bgSubtractor = cv::BackgroundSubtractorMOG2 (60, 56.0, true);
        for (int i=0; i<toBeRemoved.size(); i++)
            gui->removeWidget(toBeRemoved[i]);
        gui->autoSizeToFitWidgets();

        break;

    case 2:
        //optical flow

        firstFrameOpticalFlow = true;
        if (!findInVector(toBeRemoved, "features"))
        {
            ofxUIMinimalSlider * featuresSlider = gui->addMinimalSlider("features", 0,500, 400);
            toBeRemoved.push_back("features");
            featuresSlider->setShowValue(false);


            gui->autoSizeToFitWidgets();
        }
        bgSubtractor = cv::BackgroundSubtractorMOG2 (60, 26.0, true);

        break;

    case 3:
        //feature lines

        bgSubtractor = cv::BackgroundSubtractorMOG2 (60, 26.0, true);
        goodFeatureDetector = cv::GoodFeaturesToTrackDetector (MAX_FEATURES);

        break;

    case 4:
        //track optical flow

        firstFrameOpticalFlow = true;
        if (!findInVector(toBeRemoved, "features"))
        {
            ofxUIMinimalSlider * featuresSlider = gui->addMinimalSlider("features", 0,500, 400);
            toBeRemoved.push_back("features");
            featuresSlider->setShowValue(false);


            gui->autoSizeToFitWidgets();
        }
        bgSubtractor = cv::BackgroundSubtractorMOG2 (60, 26.0, true);
        lastPointsOpticalFlow = std::vector<cv::Point2f>(MAX_FEATURES);
        goodFeatureDetector = cv::GoodFeaturesToTrackDetector (MAX_FEATURES);

        break;

    case 5:
        //surf waves


    break;

    case 6:

    break;
    }

}

void ofApp::cleanEffect()
{

    firstFrame.setFromPixels(resized.data,(int)scaledWidth,(int)scaledHeight, OF_IMAGE_COLOR);

    switch (effectNum)
    {
    case 2:
        //optical flow

        //delete(bgSubtractor);
        //bgSubtractor= NULL;
        //delete(&lastPointsOpticalFlow);
        //lastPointsOpticalFlow=NULL;
        //delete(&goodFeatureDetector);
        //goodFeatureDetector=NULL;

        break;


    }
}

ofImage ofApp::effectBgExtract (cv::Mat color_img)
{

    ofImage ofImTmp;

    int th = color_img.rows;
    int tw = color_img.cols;

    cv::Mat imgReturn, bgMask;
    cv::Mat grey_img, blurred;

    cv::cvtColor(color_img, grey_img, CV_RGB2GRAY);

    bgSubtractor(grey_img,bgMask);

    blurred = bgMask.clone();
    for ( int i = 1; i < 5; i = i + 2 )
        cv::GaussianBlur( bgMask, blurred, cv::Size( i, i ), 0,0);

    cv::cvtColor(blurred, imgReturn, CV_GRAY2RGB);
    ofImTmp.setFromPixels(imgReturn.data,imgReturn.cols,imgReturn.rows, OF_IMAGE_COLOR);
    return ofImTmp;

}

void ofApp::effectGoodOpticalFlow (cv::Mat color_img)
{


    vector<uchar> status;
    vector<float> errors;

    cv::Mat grey_img, bgMask;

    // beautiful glitch when initiliaze this way cv::Mat blank(h,w,CV_8UC3);
    //cv::Mat blank(color_img.rows,color_img.cols,CV_8UC3, cv::Scalar(50,50,50));

    cv::cvtColor(color_img, grey_img, CV_RGB2GRAY);
    //bgSubtractor(grey_img,bgMask);

    if (firstFrameOpticalFlow)
    {
        lastImgOpticalFlow = grey_img.clone();
        firstFrameOpticalFlow = false;
    }
    else
    {
        //lastpoints = points;
        goodFeatureDetector.detect( lastImgOpticalFlow, keyPointsOpticalFlow );
        //points.resize(keypoints.size());
        for (unsigned int i=0; i<keyPointsOpticalFlow.size(); i++)
        {
            lastPointsOpticalFlow[i].x = keyPointsOpticalFlow[i].pt.x;
            lastPointsOpticalFlow[i].y = keyPointsOpticalFlow[i].pt.y;
        }
        std::vector<cv::Point2f> points(MAX_FEATURES); // 500 corners as max

        //cv::TermCriteria tc (cv::TermCriteria::COUNT+cv::TermCriteria::EPS, 30, 0.01);

        cv::calcOpticalFlowPyrLK(lastImgOpticalFlow,grey_img,lastPointsOpticalFlow,points,status, errors, cv::Size(11,11));

        lastImgOpticalFlow= grey_img.clone();

        for (unsigned int i=0; i<points.size(); i++ )
        {
            if( status[i]==0|| errors[i]>550 )
            {
                continue;
            }

            //cv::Point p = lastpoints[i];
            //cv::Point q = points[i];
            //line(blank, p, q, cv::Scalar(230,230,230),1,CV_AA);
            //circle (color_img, p, 3, cv::Scalar(255),1,CV_AA);

            ofPushMatrix();
            ofTranslate(moviePosX,moviePosY);
            ofVec2f startPoint (lastPointsOpticalFlow[i].x, lastPointsOpticalFlow[i].y);
            ofVec2f endPoint ( points[i].x, points[i].y);

            if (startPoint.distance(endPoint) <  color_img.cols /50.0)
                ofLine (lastPointsOpticalFlow[i].x, lastPointsOpticalFlow[i].y, points[i].x, points[i].y);
            ofPopMatrix();

        }



    }


}


void ofApp::effectTrackOpticalFlow (cv::Mat color_img)
{


    vector<uchar> status;
    vector<float> errors;

    cv::Mat grey_img, bgMask;

    // beautiful glitch when initiliaze this way cv::Mat blank(h,w,CV_8UC3);
    //cv::Mat blank(color_img.rows,color_img.cols,CV_8UC3, cv::Scalar(50,50,50));

    cv::cvtColor(color_img, grey_img, CV_RGB2GRAY);
    bgSubtractor(grey_img,bgMask);

    if (firstFrameOpticalFlow)
    {
        lastImgOpticalFlow = bgMask.clone();
        firstFrameOpticalFlow = false;
    }
    else
    {
        //lastpoints = points;
        goodFeatureDetector.detect( lastImgOpticalFlow, keyPointsOpticalFlow );
        //points.resize(keypoints.size());
        for (unsigned int i=0; i<keyPointsOpticalFlow.size(); i++)
        {
            lastPointsOpticalFlow[i].x = keyPointsOpticalFlow[i].pt.x;
            lastPointsOpticalFlow[i].y = keyPointsOpticalFlow[i].pt.y;
        }
        std::vector<cv::Point2f> points(MAX_FEATURES); // 500 corners as max

        //cv::TermCriteria tc (cv::TermCriteria::COUNT+cv::TermCriteria::EPS, 30, 0.01);

        cv::calcOpticalFlowPyrLK(lastImgOpticalFlow,bgMask,lastPointsOpticalFlow,points,status, errors, cv::Size(11,11));

        lastImgOpticalFlow= bgMask.clone();

        for (unsigned int i=0; i<points.size(); i++ )
        {
            if( status[i]==0|| errors[i]>550 )
            {
                continue;
            }

            //cv::Point p = lastpoints[i];
            //cv::Point q = points[i];
            //line(blank, p, q, cv::Scalar(230,230,230),1,CV_AA);
            //circle (color_img, p, 3, cv::Scalar(255),1,CV_AA);

            ofPushMatrix();
            ofTranslate(moviePosX,moviePosY);
            ofVec2f startPoint (lastPointsOpticalFlow[i].x, lastPointsOpticalFlow[i].y);
            ofVec2f endPoint ( points[i].x, points[i].y);

            if (startPoint.distance(endPoint) <  color_img.cols /50.0)
                ofLine (lastPointsOpticalFlow[i].x, lastPointsOpticalFlow[i].y, points[i].x, points[i].y);
            ofPopMatrix();

        }



    }


}

void ofApp::effectFeatureLines(cv::Mat color_img )
{

    vector<uchar> status;
    vector<float> errors;

    cv::Mat grey_img, bgMask;

    // beautiful glitch when initiliaze this way cv::Mat blank(h,w,CV_8UC3);
    //cv::Mat blank(color_img.rows,color_img.cols,CV_8UC3, cv::Scalar(50,50,50));

    cv::cvtColor(color_img, grey_img, CV_RGB2GRAY);
    bgSubtractor(grey_img,bgMask);

    goodFeatureDetector.detect( bgMask, keyPointsOpticalFlow );

    ofPushMatrix();
    ofTranslate(moviePosX,moviePosY);
    for (unsigned int i=0; i<keyPointsOpticalFlow.size(); i+=2)
    {

        //ofVec2f startPoint (lastPointsOpticalFlow[i].x, lastPointsOpticalFlow[i].y);
        //ofVec2f endPoint ( points[i].x, points[i].y);

        //if (startPoint.distance(endPoint) <  color_img.cols /50.0)
        ofLine (keyPointsOpticalFlow[i].pt.x, keyPointsOpticalFlow[i].pt.y, keyPointsOpticalFlow[i+1].pt.x, keyPointsOpticalFlow[i+1].pt.y );
    }

    ofPopMatrix();



}

void ofApp::effectSurfWaves (cv::Mat rzd)
{

    cv::Mat grey_img;
    cv::cvtColor(rzd, grey_img, CV_RGB2GRAY);
    (*siftFinder)(grey_img, cv::noArray(), siftFeatures, cv::noArray());


    /*
        if (firstSurf) {

            firstSurf = false;
            firstSurfMatch = true;
            lastSurfFeatures = surfFeatures;

        } else if (firstSurfMatch) {

            firstSurfMatch = false;
            cv::detail::MatchesInfo pairwise_matches;
            cv::detail::BestOf2NearestMatcher matcher(true);
            matcher(surfFeatures, lastSurfFeatures, pairwise_matches);
            matcher.collectGarbage();
            delete (&matcher);

            for (unsigned int i=0;((i<pairwise_matches.matches.size())&&(i<20));i+=4) {

                vector <int> bezier(4);
                bezier[0] = pairwise_matches.matches[i].trainIdx;
                bezier[1] = pairwise_matches.matches[i+1].trainIdx;
                bezier[2] = pairwise_matches.matches[i+2].trainIdx;
                bezier[3] = pairwise_matches.matches[i+3].trainIdx;
                surfBezier.push_back(bezier);

            }


            lastSurfFeatures = surfFeatures;

        } else {


            cv::detail::MatchesInfo pairwise_matches;
            cv::detail::BestOf2NearestMatcher matcher(true);
            matcher(surfFeatures, lastSurfFeatures, pairwise_matches);
            matcher.collectGarbage();
            delete (&matcher);

            for (std::vector< vector<int> > ::iterator it = surfBezier.begin() ; it != surfBezier.end(); ++it) {
            //for (unsigned int i=0;i<surfBezier.size();i++) {
                for (unsigned int j=0;j<4;j++) {
                    int bezier_complete = 0;
                    for (unsigned int k=0;k<pairwise_matches.matches.size();k++) {
                        if ((vector <int> )it[j]==pairwise_matches.matches[k].queryIdx) {
                            *it[j]= pairwise_matches.matches[k].trainIdx;
                            bezier_complete++;
                            }

                    }
                    if (bezier_complete<4)
                        surfBezier.erase(i);
                }
            }

            ofPushMatrix();
            ofNoFill();
            ofTranslate(moviePosX,moviePosY);



            for (unsigned int i=0;((i<surfFeatures.keypoints.size())&&(i<20));i+=4) {

                    //ofVec2f startPoint (lastPointsOpticalFlow[i].x, lastPointsOpticalFlow[i].y);
                    //ofVec2f endPoint ( points[i].x, points[i].y);

                    //if (startPoint.distance(endPoint) <  color_img.cols /50.0)
                //ofLine (keyPointsOpticalFlow[i].pt.x, keyPointsOpticalFlow[i].pt.y, keyPointsOpticalFlow[i+1].pt.x, keyPointsOpticalFlow[i+1].pt.y );
                //ofCircle(surfFeatures.keypoints[i].pt.x, surfFeatures.keypoints[i].pt.y, 2);


                ofBezier(surfFeatures.keypoints[i].pt.x, surfFeatures.keypoints[i].pt.y, surfFeatures.keypoints[i+1].pt.x, surfFeatures.keypoints[i+1].pt.y,
                         surfFeatures.keypoints[i+2].pt.x, surfFeatures.keypoints[i+2].pt.y, surfFeatures.keypoints[i+3].pt.x, surfFeatures.keypoints[i+3].pt.y);
            }
            ofFill();

            ofPopMatrix();

        }
    */

    ofPushMatrix();
    ofNoFill();
    ofTranslate(moviePosX,moviePosY);

    ofBeginShape();

    for (unsigned int i=0; ((i<siftFeatures.size())&&(i<20)); i++)
    {

        ofCurveVertex(siftFeatures[i].pt.x, siftFeatures[i].pt.y);


    }
    ofEndShape();
    ofFill();

    ofPopMatrix();


}

ofImage ofApp::effectGradientMotion (cv::Mat rsz) {

    ofImage ofImTmp;
    ofxCvColorImage ofxCvImgTmp;


    //cv::Mat frame(movieHeight,movieWidth,CV_32FC1,mMovie.getPixels());
    cv::Mat frame(movieHeight,movieWidth,CV_32FC1);
    ofxCvImgTmp.setFromPixels(mMovie.getPixels());
    frame=ofxCvImgTmp.getCvImage();
    cv::Mat e_orientation (rsz.rows, rsz.cols, CV_32FC1);
    cv::Mat e_mask (rsz.rows, rsz.cols, CV_32FC1);
    cv::Mat e_mhi (rsz.rows, rsz.cols, CV_32FC1);

    cv::Mat ret, gray, grad;
    cv::Mat grad_x, grad_y;
    cv::Mat abs_grad_x, abs_grad_y;
    int scale = 1;
    int delta = 0;
    int ddepth = CV_16S;

    cv::GaussianBlur( rsz, rsz, Size(3,3), 0, 0, BORDER_DEFAULT );
     /// Convert it to gray
    cv::cvtColor( rsz, gray, CV_RGB2GRAY );

  /// Gradient X
  //Scharr( src_gray, grad_x, ddepth, 1, 0, scale, delta, BORDER_DEFAULT );
    cv::Sobel( gray, grad_x, ddepth, 1, 0, 3, scale, delta, BORDER_DEFAULT );
    cv::convertScaleAbs( grad_x, abs_grad_x );

  /// Gradient Y
  //Scharr( src_gray, grad_y, ddepth, 0, 1, scale, delta, BORDER_DEFAULT );
    cv::Sobel( gray, grad_y, ddepth, 0, 1, 3, scale, delta, BORDER_DEFAULT );
    cv::convertScaleAbs( grad_y, abs_grad_y );

  /// Total Gradient (approximate)
    cv::addWeighted( abs_grad_x, 0.5, abs_grad_y, 0.5, 0, grad );



    //ofImTmp.setFromPixels(rsz.data,rsz.cols,rsz.rows, OF_IMAGE_COLOR);
    ofImTmp.setFromPixels(grad.data,grad.cols,grad.rows, OF_IMAGE_GRAYSCALE);
    return ofImTmp;
}


bool ofApp::findInVector (vector<string> haystack, string needle)
{

    for (int i=0; i<haystack.size(); i++)
    {
        if (haystack[i]==needle)
            return true;

    }

    return false;

}
