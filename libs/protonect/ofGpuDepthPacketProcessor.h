//
//  ofGpuDepthPacketProcessor.h
//
//  Created by Yuya Hanai on 2014/08/21.
//

#pragma once

#include "ofMain.h"

#include <libfreenect2/depth_packet_processor.h>
#include <libfreenect2/frame_listener.hpp>

namespace libfreenect2{
    
    class ofGpuDepthPacketProcessor : public DepthPacketProcessor{
        
    public:
        ofGpuDepthPacketProcessor();
        ~ofGpuDepthPacketProcessor();
        
        void start() {
            if (!bOpen) {
                startInvalidated = true;
                bOpen = true;
            }
        }
        
        void stop() {
            if (bOpen) {
                stopInvalidated = true;
                bOpen = false;
            }
        }
        
        void process(const libfreenect2::DepthPacket &packet);
        void loadP0TablesFromCommandResponse(unsigned char* buffer, size_t buffer_length);

    private:
        void newIrFrame()
        {
            ir_frame = new Frame(512, 424, 4);
        }
        
        void newDepthFrame()
        {
            depth_frame = new Frame(512, 424, 4);
        }
        
        void startTiming()
        {
            ts = ofGetElapsedTimeMicros();
        }
        
        void stopTiming()
        {
            t_acc += ofGetElapsedTimeMicros() - ts;
            t_acc_n++;
            
            if (t_acc_n > 100) {
                float avg = (float)(t_acc) / t_acc_n / 1000000.0f;
                std::cout << "[ofGpuDepthPacketProcessor] avg. time: " << (avg * 1000) << "ms -> ~" << (1.0/avg) << "Hz" << std::endl;
                t_acc = 0;
                t_acc_n = 0;
            }
        }
        
        void updateShaderParameters(ofShader& shader);
        
        ofBuffer x;
        ofBuffer z;
        ofBuffer lut;
        
        ofShortPixels p0tablepx[3];
        
        
        ofTexture lut11to16;
        ofTexture p0table[3];
        ofTexture x_table;
        ofTexture z_table;
        
        ofTexture input_data;
        
        ofTexture stage1_infrared;
        ofFbo stage1_fbo;
        
        ofFbo stage2_depth;
        
        ofMesh quad;
        
        ofShader stage1, stage2;
        
        Frame* depth_frame;
        Frame* ir_frame;
        
        uint64_t ts;
        uint64_t t_acc;
        int t_acc_n;
        
        bool bOpen;
        bool startInvalidated;
        bool stopInvalidated;
        bool p0tableInvalidated;
        bool params_need_update;
        
        DepthPacketProcessor::Parameters params;
    };
    
};