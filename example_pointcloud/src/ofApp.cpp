#include "ofApp.h"

bool bCreatedPoints = false;
bool bUseShader = true;
bool bResetZ = true;


//--------------------------------------------------------------
void ofApp::setup(){
    if( !ofFile::doesFileExist("11to16.bin") ){
        ofSystemAlertDialog("Make sure you have 11to16.bin, xTable.bin and zTable.bin in your data folder!");
        ofExit(); 
    }

    ofBackground(30, 30, 30);

    extrude.set("extrudeDepth", 500.0, 0.0, 1000.0);
    depthLeft.set("depthLeft",0.0,0.0,1.0);
    depthRight.set("depthRight",1.0, 0.0, 1.0);
    
    panel.setup("distance in mm", "settings.xml", 540, 100);
    panel.add(kinect.minDistance);
    panel.add(kinect.maxDistance);
    panel.add(extrude);
//    panel.add(depthLeft);
//    panel.add(depthRight);
    panel.loadFromFile("settings.xml");

    kinect.open();
    
    pointCloudShader.load("shaders/pointCloud");
    
    camera.setPosition(ofGetWidth()/2.0, ofGetHeight()/2.0, 700.0);
    camera.roll(180.0);
}

//--------------------------------------------------------------
void ofApp::update(){
    kinect.update();
    if( kinect.isFrameNew() ){
        texDepth.loadData( kinect.getDepthPixels() );
        texRGB.loadData( kinect.getRgbPixels() );
        
        int width = kinect.getDepthPixels().getWidth();
        int height = kinect.getDepthPixels().getHeight();
        
        int widthColor = texRGB.getWidth();
        int heightColor = texRGB.getHeight();
        
        if ( !bCreatedPoints && kinect.getDepthPixels().getWidth() != 0){
            bCreatedPoints = true;
            // add verts + texcoords
            
            for ( int x=0; x<width; x++){
                for ( int y=0; y<height; y++ ){
                    pointCloud.addVertex(ofVec3f(x,y,0));
                    pointCloud.addTexCoord(ofVec2f(x,y));
                }
            }
            
            // taken from @elliotwoods' addon
			auto vertices = pointCloud.getVerticesPointer();
            
            for(int i=0; i<width-1; i++) {
                for(int j=0; j<height-1; j++) {
                    auto topLeft = width * j + i;
                    auto topRight = topLeft + 1;
                    auto bottomLeft = topLeft + width;
                    auto bottomRight = bottomLeft + 1;
                    
                    //upper left triangle
                    if (vertices[topLeft].z > 0 && vertices[topRight].z > 0 && vertices[bottomLeft].z > 0) {
                        const ofIndexType indices[3] = {topLeft, bottomLeft, topRight};
                        pointCloud.addIndices(indices, 3);
                    }
                    
                    //bottom right triangle
                    if (vertices[topRight].z > 0 && vertices[bottomRight].z > 0 && vertices[bottomLeft].z > 0) {
                        const ofIndexType indices[3] = {topRight, bottomRight, bottomLeft};
                        pointCloud.addIndices(indices, 3);
                    }
                }
            }
        }
        
        if ( !bUseShader ){
            for ( auto & v : pointCloud.getVertices() ){
                v.z = kinect.getRawDepthPixels().getColor(v.x, kinect.getDepthPixels().getHeight() - v.y ).r;
            }
            
        } else if ( !bResetZ ){
            bResetZ = true;
            for ( auto & v : pointCloud.getVertices() ){
                v.z = 0.0;
            }
        }
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofDrawBitmapString("ofxKinectV2: Work in progress addon.\nBased on the excellent work by the OpenKinect libfreenect2 team\n\n-Only supports one Kinect v2 at a time. \n-Requires USB 3.0 port ( superspeed )\n-Requires patched libusb. If you have the libusb from ofxKinect ( v1 ) linked to your project it will prevent superspeed on Kinect V2", 10, 14);
    
    //texDepth.draw(10, 100);
    //texRGB.draw(10, 110 + texDepth.getHeight(), 1920/4, 1080/4);
    
    panel.draw();
    
    ofPushMatrix();
    ofEnableDepthTest();
    //ofTranslate(ofGetWidth()/2.0, ofGetHeight()/2.0);
    camera.begin();
    ofTranslate(-kinect.getDepthPixels().getWidth()/2.0, -kinect.getDepthPixels().getHeight()/2.0);
    
    if ( pointCloud.getNumVertices() != 0 ){
        if (bUseShader ){
            
            pointCloudShader.begin();
            pointCloudShader.setUniformTexture("kinectColor", texRGB, 0);
            pointCloudShader.setUniformTexture("kinectDepth", texDepth, 1);
            pointCloudShader.setUniform1f("depth", extrude);
            pointCloudShader.setUniform1f("depthRealMax", kinect.maxDistance);
            pointCloudShader.setUniform1f("depthRealMin", kinect.minDistance);
            pointCloudShader.setUniform1f("width", texRGB.getWidth());
            pointCloudShader.setUniform1f("height", texRGB.getHeight());
            
            pointCloudShader.setUniform1f("widthDepth", texDepth.getWidth());
            pointCloudShader.setUniform1f("heightDepth", texDepth.getHeight());
            
            pointCloudShader.setUniform1f("offsetLeft", (float) texRGB.getWidth() * depthLeft);
            pointCloudShader.setUniform1f("offsetRight", (float) texRGB.getWidth() * depthRight);
            
            pointCloud.drawVertices();
            pointCloudShader.end();
        } else {
            //texRGB.bind();
            pointCloud.drawWireframe();
            //texRGB.unbind();
        }
    }
    camera.end();
    ofDisableDepthTest();
    ofPopMatrix();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    if ( key == 'r' ){
        pointCloudShader.load("shaders/pointCloud");
    } else if ( key == 's' ){
        bUseShader = !bUseShader;
    } else if ( key == 'c' ){
        cout << camera.getPosition() << endl;
    }
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
