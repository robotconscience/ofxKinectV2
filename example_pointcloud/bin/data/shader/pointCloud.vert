#version 120
#extension GL_EXT_gpu_shader4 : enable
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect kinectDepth;

uniform float depth;
uniform float depthRealMin;
uniform float depthRealMax;

float ofMap(float value, float inputMin, float inputMax, float outputMin, float outputMax, bool bClamp) {
    float outVal = ((value - inputMin) / (inputMax - inputMin) * (outputMax - outputMin) + outputMin);
    
    if( bClamp ){
        if(outputMax < outputMin){
            if( outVal < outputMax )outVal = outputMax;
            else if( outVal > outputMin )outVal = outputMin;
        }else{
            if( outVal > outputMax )outVal = outputMax;
            else if( outVal < outputMin )outVal = outputMin;
        }
    }
    return outVal;
}


varying vec4 position;

void main() {
    gl_TexCoord[0] = gl_MultiTexCoord0;
    position = gl_Vertex;
    float z = ofMap(texture2DRect(kinectDepth, gl_TexCoord[0].st).x, 0.0, 1.0, 0.0, depth, true );
    position.z = z;
    
    gl_Position = gl_ModelViewProjectionMatrix * position;
    gl_FrontColor = gl_Color;
}