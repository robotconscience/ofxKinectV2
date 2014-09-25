//
//  ofProtonect2.cpp
//  example
//
//  Created by Brett Renfer on 9/25/14.
//  Copyright (c) 2014 __MyCompanyName__. All rights reserved.
//

#include "ofProtonect2.h"
#include "ofxCv.h"

ofProtonect::ofProtonect() :
dev(NULL),
bOpened(false),
depthReg(NULL),
bDepthRegStarted(false)
{
    freenect2 = new libfreenect2::Freenect2();
    ofAddListener(ofEvents().exit, this, &ofProtonect::exit);
    bDoDepthReg = false;
}

int ofProtonect::openKinect(std::string dataPath){
    if ( bOpened ){
        closeKinect();
    }
    dev = freenect2->openDefaultDevice();
    
    if(dev == 0)
    {
        std::cout << "no device connected or failure opening the default one!" << std::endl;
        return -1;
    }
    
    listener = new libfreenect2::SyncMultiFrameListener(libfreenect2::Frame::Color | libfreenect2::Frame::Ir | libfreenect2::Frame::Depth);
    
    dev->setColorFrameListener(listener);
    dev->setIrAndDepthFrameListener(listener);
    dev->start();
    
    std::cout << "device serial: " << dev->getSerialNumber() << std::endl;
    std::cout << "device firmware: " << dev->getFirmwareVersion() << std::endl;
    bOpened = true;
    
    return 0;
}


void ofProtonect::doDepthRegistration( bool bRegister ){
    bDoDepthReg = bRegister;
}

void ofProtonect::updateKinect(ofPixels & rgbPixels, ofFloatPixels & depthPixels, ofFloatPixels & irPixels){
    if ( bOpened ){
        listener->waitForNewFrame(frames);
        libfreenect2::Frame *rgb_frame = frames[libfreenect2::Frame::Color];
        libfreenect2::Frame *ir_frame = frames[libfreenect2::Frame::Ir];
        libfreenect2::Frame *depth_frame = frames[libfreenect2::Frame::Depth];
        
        // change the size of rgb image
        cv::Mat rgb = cv::Mat(rgb_frame->height, rgb_frame->width, CV_8UC3, rgb_frame->data);
        cv::Mat ir = cv::Mat(ir_frame->height, ir_frame->width, CV_32FC1, ir_frame->data) / 20000.0f;
        cv::Mat depth = cv::Mat(depth_frame->height, depth_frame->width, CV_32FC1, depth_frame->data) / 4500.0f;
        cv::Mat scaled = cv::Mat(depth_frame->height, depth_frame->width, CV_32FC1);
        
        // do depth reg
        if((depth.rows != rgb.rows || depth.cols != rgb.cols) && bDoDepthReg)
        {
            if(bDepthRegStarted == false)
            {
                depthReg = DepthRegistration::New(cv::Size(rgb.cols, rgb.rows),
                                                  cv::Size(depth.cols, depth.rows),
                                                  cv::Size(depth.cols, depth.rows),
                                                  0.5f, 20.0f, 0.015f, DepthRegistration::CPU);
                
                cv::Mat cameraMatrixColor, cameraMatrixDepth;
                cameraMatrixColor = cv::Mat::zeros(3, 3, CV_64F);
                cameraMatrixDepth = cv::Mat::zeros(3, 3, CV_64F);
                depthReg->ReadDefaultCameraInfo(cameraMatrixColor, cameraMatrixDepth);
                
                depthReg->init(cameraMatrixColor, cameraMatrixDepth,
                               cv::Mat::eye(3, 3, CV_64F), cv::Mat::zeros(1, 3, CV_64F),
                               cv::Mat::zeros(depth.rows, depth.cols, CV_32F),
                               cv::Mat::zeros(depth.rows, depth.cols, CV_32F));
                
                bDepthRegStarted=true;
            }
            
            depthReg->depthToRGBResolution(depth, scaled);
        }
        else
        {
            scaled = depth;
        }
        
        ofxCv::convertColor(rgb, rgb, CV_BGR2RGB);
        
        /*cv::Size size(640,480);//the dst image size,e.g.100x100
         resize(rgb,rgb,size);//resize image
         resize(ir,ir,size);//resize image
         resize(depth,depth,size);//resize image
         resize(scaled,scaled,size);//resize image*/
        
        rgbPixels.setFromPixels(rgb.ptr<unsigned char>(), rgb.cols, rgb.rows, rgb.channels());
        depthPixels.setFromPixels(scaled.ptr<float>(), scaled.cols, scaled.rows, scaled.channels());
        irPixels.setFromPixels(ir.ptr<float>(), ir.cols, ir.rows, ir.channels());
        listener->release(frames);
    }
}

int ofProtonect::closeKinect(){
    if ( bOpened ){
        bOpened = false;
        // TODO: restarting ir stream doesn't work!
        // TODO: bad things will happen, if frame listeners are freed before dev->stop() :(
        dev->stop();
        dev->close();
    }
}

void ofProtonect::exit(ofEventArgs & args){
    // this is kind of fucked ATM
    if ( bOpened ){
        closeKinect();
    }
    delete depthReg;
    delete listener;
}