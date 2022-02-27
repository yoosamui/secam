#!/usr/bin/env bash

if [ $EUID != 0 ]; then
	echo "this script must be run as root"
	echo ""
	echo "usage:"
	echo "sudo "$0
	exit $exit_code
   exit 1
fi

OS_CODENAME=$(cat /etc/os-release | grep VERSION= | sed "s/VERSION\=\"\(.*\)\"/\1/")

if [[ "$OS_CODENAME" = "11 (bullseye)" ]]; then
   # sanity for stitching
   version=/usr/bin/opencv_version
   if [ -f $version ]; then
       version=$($version)
       # TODO check version if necessary.
       if [ ! -z $version ]; then
           defined=$(grep "#define HAVE_OPENCV_STITCHING" /usr/include/opencv4/opencv2/opencv_modules.hpp)
           if [[ ${defined:0:7} == $"#define" ]]; then
               sed -i 's/\<Status\>/EnumStatus/' /usr/include/opencv4/opencv2/stitching.hpp
               echo "OpenCV $version stitching updated."
           fi
       fi
   fi

   # sanity for openmaxil
   if [[ ! -f "/opt/vc/lib/libopenmaxil.so" ]]; then
           sed -i 's/\<PLATFORM_LIBRARIES += openmaxil\>//' ../../../libs/openFrameworksCompiled/project/linuxarmv6l/config.linuxarmv6l.default.mk
           echo "config.linuxarmv6l.default.mk (openmaxil) updated."
   fi
fi

