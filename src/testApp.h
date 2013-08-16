#pragma once

#include "ofMain.h"
#include "ofxCv.h"
#include "ofxUI.h"
#include "ofxOsc.h"


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
    int vidDeviceId;
    float vidFrameRate;

    float cfMinRadius, cfMaxRadius, cfTreshold;
    float blurAmount, thresholdValue;
    float xPosNormalized;
    
    ofImage vidMirrored;
    ofImage diff;
    ofImage bg;
    ofImage grey;
    bool learnBg;
    bool once;
    
    void exit();
    void guiEvent(ofxUIEventArgs &e);
    ofxUICanvas *gui;
    
    void readConfig();
    void connect();
    void disconnect();
    
    ofxOscSender sender;
    vector<float> vals;
    float avg, avgSize;
    float timer;
    float maxTimer;
    float timerDiff;
    
    string host;
    int port;
    bool bConnected = false;

};
