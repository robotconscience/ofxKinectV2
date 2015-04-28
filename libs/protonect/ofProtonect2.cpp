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
    
    auto ir = dev->getIrCameraParams();
    auto color = dev->getColorCameraParams();
    
    depthReg = new libfreenect2::Registration(&ir, &color );
    
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
        
        ofxCv::convertColor(rgb, rgb, CV_BGR2RGB);
        
//        rgbPixels.setFromPixels(rgb.ptr<unsigned char>(), rgb.cols, rgb.rows, rgb.channels());
        irPixels.setFromPixels(ir.ptr<float>(), ir.cols, ir.rows, ir.channels());
        depthPixels.setFromPixels(depth.ptr<float>(), depth.cols, depth.rows, depth.channels());
        
        // do depth reg
        if(bDoDepthReg)
        {   
            cv::Mat depthColor(424, 512, CV_8UC3);
            
            static bool bLogged = false;
            
            for (int x = 0; x < 512; x++){
                for (int y = 0; y < 424; y++) {
                    float cx;
                    float cy;
                    float z = depth.at<float>(y, x) * 4500.f;
                    if (z == 0)
                    {
                        depthColor.at<cv::Vec3b>(y, x) = cv::Vec3b(0, 0, 0);
                        continue;
                    }
                    depthReg->apply(x, y, z, cx, cy);
                    if (cx > 0 && cy > 0 && cx < 1920 && cy < 1080)
                        depthColor.at<cv::Vec3b>(y, x) = rgb.at<cv::Vec3b>(cy, cx);
//                    else
//                        depthColor.at<cv::Vec3b>(y, x) = cv::Vec3b(0, 0, 0);
                }
            }
            bLogged = true;
            rgbPixels.setFromPixels(depthColor.ptr<unsigned char>(), depthColor.cols, depthColor.rows, depthColor.channels());
            
        } else {
            rgbPixels.setFromPixels(rgb.ptr<unsigned char>(), rgb.cols, rgb.rows, rgb.channels());
        }
        
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