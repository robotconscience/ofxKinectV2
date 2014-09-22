#version 150

uniform sampler2DRect kinectColor;

uniform float width;
uniform float height;

uniform float widthDepth;
uniform float heightDepth;

uniform float offsetLeft;
uniform float offsetRight;

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

// this comes from the vertex shader
in vec2 texCoordVarying;
in vec4 point_position;
in vec4 out_color;

// this is the output of the fragment shader
out vec4 outputColor;

void main()
{
    vec4 color          = out_color;
    vec2 texCoord       = vec2( texCoordVarying.st );
    texCoord.x          = ofMap(texCoord.x, 0.0, widthDepth, 0.0, width, true);
    texCoord.y          = ofMap(texCoord.y, 0.0, heightDepth, 0.0, height, true);
    vec4 texColor       = texture(kinectColor, texCoord);
    
    // could mix here
    
    if ( point_position.z < 50.0 ){
        discard;
    } else {
        outputColor = color;
    }
}