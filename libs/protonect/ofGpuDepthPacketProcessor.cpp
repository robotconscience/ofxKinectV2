//
//  ofGpuDepthPacketProcessor.cpp
//
//  Created by Yuya Hanai on 2014/08/21.
//

#include "ofGpuDepthPacketProcessor.h"
#include "ofxOpenGLContextScope.h"

#include <libfreenect2/tables.h>

namespace libfreenect2{
    static ofxOpenGLContextScope* scope = NULL;
    
    ofGpuDepthPacketProcessor::ofGpuDepthPacketProcessor(){
        ofxOpenGLContextScope::setup();
        
        bOpen = false;
        startInvalidated = false;
        stopInvalidated = false;
        p0tableInvalidated = false;
        
        for(int i = 0; i < 3; ++i) {
            p0tablepx[i].allocate(512, 424, 1);
            p0tablepx[i].set(0);
        }
        ir_frame = NULL;
        depth_frame = NULL;
        newDepthFrame();
        newIrFrame();
        
        t_acc = 0;
        t_acc_n = 0;
        params_need_update = true;
    }
    
    ofGpuDepthPacketProcessor::~ofGpuDepthPacketProcessor(){
        
    }
    
    void ofGpuDepthPacketProcessor::updateShaderParameters(ofShader& program){
        if (!params_need_update) return;
        
        program.setUniform1f("Params.ab_multiplier", params.ab_multiplier);
        program.setUniform3fv("Params.ab_multiplier_per_frq", params.ab_multiplier_per_frq);
        program.setUniform1f("Params.ab_output_multiplier", params.ab_output_multiplier);
        
        program.setUniform3fv("Params.phase_in_rad", params.phase_in_rad);
        
        program.setUniform1f("Params.joint_bilateral_ab_threshold", params.joint_bilateral_ab_threshold);
        program.setUniform1f("Params.joint_bilateral_max_edge", params.joint_bilateral_max_edge);
        program.setUniform1f("Params.joint_bilateral_exp", params.joint_bilateral_exp);
        
        {
            GLint idx = glGetUniformLocation(program.getProgram(), "Params.gaussian_kernel");
            if(idx != -1) {
                glUniformMatrix3fv(idx, 1, false, params.gaussian_kernel);
            }
        }
        
        program.setUniform1f("Params.phase_offset", params.phase_offset);
        program.setUniform1f("Params.unambigious_dist", params.unambigious_dist);
        program.setUniform1f("Params.individual_ab_threshold", params.individual_ab_threshold);
        program.setUniform1f("Params.ab_threshold", params.ab_threshold);
        program.setUniform1f("Params.ab_confidence_slope", params.ab_confidence_slope);
        program.setUniform1f("Params.ab_confidence_offset", params.ab_confidence_offset);
        program.setUniform1f("Params.min_dealias_confidence", params.min_dealias_confidence);
        program.setUniform1f("Params.max_dealias_confidence", params.max_dealias_confidence);
        
        program.setUniform1f("Params.edge_ab_avg_min_value", params.edge_ab_avg_min_value);
        program.setUniform1f("Params.edge_ab_std_dev_threshold", params.edge_ab_std_dev_threshold);
        program.setUniform1f("Params.edge_close_delta_threshold", params.edge_close_delta_threshold);
        program.setUniform1f("Params.edge_far_delta_threshold", params.edge_far_delta_threshold);
        program.setUniform1f("Params.edge_max_delta_threshold", params.edge_max_delta_threshold);
        program.setUniform1f("Params.edge_avg_delta_threshold", params.edge_avg_delta_threshold);
        program.setUniform1f("Params.max_edge_count", params.max_edge_count);
        
        program.setUniform1f("Params.min_depth", params.min_depth);
        program.setUniform1f("Params.max_depth", params.max_depth);
    }

    
    void ofGpuDepthPacketProcessor::loadP0TablesFromCommandResponse(unsigned char* buffer, size_t buffer_length){
        // TODO: check known header fields (headersize, tablesize)
        p0tables* p0table = (p0tables*)buffer;
        
        if(buffer_length < sizeof(p0tables))
        {
            std::cerr << "[CpuDepthPacketProcessor::loadP0TablesFromCommandResponse] P0Table response too short!" << std::endl;
            return;
        }
        
        size_t n = 512 * 424;
        std::copy(p0table->p0table0, p0table->p0table0 + n, p0tablepx[0].getPixels());
        std::copy(p0table->p0table1, p0table->p0table1 + n, p0tablepx[1].getPixels());
        std::copy(p0table->p0table2, p0table->p0table2 + n, p0tablepx[2].getPixels());

        for(int i = 0; i < 3; ++i) {
            p0tablepx[i].mirror(true, false);
        }
        
        p0tableInvalidated = true;
    }
    
    void ofGpuDepthPacketProcessor::process(const libfreenect2::DepthPacket &packet){
        startTiming();
        
        if (startInvalidated) {
            scope = new ofxOpenGLContextScope();
            
            // load bins
            ofBuffer lut = ofBufferFromFile("11to16.bin");
            unsigned short* ushortBuffer = new unsigned short[2048];
            for (int i=0; i<2048; ++i) {
                short* ptr = &reinterpret_cast<short*>(lut.getBinaryBuffer())[i];
                int intval = *ptr;
                intval += 32768;
                ushortBuffer[i] = (unsigned short) intval;
            }
            
            lut11to16.loadData(ushortBuffer, 2048, 1, GL_LUMINANCE);
            lut11to16.setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
            
            delete [] ushortBuffer;
            
            x = ofBufferFromFile("xTable.bin");
            x_table.loadData(reinterpret_cast<float*>(x.getBinaryBuffer()), 512, 424, GL_LUMINANCE);
            x_table.setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
            
            z = ofBufferFromFile("zTable.bin");
            z_table.loadData(reinterpret_cast<float*>(z.getBinaryBuffer()), 512, 424, GL_LUMINANCE);
            z_table.setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
            
            input_data.allocate(352, 424 * 10, GL_R16);
            
            stage1_infrared.allocate(512, 424, GL_R32F);
            
            ofFbo::Settings settings;
            settings.width = 512;
            settings.height = 424;
            settings.numColorbuffers = 3;
            settings.internalformat = GL_RGB32F_ARB;
            stage1_fbo.allocate(settings);
            stage1_fbo.attachTexture(stage1_infrared, GL_R32F, 3);
            stage2_depth.allocate(512, 424, GL_R32F);
            
            stage1.load("", "shader/stage1.fs");
            stage2.load("", "shader/stage2.fs");
            
            quad.getVertices().resize(4);
            quad.getTexCoords().resize(4);
            quad.setMode(OF_PRIMITIVE_TRIANGLE_FAN);
            
            GLfloat px0 = 0;
            GLfloat py0 = 0;
            GLfloat px1 = 512;
            GLfloat py1 = 424;

            GLfloat tx0 = 0;
            GLfloat ty0 = 0;
            GLfloat tx1 = 512;
            GLfloat ty1 = 424;
            
            quad.getVertices()[0].set(px0, py0, 0);
            quad.getVertices()[1].set(px1, py0, 0);
            quad.getVertices()[2].set(px1, py1, 0);
            quad.getVertices()[3].set(px0, py1, 0);
            
            quad.getTexCoords()[0].set(tx0, ty0);
            quad.getTexCoords()[1].set(tx1, ty0);
            quad.getTexCoords()[2].set(tx1, ty1);
            quad.getTexCoords()[3].set(tx0, ty1);
            
            startInvalidated = false;
        }
        if (stopInvalidated) {
            if (scope) {
                delete scope;
                scope = NULL;
            }
            stopInvalidated = false;
        }
        if (p0tableInvalidated) {
            for(int i = 0; i < 3; ++i)
                p0table[i].loadData(p0tablepx[i]);
            
            p0tableInvalidated = false;
        }
        
        bool has_listener = this->listener_ != 0;
        
        ofShortPixels px;
        px.setFromExternalPixels(reinterpret_cast<unsigned short*>(packet.buffer), 352, 424 * 10, 1);
        input_data.loadData(px);
        
        // stage 1
        stage1_fbo.begin();
        stage1_fbo.activateAllDrawBuffers();
        stage1.begin();
        {
            updateShaderParameters(stage1);
            for(int i = 0; i < 3; ++i)
                p0table[i].bind();
            lut11to16.bind();
            input_data.bind();
            z_table.bind();
            
            stage1.setUniformTexture("P0Table0", p0table[0], p0table[0].texData.textureID);
            stage1.setUniformTexture("P0Table1", p0table[1], p0table[1].texData.textureID);
            stage1.setUniformTexture("P0Table2", p0table[2], p0table[2].texData.textureID);
            stage1.setUniformTexture("Lut11to16", lut11to16, lut11to16.texData.textureID);
            stage1.setUniformTexture("Data", input_data, input_data.texData.textureID);
            stage1.setUniformTexture("ZTable", z_table, z_table.texData.textureID);
            
            quad.draw();
            
            z_table.unbind();
            input_data.unbind();
            lut11to16.unbind();
            for(int i = 0; i < 3; ++i)
                p0table[i].unbind();
        }
        stage1.end();
        stage1_fbo.end();
        
        ofFloatPixels tmpIr;
        tmpIr.setFromExternalPixels((float*) ir_frame->data, ir_frame->width, ir_frame->height, 1);
        stage1_fbo.getTextureReference(3).readToPixels(tmpIr);
        tmpIr.mirror(true, false);
        
        // stage 2
        stage2_depth.begin();
        stage2.begin();
        {
            updateShaderParameters(stage2);
            x_table.bind();
            z_table.bind();
            stage1_fbo.getTextureReference(0).bind();
            stage1_fbo.getTextureReference(1).bind();
            stage2.setUniformTexture("A", stage1_fbo.getTextureReference(0), stage1_fbo.getTextureReference(0).texData.textureID);
            stage2.setUniformTexture("B", stage1_fbo.getTextureReference(1), stage1_fbo.getTextureReference(1).texData.textureID);
            stage2.setUniformTexture("XTable", x_table, x_table.texData.textureID);
            stage2.setUniformTexture("ZTable", z_table, z_table.texData.textureID);
            
            quad.draw();

            stage1_fbo.getTextureReference(0).unbind();
            stage1_fbo.getTextureReference(1).unbind();
            x_table.unbind();
            z_table.unbind();
        }
        stage2.end();
        stage2_depth.end();
        
        ofFloatPixels tmpDepth;
        tmpDepth.setFromExternalPixels((float*) depth_frame->data, depth_frame->width, depth_frame->height, 1);
        stage2_depth.getTextureReference().readToPixels(tmpDepth);
        tmpDepth.mirror(true, false);
        
        if(has_listener)
        {
            if(listener_->addNewFrame(Frame::Ir, ir_frame))
            {
                newIrFrame();
            }
            
            if(listener_->addNewFrame(Frame::Depth, depth_frame))
            {
                newDepthFrame();
            }
        }
        
        stopTiming();
        params_need_update = false;
    }
}
