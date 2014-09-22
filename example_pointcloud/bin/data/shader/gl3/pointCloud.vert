#version 150

// these come from the programmable pipeline
uniform mat4 modelViewProjectionMatrix;

in vec4 position;
in vec2 texcoord;
in vec4 color;
out vec4 out_color;

// texture coordinates are sent to fragment shader
out vec2 texCoordVarying;

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


out vec4 point_position;

void main() {
    texCoordVarying = texcoord;
	point_position = position;
    float z = ofMap(texture(kinectDepth, texCoordVarying.st).x, 0.0, 1.0, 0.0, depth, true );
    point_position.z = z;
    
    gl_Position = modelViewProjectionMatrix * point_position;
    out_color = color;
}