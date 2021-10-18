# 设置cmake的最低版本
cmake_minimum_required(VERSION 2.9)

# 设置项目名
project(DetectorSamples)

# 设置编译器
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -std=c++11)
set(CMAKE_BUILD_TYPE release)

# 添加头文件路径
set(INCLUDE_PATH    ${CMAKE_CURRENT_SOURCE_DIR}/Src/
                    ${CMAKE_CURRENT_SOURCE_DIR}/Src/Utility/ 
                    ${CMAKE_CURRENT_SOURCE_DIR}/Src/Detector/
                    ${CMAKE_CURRENT_SOURCE_DIR}/depend/include/)
include_directories(${INCLUDE_PATH})

# 添加依赖库路径
set(LIBRARY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/depend/lib64/)
link_directories(${LIBRARY_PATH})

# 添加依赖库
set(LIBRARY opencv_core
            opencv_imgproc
            opencv_imgcodecs
            opencv_dnn)
link_libraries(${LIBRARY})

# 添加源文件
set(SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/Src/main.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/Src/Sample.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/Src/Detector/DetectorSSD.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/Src/Detector/DetectorYOLOV3.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/Src/Detector/DetectorYOLOV5.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/Src/Detector/DetectorInterface.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/Src/Utility/CommonUtility.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/Src/Utility/Filesystem.cpp)

# 添加可执行目标
add_executable(DetectorSamples ${SOURCE_FILES})
