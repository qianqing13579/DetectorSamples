QT += core
QT -= gui

TARGET = DetectorSamples
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

QMAKE_CXXFLAGS += -O3 # 优化

DEFINES += DLIB_NO_GUI_SUPPORT  # dlib needed

QMAKE_CXXFLAGS+=-std=c++11

SOURCES += \
    Src/Detector/DetectorInterface.cpp \
    Src/Detector/DetectorSSD.cpp \
    Src/Detector/DetectorYOLOV3.cpp \
    Src/Detector/DetectorYOLOV5.cpp \
    Src/Utility/CommonUtility.cpp \
    Src/Utility/Filesystem.cpp \
    Src/Utility/SimpleLog.cpp \
    Src/Sample.cpp \
    Src/main.cpp

HEADERS += \
    Src/Detector/DetectorInterface.h \
    Src/Detector/DetectorSSD.h \
    Src/Detector/DetectorYOLOV3.h \
    Src/Detector/DetectorYOLOV5.h \
    Src/Utility/CommonDefinition.h \
    Src/Utility/CommonUtility.h \
    Src/Utility/Filesystem.h \
    Src/Utility/SimpleLog.h \
    Src/Sample.h

INCLUDEPATH +=./Resource/3rdParty/OpenCV_3.4.11/include/ \
            ./Src/  \
            ./Src/Utility/ \
            ./Src/Detector/

# QT的配置文件中，LIBS路径相对于可执行文件路径，其他相对于项目当前路径
# windows
win32-msvc*{
CONFIG(debug,debug|release) {
LIBS += -L../../Resource/3rdParty/OpenCV_3.4.11/x64/vc12/lib/ \
    -lopencv_calib3d3411d \
    -lopencv_core3411d \
    -lopencv_dnn3411d \
    -lopencv_features2d3411d \
    -lopencv_flann3411d \
    -lopencv_highgui3411d \
    -lopencv_imgcodecs3411d \
    -lopencv_imgproc3411d \
    -lopencv_ml3411d \
    -lopencv_photo3411d \
    -lopencv_shape3411d \
    -lopencv_superres3411d \
    -lopencv_videoio3411d \
    -lopencv_video3411d
} else {
LIBS += -L../../Resource/3rdParty/OpenCV_3.4.11/x64/vc12/lib/ \
    -lopencv_calib3d3411 \
    -lopencv_core3411 \
    -lopencv_dnn3411 \
    -lopencv_features2d3411 \
    -lopencv_flann3411 \
    -lopencv_highgui3411 \
    -lopencv_imgcodecs3411 \
    -lopencv_imgproc3411 \
    -lopencv_ml3411 \
    -lopencv_photo3411 \
    -lopencv_shape3411 \
    -lopencv_superres3411 \
    -lopencv_videoio3411 \
    -lopencv_video3411
}

}

# linux
linux-g++*{
LIBS += -L../../Resource/3rdParty/OpenCV_3.4.11/Ubuntu16.04/lib/ \
    -lopencv_calib3d \
    -lopencv_core \
    -lopencv_dnn \
    -lopencv_features2d \
    -lopencv_flann \
    -lopencv_highgui \
    -lopencv_imgcodecs \
    -lopencv_imgproc \
    -lopencv_ml \
    -lopencv_objdetect \
    -lopencv_photo \
    -lopencv_shape \
    -lopencv_superres \
    -lopencv_videoio \
    -lopencv_video
}
