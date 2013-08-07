#include "testApp.h"

using namespace ofxCv;
using namespace cv;

void testApp::setup() {
    
	ofSetVerticalSync(true);
	ofBackground(0);
	
	camWidth 		= 640;	// try to grab at this size.
	camHeight 		= 480;
	
	vidGrabber.setVerbose(true);
	vidGrabber.setDeviceID(1);
	vidGrabber.setDesiredFrameRate(120);
	vidGrabber.initGrabber(camWidth,camHeight);
	
	// wait for half a frame before forgetting something
    // should be half of framerate?
	contourFinder.getTracker().setPersistence(60);
	// an object can move up to 32 pixels per frame
	contourFinder.getTracker().setMaximumDistance(32);
	
	showLabels = true;
    
    diff.allocate(camWidth, camHeight, OF_IMAGE_COLOR);
    bg.allocate(camWidth, camHeight, OF_IMAGE_COLOR);
    grey.allocate(camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    learnBg = false;
    once = true;
    thresholdValue = 200;
    
    float dim = 16;
	float xInit = 30;
    float length = 320-xInit;
    gui = new ofxUICanvas(camWidth, 0, 320, 320);
    ofAddListener(gui->newGUIEvent, this, &testApp::guiEvent);
    gui->addSlider("threshold ", 0.0, 255.0, &thresholdValue, length, dim);
    gui->addSlider("blur", 0.0, 255.0, &blurAmount, length, dim);
    gui->addSlider("contour min radius", 0.0, 255.0, &cfMinRadius, length, dim);
    gui->addSlider("contour max radius", 0.0, 255.0, &cfMaxRadius, length, dim);
    gui->addSlider("contour threshold", 0.0, 255.0, &cfTreshold, length, dim);
    gui->loadSettings("GUI/guiSettings.xml");
    gui->setTheme(OFX_UI_THEME_GRAYRED);
    
    sender.setup(HOST, PORT);
    
}

void testApp::update() {

	contourFinder.setMinAreaRadius(cfMinRadius);
	contourFinder.setMaxAreaRadius(cfMaxRadius);
	contourFinder.setThreshold(cfTreshold);

	vidGrabber.update();
	
	if (vidGrabber.isFrameNew()){
        absdiff(bg, vidGrabber, diff);
		blur(diff, blurAmount);
        convertColor(diff, grey, CV_RGB2GRAY);
        invert(grey, grey);
        threshold(grey, thresholdValue, true);
        diff.update();
        grey.update();
		contourFinder.findContours(grey);
	}
    
    if(learnBg || once){
        bg.setFromPixels(vidGrabber.getPixels(), vidGrabber.getWidth(), vidGrabber.getHeight(), OF_IMAGE_COLOR);
        learnBg = false;
        once = false;
    }

    ofxOscMessage mNumContours;
    mNumContours.setAddress("/numContours");
    mNumContours.addIntArg(contourFinder.size());
    sender.sendMessage(mNumContours);
    
    ofxOscMessage mMode;
    mMode.setAddress("/mode");
    if(contourFinder.size() == 0){
        mMode.addStringArg("nobody");
    } else if(contourFinder.size() == 1){
        mMode.addStringArg("1 person");
    } else if(contourFinder.size() > 1){
        mMode.addStringArg("panic");
    }
    sender.sendMessage(mMode);
    
    if(contourFinder.size() >= 1){
        ofxOscMessage mTest;
        ofPoint center = toOf(contourFinder.getCentroid(0));
        float test = ofMap(center.x, 0, camWidth, 0, 1);
        cout << test << endl;
        mTest.setAddress("/activeclip/video/position/values");
        mTest.addFloatArg(test);
        sender.sendMessage(mTest);
    }

    ofxOscMessage mPos;
    if(contourFinder.size() >= 1){
        ofPoint center = toOf(contourFinder.getCentroid(0));
        mPos.addFloatArg(center.x);
        mPos.addFloatArg(center.y);
        sender.sendMessage(mPos);
    }
    
}

void testApp::draw() {
	ofSetBackgroundAuto(showLabels);
	RectTracker& tracker = contourFinder.getTracker();
	
	if(showLabels) {
        
		ofSetColor(255);
		grey.draw(0, 0);

        ofNoFill();
		ofSetColor(255, 0, 0);
        ofRect(0, 0, camWidth, camHeight);
		contourFinder.draw();
        ofFill();

        // draw cam
		ofSetColor(255);
        ofPushMatrix();
        ofTranslate(camWidth/2, camHeight, 0);
        ofScale(-1, 1);
		vidGrabber.draw(0, 0, camWidth/2, camHeight/2);
        ofPopMatrix();
        ofSetColor(255, 0, 0);
        ofDrawBitmapString("cam", 0, camHeight + 10);
        
        // draw bg
		ofSetColor(255);
        ofPushMatrix();
        ofTranslate(camWidth, camHeight, 0);
        ofScale(-1, 1);
        bg.draw(0, 0, camWidth/2, camHeight/2);
        ofPopMatrix();
        ofSetColor(255, 0, 0);
        ofDrawBitmapString("bg", camWidth/2, camHeight + 10);
        
        // draw diff
        ofSetColor(255);
        diff.draw(camWidth, camHeight, camWidth/2, camHeight/2);
        ofSetColor(255, 0, 0);
        ofDrawBitmapString("diff", camWidth, camHeight + 10);
        
        ofSetColor(255);
		for(int i = 0; i < contourFinder.size(); i++) {
			ofPoint center = toOf(contourFinder.getCenter(i));
			ofPushMatrix();
			ofTranslate(center.x, center.y);
			int label = contourFinder.getLabel(i);
			string msg = ofToString(label) + ":" + ofToString(tracker.getAge(label));
			ofDrawBitmapString(msg, 0, 0);
			ofVec2f velocity = toOf(contourFinder.getVelocity(i));
			ofScale(5, 5);
			ofLine(0, 0, velocity.x, velocity.y);
			ofPopMatrix();
		}
	} else {
		for(int i = 0; i < contourFinder.size(); i++) {
			unsigned int label = contourFinder.getLabel(i);
			// only draw a line if this is not a new label
			if(tracker.existsPrevious(label)) {
				// use the label to pick a random color
				ofSeedRandom(label << 24);
				ofSetColor(ofColor::fromHsb(ofRandom(255), 255, 255));
				// get the tracked object (cv::Rect) at current and previous position
				const cv::Rect& previous = tracker.getPrevious(label);
				const cv::Rect& current = tracker.getCurrent(label);
				// get the centers of the rectangles
				ofVec2f previousPosition(previous.x + previous.width / 2, previous.y + previous.height / 2);
				ofVec2f currentPosition(current.x + current.width / 2, current.y + current.height / 2);
				ofLine(previousPosition, currentPosition);
			}
		}
	}
	
	// this chunk of code visualizes the creation and destruction of labels
	const vector<unsigned int>& currentLabels = tracker.getCurrentLabels();
	const vector<unsigned int>& previousLabels = tracker.getPreviousLabels();
	const vector<unsigned int>& newLabels = tracker.getNewLabels();
	const vector<unsigned int>& deadLabels = tracker.getDeadLabels();
	ofSetColor(cyanPrint);
	for(int i = 0; i < currentLabels.size(); i++) {
		int j = currentLabels[i];
		ofLine(j, 0, j, 4);
	}
	ofSetColor(magentaPrint);
	for(int i = 0; i < previousLabels.size(); i++) {
		int j = previousLabels[i];
		ofLine(j, 4, j, 8);
	}
	ofSetColor(yellowPrint);
	for(int i = 0; i < newLabels.size(); i++) {
		int j = newLabels[i];
		ofLine(j, 8, j, 12);
	}
	ofSetColor(ofColor::white);
	for(int i = 0; i < deadLabels.size(); i++) {
		int j = deadLabels[i];
		ofLine(j, 12, j, 16);
	}
    
    if(contourFinder.size() >= 1){
        ofPoint center = toOf(contourFinder.getCentroid(0));
        ofSetColor(255, 0, 0);
        ofCircle(center, 10);
    }

	string buf;
	buf = "sending osc messages to " + string(HOST) + " " + ofToString(PORT) + "\n" + "press 'b' to set background";
    ofSetColor(255, 0, 0);
	ofDrawBitmapString(buf, 10, 20);

}

void testApp::keyPressed(int key) {
	if(key == ' ') {
		showLabels = !showLabels;
	}
	if(key == 'b') {
        learnBg = true;
	}
}

void testApp::exit(){
    gui->saveSettings("GUI/guiSettings.xml");
    delete gui;
}

void testApp::guiEvent(ofxUIEventArgs &e){
	
}