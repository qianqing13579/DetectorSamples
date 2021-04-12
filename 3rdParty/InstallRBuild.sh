
############################ 在线安装依赖 ###############################
#cd ./3rdParty
#pip install rbuild-master.tar.gz


############################ 离线安装依赖 ###############################
# 安装依赖
cd ./3rdParty/rbuild_depend
pip install click-6.6-py2.py3-none-any.whl
pip install six-1.15.0-py2.py3-none-any.whl
pip install subprocess32-3.5.4.tar.gz
pip install cget-0.1.9.tar.gz

# 安装rbuild
cd ../
pip install rbuild-master.tar.gz


