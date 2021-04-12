# DetectorSamples

## 目录
- [目录结构](#目录结构)
- [构建安装](#构建安装)
- [运行](#运行)
- [模型转换说明](#模型转换说明)

## 目录结构
```
├── 3rdParty:依赖库
├── Resource:资源文件目录，包括模型，测试图像等
│   └── Models:示例模型
│   └── TestImage:测试图像
│   └── Configuration.xml:配置文件
├── Src
│   └── Detector:检测器源码
│   └── Utility:工具
│   └── Sample.cpp:示例程序
│   └── main.cpp
├── CMakeLists.txt
├── README.md
├── requirements.txt

```
## 构建安装


### 安装rbuild
当前目录为工程根目录

```
sh ./3rdParty/InstallRBuild.sh
```

### 安装OpenCV依赖
```
sh ./3rdParty/InstallOpenCVDependences.sh
```
注意:根据不同的linux系统,选择脚本中相应的安装方式,默认为centos系统

### 修改CMakeLists.txt
如果使用ubuntu系统，需要修改CMakeLists.txt中依赖库路径：
将"${CMAKE_CURRENT_SOURCE_DIR}/depend/lib64/"修改为"${CMAKE_CURRENT_SOURCE_DIR}/depend/lib/"

### 安装OpenCV并编译工程
当前目录为工程根目录

```
rbuild build -d depend
```

### 设置环境变量

将OpenCV库路径加入环境变量LD_LIBRARY_PATH，在~/.bashrc中添加如下语句：

**Centos**:
```
export LD_LIBRARY_PATH=项目根目录/depend/lib64/:$LD_LIBRARY_PATH
```
**Ubuntu**:
```
export LD_LIBRARY_PATH=项目根目录/depend/lib/:$LD_LIBRARY_PATH
```

然后执行:
```
source ~/.bashrc
```

## 运行
```
cd ./build/
./DetectorSamples
```
根据提示选择要运行的示例程序，比如执行
```
./DetectorSamples 0
```
运行SSD示例程序,会在当前目录下生成检测结果图像Result.jpg

## 模型转换说明
### SSD模型
1. Resource/Models/SSD/目录下提供了模型转换示例，deploy.prototxt对应的onnx版本为deploy_Caffe2ONNX.prototxt。本示例使用caffe框架，其他框架需要按照caffe的实现方式作出相应的修改。
2. 模型转换成功后需要修改 Resource/Configuration.xml中SSD网络结构参数，xml中对每个参数的含义作出了详细的解释。

### YOLOV3模型
1. Resource/Models/YOLOV3/目录下提供了模型转换示例，deploy.prototxt对应的onnx版本为deploy_Caffe2ONNX.prototxt。本示例使用caffe框架，其他框架需要按照caffe的实现方式作出相应的修改。
2. 模型转换成功后需要修改 Resource/Configuration.xml中YOLOV3网络结构参数，xml中对每个参数的含义作出了详细的解释。

### YOLOV5模型
本示例中的模型使用的是https://github.com/ultralytics/yolov5 中提供的模型，需要注意的是官方的YOLOV5模型中的Focus模块使用到了步长为2的切片操作,OpenCV暂时不支持该操作,在转换为onnx模型的时候对这部分做了修改，使用view和permute操作代替(这部分代码官方代码中也提供了，只是被注释掉了)，具体修改方法参考Resource/Models/YOLOV5/yolov5s.onnx模型。