#pragma once

#include "ofMain.h"
#include "ofxCv.h"
#include "ofxUI.h"
#include "ofxOsc.h"

#define HOST "localhost"
#define PORT 12345

class testApp : public ofBaseApp {
public:
	void setup();
	void update();
	void draw();
	void keyPressed(int key);
	
	ofxCv::ContourFinder contourFinder;
	bool showLabels;
    ofVideoGrabber 		vidGrabber;
    int 				camWidth;
    int 				camHeight;
    float cfMinRadius, cfMaxRadius, cfTreshold;
    float blurAmount, thresholdValue;
    
    ofImage diff;
    ofImage bg;
    ofImage grey;
    bool learnBg;
    bool once;
    
    void exit();
    void guiEvent(ofxUIEventArgs &e);
    ofxUICanvas *gui;
    
    ofxOscSender sender;

};
