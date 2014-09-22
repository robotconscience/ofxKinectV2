#version 120
#extension GL_ARB_texture_rectangle: enable

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

varying vec4 position;

void main()
{
    vec4 color  = gl_Color;
    vec2 texCoord       = vec2( gl_TexCoord[0].st );
    texCoord.x          = ofMap(texCoord.x, 0.0, widthDepth, 0.0, width, true);
    texCoord.y          = ofMap(texCoord.y, 0.0, heightDepth, 0.0, height, true);
    vec4 texColor       = texture2DRect(kinectColor, texCoord);
    
    // could mix here
    
    if ( position.z < 50.0 ){
        discard;
    } else {
        gl_FragColor = color;
    }
}