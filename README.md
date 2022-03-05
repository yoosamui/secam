# Camera surveillance with OF and OpenCv

Real-time streaming Using CCTV.

secam is a small project about using live stream from IP cameras. 
With great libraries like openFrameworks, OpenCV and yolo is posible to create this Surveillance aproach in short time.

More info about OF [here](https://openframeworks.cc/)

![live stream from gate camera](https://github.com/yoosamui/secam/blob/main/motion/bin/data/images/gate.png)

## Target platforms
Android   (comming soon)
Mac OS    (comming soon)
Windows   (comming soon)
Linux
Raspberry pi

## Code explanation
comming soon


## Prerequisites

You will need the folowing libraries on your system:

- Any Linux OS PC, and/or a Raspberry pi 
- OpenFrameworks Version 0.11.2 (for amd64)
- OpenFrameworks Version 0.11.2 (for arm)

## installation
step 1:
install OpenCV 4.5.1+

```bash
https://docs.opencv.org/4.5.1/d7/d9f/tutorial_linux_install.html
```

step 2:
install OF and secam

```bash
We need to get OF 0.11.2 from [here](https://openframeworks.cc/download/)
For linux downloaf
Download it and folow the installation instructions from the OF site.

For linux OS 64 Bits
of_v0.11.2_linux64gcc6_release.tar.gz

For Raspberry PI OS 32 Bits
of_v0.11.2_linuxarmv6l_release.tar.gz

extract the OF archive in. (e.g asumme /home/user/develop) as the OF target directory.

cd to the OF framework apps directory:
cd /home/user/develop/of_v0.11.2_linuxXXX_release/apps

clone this repo inside the apps folder.
git clone https://github.com/yoosamui/secam.git

copy the sanity.sh script to OF;
cd /home/user/develop/of_v0.11.2_linuxXXX_release/scripts/linux/debian
cp /home/user/develop/of_v0.11.2_linuxXXX_release/apps/secam/motion/data/sanity.sh .

chmod a+x sanity.sh

now install the dependencies and compile OF.

sudo ./install_dependencies.sh  (this can take a while...)
sudo ./install_codecs.sh

after finish and no errors, execute the sanity script.

sudo ./sanity.sh

now compile OF.

cd ..
./compileOF.sh -j4

if no errors the installation for a Linux Desltop is done.

```
step 3:
Install Service and Configure

secam motion client/server needs a camera config file.
Create a config file in the folowing folder:

```bash
cp /home/user/develop/of_v0.11.2_linuxXXX_release/apps/secam/motion/bin/data
nano cam1.cfg

paste this content and modify the elements.

<config>

<!-- use a substream with medium resolution e.g 640x360 or 640x460 gives you accurate videos at 25 fps and medium CPU usage -->
<uri>rtsp://user:password@my.camera.com:554/Streaming/channels/102</uri>

<!-- enter the timezone where the camera is located -->
<timezone>Asia/Bangkok</timezone>

<!-- enter the storage location for the videos -->
<storage>/media/share/cameras/</storage>


</config
```
set your camera frame rate at 25 fps.

Make sure that the url works. you can check it with VLC or ffplay.

```bash
You can now start motion

cd /home/user/develop/of_v0.11.2_linuxXXX_release/apps/secam/motion/bin/

./motion -c=cam1

```

create a mask
press 1 to change the view in mask edit mode.
![create a mask for the gate camera](https://github.com/yoosamui/secam/blob/main/motion/bin/data/images/gate_mask_1.png)

Press F5 to start create a new mask.
select the polygon points with left mouse click and finish/close the polygon with right mouse click.
![shows the mask for the gate camera](https://github.com/yoosamui/secam/blob/main/motion/bin/data/images/gate_mask_2.png)

This will save the mask in the config file.


step 4:
Install systemctl server for the raspberry pi

```bash
cp /home/user/develop/of_v0.11.2_linuxXXX_release/apps/secam/motion/bin/data/server
sudo cp secam-motion.service /etc/systemd/system
sudo chmod a+x /etc/systemd/system/secam-motion.service

sudo nano /etc/systemd/system/secam-motion.service

modify the folowing lines.
ExecStart=/media/yoo/develop/of_v0.11.2_linux64gcc6_release/apps/secam/motion/bin/motion -c=gate -m=1
WorkingDirectory=/media/yoo/develop/of_v0.11.2_linux64gcc6_release/apps/secam/motion/bin

change the directories:

ExecStart=/home/user/develop/of_v0.11.2_linuxXXX_release/apps/secam/motion/bin/motion -c=cam1 -m=1
WorkingDirectory=/home/user/develop/of_v0.11.2_linuxXXX_release/apps/secam/motion/bin

set the user e.g pi
User=pi

save it.

start the secam service

sudo systemctl start secam-motion.service
systemctl status secam-motion.service

enable the service for autostart
sudo chmod 664 /etc/systemd/system/secam-motion.service
sudo systemctl daemon-reload
sudo systemctl enable secam-motion.service 

reboot your RPI

```

in development...
