#pragma once

#include "ofMain.h"
#include "ofxUI.h"
#include "ofxOpenCv.h"
#include "ofxVideoRecorder.h"

#include "opencv2/opencv_modules.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/video/background_segm.hpp"
#include "opencv2/video/tracking.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/stitching/detail/autocalib.hpp"
#include "opencv2/stitching/detail/blenders.hpp"
#include "opencv2/stitching/detail/camera.hpp"
#include "opencv2/stitching/detail/exposure_compensate.hpp"
#include "opencv2/stitching/detail/matchers.hpp"
#include "opencv2/stitching/detail/motion_estimators.hpp"
#include "opencv2/stitching/detail/seam_finders.hpp"
#include "opencv2/stitching/detail/util.hpp"
#include "opencv2/stitching/detail/warpers.hpp"
#include "opencv2/stitching/warpers.hpp"
#include "opencv2/nonfree/nonfree.hpp"
#include "opencv2/features2d/features2d.hpp"


#define HEIGHT 1024
#define WIDTH 768

using namespace cv;
using namespace cv::detail;


template <class T>
inline void zap(T & x)
{
        {assert(x != NULL);}
        delete x;
        x = NULL;
}

// In C++ the reason there are 2 forms of the delete operator is - because
// there is no way for C++ to tell the difference between a pointer to
// an object and a pointer to an array of objects. The delete operator
// relies on the programmer using "[]" to tell the two apart.
// Hence, we need to define zaparr function below.
// To delete array of pointers
template <class T>
inline void zaparr(T & x)
{
         {assert(x != NULL);}
     delete [] x;
     x = NULL;
}

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);


        ofxUICanvas *gui;
        ofxUIDropDownList *ddl;
        ofxUIRadio *scaleRadio;

        void setScale (int scaleChoice);
        vector<string> toBeRemoved;

        void exit();
        void guiEvent(ofxUIEventArgs &e);

        float movieWidth, movieHeight, scaledWidth, scaledHeight;
        float scaleFloat;

        string fileName;
        ofVideoPlayer 		mMovie;
        void openMovie(string _fileName);
        ofxVideoRecorder    vidRecorder;
        void exportMovie();
        void exportFrames();

        bool exportFramesOn = false;
        string savePath;
        int frameNumber;

        cv::Mat myResize(cv::Mat src, float s);
        cv::Mat resized;

        ofImage ofIm;

        float moviePosY = 10.0;
        float moviePosX = 240.0;

        int effectNum;
        void setupEffect();
        void cleanEffect();
        void selectEffect(string effectName);

        int backgroundMode = -1;
        int blendLevel = 0;
        ofImage firstFrame;

        cv::BackgroundSubtractorMOG2 bgSubtractor;
        ofImage effectBgExtract (cv::Mat color_img) ;
        void effectGoodOpticalFlow(cv::Mat color_img);
        void effectTrackOpticalFlow(cv::Mat color_img);

        void effectFeatureLines(cv::Mat color_img);
        ofImage effectGradientMotion(cv::Mat rsz);

        cv::GoodFeaturesToTrackDetector goodFeatureDetector;
        std::vector<cv::Point2f> lastPointsOpticalFlow;
        std::vector<cv::KeyPoint> keyPointsOpticalFlow;

        cv::Mat lastImgOpticalFlow;
        bool firstFrameOpticalFlow = true;
        int MAX_FEATURES = 500;

        cv::SIFT *siftFinder;
        vector <cv::KeyPoint> siftFeatures;


        bool findInVector(vector<string> haystack, string needle);

        void effectSurfWaves(cv::Mat resized);

};
