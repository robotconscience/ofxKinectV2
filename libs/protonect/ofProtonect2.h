//
//  ofProtonect2.h
//  example
//
//  Created by Brett Renfer on 9/25/14.
//  Copyright (c) 2014 __MyCompanyName__. All rights reserved.
//

#pragma once

#include "ofMain.h"

#include <libfreenect2/libfreenect2.hpp>
#include <libfreenect2/frame_listener_impl.h>
#include <libfreenect2/threading.h>

#include <libfreenect2/depth_registration.h>

class ofProtonect
{
public:
    
    ofProtonect();
    
    int openKinect(std::string dataPath);
    
    void updateKinect(ofPixels & rgbPixels, ofFloatPixels & depthPixels, ofFloatPixels & irPixels);
    int closeKinect();
    void exit(ofEventArgs & args);
    
    void doDepthRegistration( bool bRegister=true );
    
protected:
    
    libfreenect2::Freenect2 * freenect2;
    libfreenect2::Freenect2Device *dev;
    libfreenect2::SyncMultiFrameListener * listener;
    libfreenect2::FrameMap frames;
    
    bool bOpened;
    bool bDoDepthReg;
    
    DepthRegistration *depthReg;
    bool bDepthRegStarted;
};

