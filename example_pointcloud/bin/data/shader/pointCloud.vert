#version 120
#extension GL_EXT_gpu_shader4 : enable
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect kinectDepth;

uniform float depth;
uniform float depthRealMin;
uniform float depthRealMax;

float rawToCentimeters(float raw) {
    float k1 = 0.1236, k2 = 2842.5, k3 = 1.1863, k4 = 0.0370;
    return 100 * (k1 * tan((raw / k2) + k3) - k4);
}

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
    float z = texture2DRect(kinectDepth, gl_TexCoord[0].st).x * depth * 10;
    position.z = z;
    
    gl_Position = gl_ModelViewProjectionMatrix * position;
    gl_FrontColor = gl_Color;
}