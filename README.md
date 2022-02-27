# Camera surveillance with OF and OpenCv

Real-time streaming to interact with our environment.
Using CCTV cameras and combine the great technology available today.

secam is a small project about using live stream from IP cameras. 
With great libraries like openFrameworks, OpenCV and yolo is posible to create this Surveillance aproach in short time.

More info about OF [here](https://openframeworks.cc/)

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
install compiler and  dependencies.

```bash
$ sudo apt update
$ sudo apt install build-essential

```

step 2:
install OF, OpenCV and secam

```bash
We need to get OF 0.11.2 from [here](https://openframeworks.cc/download/)
For linux downloaf
Download it and folow the installation instructions from the OF site.

For linux OS 64 Bits
of_v0.11.2_linux64gcc6_release.tar.gz

For Raspberry PI OS 32 Bits
of_v0.11.2_linuxarmv6l_release.tar.gz

extract the Downloaded OF archive in. (e.g asumme /home/user/develop) as the OF target directory.

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

paste this content and modify the uri.

<config>
<!--- EXAMPLE
<uri>rtsp://USERNAME:PASSWORD@IP:PORT/LINK_TO_STREAM</uri>
---->

<uri>rtsp://admin:12345@my.camera.com:554/Streaming/channels/102</uri>


</config
```


Make sure that the url works. you can check it with VLC or ffplay.

```bash
You can now start motion

cd /home/user/develop/of_v0.11.2_linuxXXX_release/apps/secam/motion/bin/

./motion -c=cam1


```

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

save it.

start the secam service

sudo systemctl start secam-motion.service
systemctl status secam-motion.service









```
