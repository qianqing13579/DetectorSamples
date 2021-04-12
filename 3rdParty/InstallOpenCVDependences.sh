#! /bin/sh

# Ubuntu
# sudo apt-get install build-essential -y # 安装各种开发工具，比如g++,libstdc++等
# sudo apt-get install cmake git libgtk2.0-dev pkg-config libavcodec-dev libavformat-dev libswscale-dev -y
# sudo apt-get install python-dev python-numpy libtbb2 libtbb-dev libjpeg-dev libpng-dev libtiff-dev libjasper-dev libdc1394-22-dev -y # 处理图像所需的包，可选
# sudo apt-get install libavcodec-dev libavformat-dev libswscale-dev libv4l-dev liblapacke-dev -y
# sudo apt-get install libxvidcore-dev libx264-dev -y # 处理视频所需的包
# sudo apt-get install libatlas-base-dev gfortran -y # 优化opencv功能
# sudo apt-get install ffmpeg -y

# CentOS
yum install gcc gcc-c++ gtk2-devel gimp-devel gimp-devel-tools gimp-help-browser zlib-devel libtiff-devel libjpeg-devel libpng-devel gstreamer-devel libavc1394-devel libraw1394-devel libdc1394-devel jasper-devel jasper-utils swig python libtool nasm -y