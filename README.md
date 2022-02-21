# Camera surveillance with OF and OpenCv

Real-time streaming to interact with our environment.
Using CCTV cameras and combine the great technology available today.

secam is a small project about using live stream from IP cameras. 
With great libraries like openFrameworks, OpenCV and yolo is posible to create this Surveillance aproach in short time.

More info about OF [here](https://openframeworks.cc/)

## Target platforms
Raspberry pi with debian 11

comming soon

## Code explanation
comming soon


## Prerequisites

You will need the folowing libraries on your system:

- Any Linux OS PC, or a Raspberry pi 
- OpenCV 4.5.4+
- Python 3.7+ 
- GCC 7.0+ 
- Yolo 5+


## installation
step 1:
install compiler and  dependencies.

```bash
$ sudo apt update
$ sudo apt install build-essential

$ sudo apt install python3

```

step 2:
install OF and OpenCV
We need to get OF 0.11.2 from [here](https://openframeworks.cc/download/)
Download it and folow the installation instructions from the OF site.

OF will allready install OpenCV 4+ for you.
Make sure OF works. Compile and Run some OpenCV examples.

step 3:
install Yolo 5
https://github.com/ultralytics/yolov5

```bash
git clone https://github.com/ultralytics/yolov5.git
cd yolov5
pip install -r requirements.txt  # install
```

step 4:
clone secam

```bash

git clone https://github.com/yoosamui/secam.git
```



