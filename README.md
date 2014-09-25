ofxKinectV2
===========

An addon for the new Kinect For Windows V2 sensor. 
Based on the excellent work by the https://github.com/OpenKinect/libfreenect2 team ( @JoshBlake @floe and @christiankerl plus others ) 

New: 
- Now supports both the early beta device and the new retail device. 


Notes:
- Build in Release for much faster performance ( 15fps capture rate in Release vs 5fps in Debug )!!
- Currently only supports one Kinect V2 at a time. 
- Kinect needs to have flashed firmware ( this currently needs to be done on Windows 8 ) 
- Requires USB 3 port on machine. 
- Only tested on OS X though Win / Nix should be possible too with patched libusb ( see: https://github.com/OpenKinect/libfreenect2/blob/master/depends/README.depends.txt ) 
- If you have the ofxKinect ( v1 ) addon in your project remove the ofxKinect libusb lib and use the one that comes with this repo instead. 
- Depth decoding is not at 30fps as it is decoded on the CPU - GPU decoding is possible, but not stable currently with OF. 
- Note: GPU decoding by @hanasaan requires https://github.com/satoruhiga/ofxOpenGLContextScope


Huge thanks to @christiankerl for a lot of the recent changes that made this work well on OS X. 

OPENCL notes
===========
* OpenCL only tested on OS X (sorry)
* to get OpenCL working, right now you must install dependencies via this branch of libfreenect2: https://github.com/dorian3d/libfreenect2
``` cd /anywhere
    git clone https://github.com/dorian3d/libfreenect2.git
    cd libfreenect2
    sh depends/install_mac.sh
```
* this will patch your OpenCL and install eigen3
* that should be it... if it doesn't work, you may have to run through the entire libfreenect2 build process....