> 首次部署必读：[资源获取与复现检查](7_Resources.md)。本项目代码统一从 `DIY_hyrobot` 获取；第三方模型和数据集沿用上游下载，只有硬件目录压缩发布。

# 一、主机环境配置

本项目主机环境为 Ubuntu22.04 系统，确保与端侧处于同一局域网络环境。

1. 先按[资源获取与复现说明](7_Resources.md)克隆 `DIY_hyrobot`，按说明从上游获取所需模型，并还原项目自有测试文件。推荐位置为 `~/hy_linux/nfs/DIY_hyrobot`，并在 NFS 导出目录建立 `hy_ros -> DIY_hyrobot/hy_ros` 软链接。这样下文 `/mnt/nfs/hy_ros` 命令可以继续使用。

2. 按资源说明中的 NFS 命令配置主机共享目录。原教程的 `nfs_config.sh` 不在项目内，现已改为明确的系统命令。

3. 参考教程：[鱼香 ROS2 一键安装](https://blog.csdn.net/weixin_71683006/article/details/150465016?ops_request_misc=elastic_search_misc&request_id=dd0d7d3fd479c65031b7b2767c112822&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-150465016-null-null.142^v102^pc_search_result_base3&utm_term=%E9%B1%BC%E9%A6%99ros2%E4%B8%80%E9%94%AE%E5%AE%89%E8%A3%85&spm=1018.2226.3001.4187)

   ```shell
   # 一键配置ROS2 humble开发环境
   cd /opt
   wget --no-check-certificate https://fishros.com/install -O fishros
   chmod +x fishros
   ./fishros
   # 1:一键安装 -> 1:更换系统源再继续安装 -> 2:更新系统源并清理第三方源 -> 1：自动测速选择最快的源 -> \
   # 1:中科大镜像源 -> 1:humble(ROS2) -> 1:humble(ROS2)桌面版 -> 开始自动安装等待20min左右
   ```

   [![ROS2 安装界面](_static/recovered/ylDjRa.png)](_static/recovered/ylDjRa.png)

4. 可在主机环境按照相同方法同步配置完 ros2 后，测试双端通信是否成功

   - 端侧：`ros2 run demo_nodes_cpp talker`
   - 主机侧：`ros2 run demo_nodes_cpp listener`

   [![通信测试](_static/recovered/ylDFP4.png)](_static/recovered/ylDFP4.png)

5. 安装必要的 python 库

   ```shell
   # 用于语音复刻模型
   sudo apt-get install python3-aiohttp python3-websockets
   ```

# 二、端侧环境配置

## 1. 个人初始配置

### 1.1 烧录镜像文件

烧录镜像文件 **Orangepi5pro_1.0.6_ubuntu_jammy_server_linux5.10.160** 到 tf 卡中。

### 1.2 修改 root 用户自动登录终端

密码为 **orangepi**

```shell
sudo auto_login_cli.sh root
```

### 1.3 连接 wifi

```{note}
输入自己的 wifi 名和密码，并记录 ip 地址。
```

```shell
sudo nmcli dev wifi connect wifi名称 password wifi密码
ip addr show wlan0
```

### 1.4 SSH 远程连接测试

密码为 orangepi

```{note}
替换为端侧 ip 地址。例如 192.168.22.188
```

```shell
ssh root@端侧IP地址
```

## 2. 挂载配置

### 2.1 挂载 NFS

```{important}
挂载命令中需修改为实际的 ubuntu 主机侧 IP 地址（如 192.168.22.177）、NFS 开发路径（如 /home/k/hy_linux/nfs）和端侧挂载路径（如 /mnt/nfs）。
```

```shell
# 1. 安装和挂载NFS
sudo apt update
sudo apt install -y nfs-common
mkdir /mnt/nfs
# 手动挂载，重启后不会自动挂载
sudo mount -t nfs -o nolock 主机IP地址:/主机挂载路径 端侧挂载路径
sudo mount -t nfs -o nolock 192.168.22.177:/home/k/hy_linux/nfs /mnt/nfs

# 2. 配置nfs开机自动挂载
# 2.1 修改环境变量，设置开机自动加载~/.bashrc
echo -e 'if [ -f ~/.bashrc ]; then\n    . ~/.bashrc\nfi' >> ~/.bash_profile

# 2.2 修改环境变量，添加自动挂载nfs命令并激活环境
echo 'sudo mount -t nfs -o nolock 192.168.22.177:/home/k/hy_linux/nfs /mnt/nfs' >> ~/.bashrc
source ~/.bash_profile
```

### 2.2 挂载 emmc(可选)

```shell
# 挂载emmc作为存储空间并配置开机自动挂载
# 查看所有存储设备和分区，查看是否有未挂载的eMMC分区
lsblk
# 格式化存储空间大的分区格式化为ext4（-F强制格式化，-L添加卷标hy_tmp，方便识别）(按实际情况修改mmcblk0p2)
sudo mkfs.ext4 -F -L hy_tmp /dev/mmcblk0p2
# 创建挂载点目录
sudo mkdir -p /hy_tmp
# 设置目录权限（允许所有用户读写执行，方便存放文件）
sudo chmod 777 /hy_tmp
# 设置目录所有者（可选，让当前用户拥有完整权限）
sudo chown $USER:$USER /hy_tmp
# 查看mmcblk0p2的UUID（复制输出中的UUID值，不含引号）
blkid /dev/mmcblk0p2
# 使用nano编辑器打开fstab（新手推荐nano，操作简单）
sudo nano /etc/fstab
# 添加以下内容到fstab文件末尾(将UUID替换为实际值)
UUID=实际UUID /hy_tmp ext4 defaults 0 2
# 查看是否挂载成功
df -h
# 读写测试
# 进入/hy_tmp目录
cd /hy_tmp
# 创建测试文件夹和文件
mkdir video audio screenshot
touch audio/test.mp3 video/test.mp4 screenshot/test.png
# 查看创建结果
ls -l
```

## 3. 安装 ROS2 humble

参考教程：[鱼香 ROS2 一键安装](https://blog.csdn.net/weixin_71683006/article/details/150465016?ops_request_misc=elastic_search_misc&request_id=dd0d7d3fd479c65031b7b2767c112822&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-150465016-null-null.142^v102^pc_search_result_base3&utm_term=%E9%B1%BC%E9%A6%99ros2%E4%B8%80%E9%94%AE%E5%AE%89%E8%A3%85&spm=1018.2226.3001.4187)

```shell
# 一键配置ROS2 humble开发环境
cd /opt
wget --no-check-certificate https://fishros.com/install -O fishros
chmod +x fishros
./fishros
# 1:一键安装 -> 1:更换系统源再继续安装 -> 2:更新系统源并清理第三方源 -> 1：自动测速选择最快的源 -> \
# 1:中科大镜像源 -> 1:humble(ROS2) -> 1:humble(ROS2)桌面版 -> 开始自动安装等待20min左右
```

[![ROS2 安装界面](_static/recovered/ylDjRa.png)](_static/recovered/ylDjRa.png)

- 可在主机环境按照相同方法同步配置完 ros2 后，测试双端通信是否成功
  - 端侧：`ros2 run demo_nodes_cpp talker`
  - 主机侧：`ros2 run demo_nodes_cpp listener`

[![通信测试](_static/recovered/ylDFP4.png)](_static/recovered/ylDFP4.png)

## 4. 硬件测试

### 4.1 摄像头测试

```shell
# 配置推流环境
mkdir /opt/camera
cd /opt/camera
# 安装cmake和libjpeg8-dev（用于编译mjpg-streamer）
sudo apt-get install cmake libjpeg8-dev
# 从公开上游获取；不依赖作者本地的 master.zip
git clone https://github.com/jacksonliam/mjpg-streamer.git
cd mjpg-streamer
git checkout 310b29f4a94c46652b20c4b7b6e5cf24e532af39
cd mjpg-streamer-experimental
make
sudo make install
# 安装v4l工具（用于查询摄像头参数）
sudo apt install -y v4l-utils
# 查看摄像头支持的格式、分辨率、帧率
v4l2-ctl --list-formats-ext
# 启动推流服务
/usr/local/bin/mjpg_streamer -i "/usr/local/lib/mjpg-streamer/input_uvc.so -d /dev/video0 -n -f 10 -r 1920x1080" -o "/usr/local/lib/mjpg-streamer/output_http.so -p 8085 -w /usr/local/share/mjpg-streamer/www"
```

```{tip}
打开浏览器，输入**端侧 IP 地址加端口号**（如：http://192.168.22.188:8085）即可查看实时视频流。
```

[![摄像头推流](_static/recovered/ylDTcN.png)](_static/recovered/ylDTcN.png)

### 4.2 雷达测试

```shell
# 新建应用程序空间
mkdir /opt/lidar
cd /opt/lidar
# 上传wheeltec_udev.sh脚本到该目录，上传ldlidar_driver_python3.py文件到该目录
cp /mnt/nfs/hy_ros/source/1.Lidar/wheeltec_udev.sh /opt/lidar/
cp /mnt/nfs/hy_ros/source/1.Lidar/ldlidar_driver_python3.py /opt/lidar/
# 赋予执行权限
sudo chmod 777 wheeltec_udev.sh
sudo chmod 777 ld*
# 执行脚本
sudo ./wheeltec_udev.sh
# 重新插拔后检查更名是否成功（wheeltec_lidar -> ttyUSB0）
ll /dev
# 检查python版本，安装pyserial库
sudo apt install -y python3-pip
sudo pip3 install pyserial
# 执行对应测试脚本（输出刷新说明雷达正常工作）
python3 ldlidar_driver_python3.py
```

[![雷达串口配置](_static/recovered/ylDU8H.png)](_static/recovered/ylDU8H.png)

[![雷达输出](_static/recovered/ylDARQ.png)](_static/recovered/ylDARQ.png)

### 4.3 麦克风测试

```shell
# 查看麦克风设备列表
arecord -l
# 测试麦克风录音
mkdir -p /tmp/record
arecord --device=plughw:3,0 --format S16_LE --rate 44100 -c 2 -d 5 /tmp/record/test.wav
# 参数说明：
# --device=plughw:3,0：指定使用的麦克风设备（根据arecord -l输出的设备列表修改）
# --format S16_LE：指定音频格式为16位小端字节序
# --rate 44100：指定采样率为44100Hz
# -c 2：指定通道数为2（立体声）
# -d 5：指定录音时长为5秒
# /tmp/record/test.wav：指定输出文件路径
# 检查并下载test.wav文件是否录音成功
```

### 4.4 串口启动

```shell
# 启动串口3
sudo orangepi-config
# 依次选择System->Hardware -> uart3-m2 -> Space开启 -> save -> back -> Reboot
```

# 三、语音模型部署

## 1. 部署 Sherpa-ONNX-RKNPU 流式 ASR 模型

**参考：**[Sherpa-ONNX for RKNPU](https://k2-fsa.github.io/sherpa/onnx/rknn/install.html#)

### 安装 Sherpa-ONNX-RKNPU

```shell
# 1. ROS2功能包API调用库
sudo apt-get install libportaudio2  # 安装 PortAudio 库
pip3 install sounddevice # 安装 Python 音频处理库

# 2.安装sherpa-onnx，
mkdir /opt/sherpa-onnx
cd /opt/sherpa-onnx

# 1> 方式1：在线下载安装
pip install sherpa-onnx -f https://k2-fsa.github.io/sherpa/onnx/rk-npu-cn.html

# 2> 方式2：直接复制文件到/opt/sherpa-onnx/目录下
cp /mnt/nfs/hy_ros/source/0.RK3588S/1.sherpa-onnx/sherpa-onnx-cp310/sherpa_onnx-1.12.19-cp310-cp310-manylinux_2_27_aarch64.whl /opt/sherpa-onnx/
chmod +x /opt/sherpa-onnx/sherpa_onnx-1.12.19-cp310-cp310-manylinux_2_27_aarch64.whl
# 安装sherpa-onnx
pip3 install sherpa_onnx-1.12.19-cp310-cp310-manylinux_2_27_aarch64.whl
which sherpa-onnx

# 3.复制librknnrt.so文件到/opt/sherpa-onnx/目录下
cp /mnt/nfs/hy_ros/source/0.RK3588S/1.sherpa-onnx/sherpa-onnx-rknn-stream/librknnrt.so /opt/sherpa-onnx/
chmod +x /opt/sherpa-onnx/librknnrt.so

# 更新/lib/librknnrt.so 或 /usr/lib/librknnrt.so
cp /opt/sherpa-onnx/librknnrt.so /lib/
cp /opt/sherpa-onnx/librknnrt.so /usr/lib/

# 4. 系统级修复libonnxruntime.so库动态链接问题，为后续ros2 C++开发提供支持
echo "$(python3 -c "import sherpa_onnx, os; print(os.path.join(os.path.dirname(sherpa_onnx.__file__), 'lib'))")" | sudo tee /etc/ld.so.conf.d/sherpa_onnx.conf
sudo ldconfig

# 5. 检查是否开启rknn支持，具体检查是否具有librknnrt.so库，若没有则参考连接手动安装
ldd $(which sherpa-onnx)
# 查看当前rknnrt版本，实测2.3.0版本可用无报错
strings /lib/librknnrt.so | grep "librknnrt version"
```

[![rknnrt 版本检查](_static/recovered/ylDRYF.png)](_static/recovered/ylDRYF.png)

### 下载预训练模型并终端测试

当前上游同名模型包与作者本地旧包有差异，下载校验值和 RKNN 运行库兼容说明见[资源来源说明](7_Resources.md)。原记录的“2.3.0 可用”不等于所有新版模型/运行库组合均已实机验证。

**参考：**[pre-trained models for RKNPU](https://k2-fsa.github.io/sherpa/onnx/rknn/models.html#)

```shell
# 1> 方式1：在线下载文件
wget https://github.com/k2-fsa/sherpa-onnx/releases/download/asr-models/sherpa-onnx-rk3588-streaming-zipformer-small-bilingual-zh-en-2023-02-16.tar.bz2

# 2> 方式2：直接复制文件到/opt/sherpa-onnx/目录下
cp /mnt/nfs/hy_ros/source/0.RK3588S/1.sherpa-onnx/sherpa-onnx-rknn-stream/sherpa-onnx-rk3588-streaming-zipformer-small-bilingual-zh-en-2023-02-16.tar.bz2 /opt/sherpa-onnx/

# 2. 解压文件
tar xvf sherpa-onnx-rk3588-streaming-zipformer-small-bilingual-zh-en-2023-02-16.tar.bz2
rm sherpa-onnx-rk3588-streaming-zipformer-small-bilingual-zh-en-2023-02-16.tar.bz2

# 3. 可监控rknpu加载状态
watch -n 0.5 cat /sys/kernel/debug/rknpu/load

# 4. 解码文件,查看是否正常工作
sherpa-onnx \
  --provider=rknn \
  --encoder=./sherpa-onnx-rk3588-streaming-zipformer-small-bilingual-zh-en-2023-02-16/encoder.rknn \
  --decoder=./sherpa-onnx-rk3588-streaming-zipformer-small-bilingual-zh-en-2023-02-16/decoder.rknn \
  --joiner=./sherpa-onnx-rk3588-streaming-zipformer-small-bilingual-zh-en-2023-02-16/joiner.rknn \
  --tokens=./sherpa-onnx-rk3588-streaming-zipformer-small-bilingual-zh-en-2023-02-16/tokens.txt \
  ./sherpa-onnx-rk3588-streaming-zipformer-small-bilingual-zh-en-2023-02-16/test_wavs/4.wav

# 5. 实时音频测试
sherpa-onnx-alsa \
  --provider=rknn \
  --encoder=./sherpa-onnx-rk3588-streaming-zipformer-small-bilingual-zh-en-2023-02-16/encoder.rknn \
  --decoder=./sherpa-onnx-rk3588-streaming-zipformer-small-bilingual-zh-en-2023-02-16/decoder.rknn \
  --joiner=./sherpa-onnx-rk3588-streaming-zipformer-small-bilingual-zh-en-2023-02-16/joiner.rknn \
  --tokens=./sherpa-onnx-rk3588-streaming-zipformer-small-bilingual-zh-en-2023-02-16/tokens.txt \
  plughw:3,0
```

[![模型解码](_static/recovered/ylDg1m.png)](_static/recovered/ylDg1m.png)

[![实时音频](_static/recovered/ylDqT9.png)](_static/recovered/ylDqT9.png)

## 2. 部署 RKLLM Qwen3-VL-2B 模型

**参考：**

- [RKLLM 模型转换](https://doc.embedfire.com/linux/rk356x/Ai/zh/latest/lubancat_ai/example/qwen2_vl.html)
- [Qwen-VL-2B 部署 RK3588S](https://blog.csdn.net/lajuchenghui/article/details/150111489)

```shell
# 1. 安装OpenCV库，Qwen3-VL-2B ROS功能包多模态输入需求库
sudo apt-get install libopencv-dev python3-opencv # 安装 OpenCV 库

# 2.新建目录
mkdir /opt/rknn-llm
cd /opt/rknn-llm
# 本地 SDK 的真实版本是 1.2.3，固定提交以匹配模型
git clone --branch release-v1.2.3 --depth 1 https://github.com/airockchip/rknn-llm.git
cd rknn-llm
git checkout f7df8e5de9b40dc7aaa66194cf14d4895aaf159a
# 还原原作者修改过的编译器路径（小脚本直接保存在本项目中）
cp /mnt/nfs/hy_ros/resources/rkllm/build-linux.sh examples/multimodal_model_demo/deploy/build-linux.sh
sudo apt install -y cmake gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

# 3.进入multimodal_model_demo/deploy目录
cd /opt/rknn-llm/rknn-llm/examples/multimodal_model_demo/deploy
chmod +x build-linux.sh
./build-linux.sh

# 4.更新librkllmrt.so库，为后续ros2 C++开发提供支持
cp /opt/rknn-llm/rknn-llm/rkllm-runtime/Linux/librkllm_api/aarch64/librkllmrt.so /lib/
chmod +x /lib/librkllmrt.so

# 5. 更新rkllm.h头文件，为后续ros2 C++开发提供支持
cp /opt/rknn-llm/rknn-llm/rkllm-runtime/Linux/librkllm_api/include/rkllm.h /usr/include/
cp /opt/rknn-llm/rknn-llm/examples/multimodal_model_demo/deploy/3rdparty/librknnrt/Linux/librknn_api/include/rknn_api.h /usr/include/
sudo ldconfig
ls -l /usr/include/rkllm.h
ls -l /usr/include/rknn_api.h

# 6.复制rkllm模型文件
cd install/demo_Linux_aarch64

# 复制模型文件到示例目录
cp /mnt/nfs/hy_ros/source/0.RK3588S/2.rknn-llm/qwen3-vl-2b_vision_rk3588.rknn /opt/rknn-llm/
cp /mnt/nfs/hy_ros/source/0.RK3588S/2.rknn-llm/qwen3-vl-2b-instruct_w8a8_rk3588.rkllm /opt/rknn-llm/
chmod +x /opt/rknn-llm/qwen*

# 7.设置环境变量
export LD_LIBRARY_PATH=./lib

# 8. 运行示例
./demo demo.jpg /opt/rknn-llm/qwen3-vl-2b_vision_rk3588.rknn /opt/rknn-llm/qwen3-vl-2b-instruct_w8a8_rk3588.rkllm 2048 4096 3
```

[![Qwen2-VL 运行](_static/recovered/ylD5Qc.png)](_static/recovered/ylD5Qc.png)

[![Qwen2-VL 结果](_static/recovered/ylDef6.png)](_static/recovered/ylDef6.png)

## 3. 部署百度大模型音色复刻 TTS

**参考：**[百度大模型音色复刻 TTS](https://cloud.baidu.com/doc/SPEECH/s/Am8wytft2)

1. 安装必要的 python 库

   ```{note}
   主机侧和端侧均需要安装，音色管理创建等操作可以在主机侧进行。
   ```

   ```shell
   # 安装异步 http 库和 websockets 库
   sudo apt-get install python3-aiohttp python3-websockets
   ```

2. 复刻本人音色，需录制 15s 任意文本音频，并转换为 base64 编码的 mp3 格式

   - [在线转换工具](https://www.toolfk.com/en/tools/audio-to-base64.html)
   - 下载 base64.txt 文件，替换 `hy_ros/src/audio_tts/audio_tts/base64.txt`

3. 机器人音色示例位于 `hy_ros/src/audio_tts/audio_tts/base64_robot.txt`，也可自行准备录音。

   ```{tip}
   通过 `BAIDU_VOICE_SAMPLE` 环境变量指定 base64 文件的完整路径。
   ```

4. 获取 API key

   - 参考[百度大模型应用创建](https://ai.baidu.com/ai-doc/REFERENCE/Bkru0l60m)创建应用配置百度智能云 api 密钥，Server Key
   - 在运行终端设置 `BAIDU_API_KEY`、`BAIDU_SECRET_KEY` 环境变量。启动文件与测试程序均从环境读取，不要把密钥提交到 Git。
   - 设置 `tts_api.py` 文件中的 `run_create_voice` 变量为 True，表示在运行时创建 Voice ID

5. 创建 Voice ID

   ```{note}
   可直接在主机侧运行创建并更新 Voice ID。
   ```

   - 在 `hy_ros/src/audio_tts/audio_tts` 目录下运行 `python3 tts_api.py`
   - 查看并记录返回的 Voice ID
   - 设置 `BAIDU_VOICE_ID` 环境变量为自己账号创建的音色 ID；每个启动 TTS 的终端都需加载这三个环境变量。

6. 音色管理

   - 设置 `tts_api.py` 文件中的 `run_tts` 变量为 True，表示在运行时使用音色复刻 TTS 进行测试
   - 设置 `tts_api.py` 文件中的 `run_list_voice` 变量为 True，表示在运行时查看 Voice ID 列表
   - 设置 `tts_api.py` 文件中的 `run_query_detail` 变量为 True，表示在运行时查看 Voice ID 详情
   - 设置 `tts_api.py` 文件中的 `run_delete_voice` 变量为 True，表示在运行时删除 Voice ID

[![语音复刻模型](_static/recovered/ylD9td.png)](_static/recovered/ylD9td.png)

[![语音复刻模型测试](_static/recovered/ylDMyg.png)](_static/recovered/ylDMyg.png)

# 四、Foxglove Web 可视化配置

## 1. 端侧部署 (RK3588S)

Foxglove Bridge 用于将 ROS 2 话题通过 WebSocket 转发给 Foxglove Studio。

```bash
# 配置 joint_state_publisher,实现关节状态发布
sudo apt-get install -y ros-humble-joint-state-publisher
# 配置 foxglove_bridge,实现话题转发
sudo apt-get install -y ros-humble-foxglove-bridge
# 配置 RTSP 转 WebRTC 网关，使用 go2rtc 拉取硬件编码 RTSP 流
```

## 2. 上位机连接 (PC/iPad)

1. 打开 [Foxglove Studio](https://foxglove.dev/) (网页版或客户端)。
2. 点击 **"Open Connection"**。
3. 选择 **"Foxglove WebSocket"** 协议。
4. 输入端侧地址：`ws://192.168.22.219:8765` (IP 根据实际情况调整)。
5. 连接成功后即可订阅和查看端侧的所有话题。

[![Foxglove 连接](_static/recovered/ylDPg0.png)](_static/recovered/ylDPg0.png)

# 五、 yolov8 目标检测模型训练

如果希望自己训练 yolov8 黄油或其他目标检测模型，以下教程可提供参考。

## 1. 数据集准备

1. 使用摄像头自行采集并保存图片到 `source/butter_img`（建议 300 张以上），再按下文标注。原文提及的 `tools/capture_tool.py` 未包含在项目中；也可以直接使用下方 Roboflow 的已标注数据集。

2. 使用 [roboflow](https://app.roboflow.com) 平台实现数据集标注和管理。

```{tip}
本次训练使用 60% 网络数据集 + 40% 个人自定义数据集 [butter_robot_dataset](https://universe.roboflow.com/butter-robot/butter_robot)，可自行下载删除修改。
```

[![黄油图像样本](_static/recovered/ylDQYY.png)](_static/recovered/ylDQYY.png)

[![黄油检测样本画面](_static/recovered/ylDW3b.png)](_static/recovered/ylDW3b.png)

[![数据集图像列表](_static/recovered/ylD2T1.png)](_static/recovered/ylD2T1.png)

3. 数据集预处理包括数据增强、数据划分，下载 yolov8 数据集配置文件。

[![数据集版本与导出](_static/recovered/ylDJuL.png)](_static/recovered/ylDJuL.png)

## 2. Win 环境实现模型训练

### 2.1 配置虚拟环境

1. 初始化 uv 项目

```powershell
# 安装uv
pip3 install uv
# 查看python版本
uv python list
# 选择python版本创建虚拟环境项目
uv init -p 3.12 butter_train_env
cd butter_train_env
```

2. 添加依赖

```powershell
uv add pandas  # 安装pandas库
uv add onnxscript  # 安装onnxscript库，用于转换模型
uv add --dev ipykernel  # 安装ipykernel库，用于在Jupyter中选择项目内核
```

3. 创建并注册项目内核

在项目虚拟环境下注册一个可在 Jupyter/Trae 中选择的内核：

```powershell
.venv\Scripts\python -m ipykernel install --user --name butter_train --display-name "Python (butter_train)"
```

完成后，系统目录 `C:\Users\<你的用户名>\AppData\Roaming\jupyter\kernels\butter_train` 会出现对应的 `kernelspec`。

4. 在 VSCode 或者 Pycharm IDE 中选择内核

打开 Notebook，使用右上角的内核选择器切换到：`Python (butter_train)`。选择后，`import pandas as pd` 即可在该项目环境中正常使用。

5. UV 常用命令

```shell
uv tree  # 查看项目依赖树
uv sync  # 根据 `pyproject.toml` 同步安装依赖到 `.venv`
uv sync --no-dev  # 仅安装非开发依赖
uv add 'pandas==2.31.0'  # 安装指定版本的 pandas 到项目环境
uv add -r requirements.txt  # 根据 `requirements.txt` 安装非开发依赖
uv add --dev ipykernel  # 包仅会被加入到开发环境的分组中
```

### 2.2 安装 torch-gpu

1. 确认 CUDA 版本以及安装 CUDA 和 cuDNN 版本

参考：[CUDA 和 cuDNN 安装](https://blog.csdn.net/sinat_26398509/article/details/143566753)

2. UV 安装 torch-gpu

参考：[torch-gpu 安装](https://blog.csdn.net/Humbunklung/article/details/146046406)

```text
# 测试环境
显卡：Nvida GeForce RTX 4070 Super
CUDA 版本：13.1
CUDA Toolkit 版本 13.0
cuDNN 版本：9.11.0
pytorch-gpu 版本：2.9.1
```

### 2.3 安装 ultralytics

安装 ultralytics 库，测试 yolov8n 预训练参数是否成功加载到 GPU。

```{important}
必须使用 ultralytics rknn 修改版才能正确导出 RKNN 模型，且依然可以通过 UV 进行管理。
```

```shell
# 安装适配 RKNN 的 ultralytics 库（airockchip 维护版本）
# 注意：必须使用此修改版才能正确导出 RKNN 模型，且依然可以通过 UV 进行管理
uv add git+https://github.com/airockchip/ultralytics_yolov8.git
```

pyproject.toml 配置文件可参考如下：

```toml
# UV配置文件仅供参考，实际使用时请根据自己的环境进行修改
[project]
name = "butter-train-env"
version = "0.1.0"
description = "Add your description here"
readme = "README.md"
requires-python = ">=3.12"
dependencies = [
    "onnxscript>=0.5.0",
    "pandas>=2.3.3",
    "tensorboard>=2.20.0",
    "torch==2.5.1",
    "torchaudio==2.5.1",
    "torchvision==0.20.1",
    "ultralytics",
]

[tool.uv.sources]
torch = [
  { index = "pytorch-cu124", marker = "sys_platform == 'linux' or sys_platform == 'win32'" },
]
torchvision = [
  { index = "pytorch-cu124", marker = "sys_platform == 'linux' or sys_platform == 'win32'" },
]
torchaudio = [
  { index = "pytorch-cu124", marker = "sys_platform == 'linux' or sys_platform == 'win32'" },
]
ultralytics = { git = "https://github.com/airockchip/ultralytics_yolov8.git" }

[[tool.uv.index]]
name = "pytorch-cu124"
url = "https://download.pytorch.org/whl/cu124"
explicit = true

[dependency-groups]
dev = [
    "ipykernel>=7.1.0",
]
```

```python
# 测试安装是否全部成功
import torch
import ultralytics
from ultralytics import YOLO
from pathlib import Path

# 验证PyTorch GPU可用性
print("PyTorch GPU可用：", torch.cuda.is_available())
print("CUDA版本：", torch.__version__)
# 验证Ultralytics版本及GPU适配
print("Ultralytics版本：", ultralytics.__version__)
# 加载YOLOv8n并指定GPU
model = YOLO("yolov8n.pt")
model.to("cuda:0")
print("YOLOv8n已加载到GPU：", next(model.model.parameters()).is_cuda)
```

[![CUDA torch 训练yolov8n模型](_static/recovered/ylD4eM.png)](_static/recovered/ylD4eM.png)

### 2.4 yolov8n 模型训练和转换

```python
# 开始训练
# data: 数据集配置文件路径 (使用绝对路径避免错误)
# epochs: 训练轮数
# imgsz: 输入图像大小
# batch: 批次大小
# device: 训练设备 (0 表示使用第一块 GPU)
results = model.train(
    data=str(Path("dataset.yaml").resolve()),
    epochs=120,
    imgsz=640,
    batch=32,
    device=0,
    project="butter_train_results",
    name="yolov8n_butter",
    exist_ok=True,  # 允许覆盖同名目录，方便调试
    verbose=True,  # 显示详细训练日志
)
```

```shell
# 启动 tensorboard 可视化训练过程
cd butter_train_env
uv run tensorboard --logdir ../butter_train_results/yolov8n_butter --port 6006
```

|        数据类别         | 指标 / 损失名称 | 最终数值 | 单位 | 备注                          |
| :---------------------: | :-------------: | :------: | :--: | :---------------------------- |
|  **Metrics 检测指标**   |    mAP50(B)     |  92.56%  |  -   | IoU≥0.5 时的平均精度          |
|                         |   mAP50-95(B)   |  75.70%  |  -   | IoU 从 0.5 到 0.95 的平均精度 |
|                         |  Precision(B)   |  89.25%  |  -   | 预测为黄油的精准度            |
|                         |    Recall(B)    |  87.58%  |  -   | 实际黄油的召回率              |
| **训练集（Train）损失** |    box_loss     |  0.2732  |  -   | 预测框位置损失                |
|                         |    cls_loss     |  0.1944  |  -   | 黄油类别预测损失              |
|                         |    dfl_loss     |  0.8377  |  -   | 框分布拟合损失                |
|  **验证集（Val）损失**  |    box_loss     |  0.7615  |  -   | 预测框位置损失                |
|                         |    cls_loss     |  0.7156  |  -   | 黄油类别预测损失              |
|                         |    dfl_loss     |  1.3528  |  -   | 框分布拟合损失                |

[![训练结果](_static/recovered/ylDEKr.png)](_static/recovered/ylDEKr.png)

[![训练结果](_static/recovered/ylDlyh.png)](_static/recovered/ylDlyh.png)

```python
from ultralytics import YOLO
from pathlib import Path
# 1. 加载训练好的最佳权重文件
model_path = "butter_train_results/yolov8n_butter/weights/best.pt"
model = YOLO(model_path)
# 2. 导出为 RKNN 兼容的 ONNX 模型
# 关键参数说明：
# format='rknn': 指定导出格式为 rknn (实际上会先导出为特殊的 onnx)
success = model.export(format="rknn")
print(f"导出完成: {success}")
```

```{warning}
必须使用 ultralytics rknn 修改版才能正确导出 RKNN 模型，导出的模型必须是唯一的 .onnx 文件。
```

[![转换结果](_static/recovered/ylDtge.png)](_static/recovered/ylDtge.png)

## 3. Ubuntu 环境实现模型转换

### 3.1 docker 安装 RKNN-Toolkit2

因为在宿主机配置 RKNN-Toolkit2 环境极易出错，所以建议在 Ubuntu 环境下使用 Docker 容器来运行 RKNN-Toolkit2。

1. 下载并安装 Docker

```shell
# 1. 安装Docker并配置自启
sudo apt update -y

# 安装apt依赖工具（允许HTTPS访问仓库、解压缩等）
sudo apt install -y ca-certificates curl gnupg lsb-release git

# 创建密钥存储目录
sudo mkdir -p /etc/apt/trusted.gpg.d

# 下载并添加Docker官方GPG密钥（使用阿里云镜像修复SSL连接问题）
curl -fsSL https://mirrors.aliyun.com/docker-ce/linux/ubuntu/gpg | sudo gpg --dearmor -o /etc/apt/trusted.gpg.d/docker.gpg

# 配置Docker仓库源（使用阿里云镜像加速）
echo \
  "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/trusted.gpg.d/docker.gpg] https://mirrors.aliyun.com/docker-ce/linux/ubuntu \
  $(lsb_release -cs) stable" | sudo tee /etc/apt/sources.list.d/docker.list > /dev/null

# 更新软件包索引
sudo apt update

# 安装Docker核心组件（engine+cli+containerd）
sudo apt install -y docker-ce docker-ce-cli containerd.io

# 启动Docker服务（若未自动启动）
sudo systemctl start docker

# 设置Docker开机自启
sudo systemctl enable docker

# 配置 Docker 镜像加速（解决拉取镜像失败/速度慢的问题）
sudo mkdir -p /etc/docker
sudo tee /etc/docker/daemon.json <<-'EOF'
{
  "registry-mirrors": [
    "https://docker.m.daocloud.io",
    "https://huecker.io"
  ]
}
EOF
sudo systemctl daemon-reload
sudo systemctl restart docker

# 验证Docker是否正常运行（输出"Hello from Docker!"即成功）
sudo docker run --rm hello-world

# 将当前用户添加到docker组
sudo usermod -aG docker $USER

# 刷新组权限（无需重启，立即生效）
newgrp docker

# 验证：无需sudo执行docker命令
docker ps
```

[![安装docker并配置自启](_static/recovered/ylDKoa.png)](_static/recovered/ylDKoa.png)

2. 下载 rknn-toolkit2 的 Dockerfile

```shell
# 1. 下载 rknn-toolkit2 的 Dockerfile
# 创建目录并进入
sudo mkdir -p /opt/rknn-toolkit2/tools-file
cd /opt/rknn-toolkit2/tools-file

# 克隆仓库（使用 Gitee 镜像加速下载）
sudo git clone https://gitee.com/cddssgl/rknn-toolkit2.git
cd rknn-toolkit2/rknn-toolkit2/docker/docker_file/ubuntu_20_04_cp38

# 2. 修改 Dockerfile 源（解决 pip 安装依赖失败问题）
sudo sed -i 's|mirror.baidu.com|mirrors.aliyun.com|g' Dockerfile_ubuntu_20_04_for_cp38

# 3. 构建镜像
sudo docker build -f Dockerfile_ubuntu_20_04_for_cp38 -t rknn-toolkit2:cp38-2.3.2 .

# 4. 验证镜像是否构建成功
sudo docker images | grep rknn-toolkit2:cp38-2.3.2
```

[![构建rknn-toolkit2镜像](_static/recovered/ylDGWC.png)](_static/recovered/ylDGWC.png)

3. 挂载启动 docker 容器

```shell
# 1.先创建本地工作目录
sudo mkdir -p /opt/rknn-toolkit2/workspace
cd /opt/rknn-toolkit2/workspace

# 2.启动命令（挂载本地目录+映射 8888 端口）
sudo docker run -it --privileged --rm \
  -v /opt/rknn-toolkit2/workspace:/root/workspace \
  -p 8888:8888 \
  --name rknn-cp38-container \
  rknn-toolkit2:cp38-2.3.2 /bin/bash

# 3.进入容器后执行以下命令
python3 --version
python3 -c "from rknn.api import RKNN; print('RKNN 导入成功！版本：2.3.2')"
```

[![挂载启动rknn-toolkit2容器](_static/recovered/ylDmu4.png)](_static/recovered/ylDmu4.png)

4. 常用容器管理命令

```shell
# 1. 列出运行中的容器
docker ps

# 2. 停止容器（替换为容器名称/ID）
docker stop rknn-cp38-container

# 3. 重新启动已停止的容器
docker start rknn-cp38-container

# 4. 进入已运行的容器（再次打开终端）
docker exec -it rknn-cp38-container /bin/bash

# 5. 删除镜像（无需使用时清理磁盘空间）
docker rmi rknn-toolkit2:cp38-2.3.2
```

### 3.2 转换模型

```{note}
此时环境为 Ubuntu 主机环境，容器只是用于执行转换脚本，所有模型文件挂载在主机目录下 `/opt/rknn-toolkit2/workspace` 中。
```

```shell
# 1. 克隆 rknn_model_zoo 仓库，使用脚本文件转换模型
cd /opt/rknn-toolkit2/workspace
sudo git clone https://gitee.com/airockchip/rknn_model_zoo.git

cd rknn_model_zoo/examples/yolov8/python

# 2. 上传模型文件和黄油图片到ubuntu目录
chmod +x best.onnx
sudo cp best.onnx /opt/rknn-toolkit2/workspace/rknn_model_zoo/examples/yolov8/python/
```

```{note}
下面环境为 rknn-toolkit2:cp38-2.3.2 容器内，需要在容器内执行转换脚本。
```

```shell
cd /root/workspace/rknn_model_zoo/examples/yolov8/python
# pip uninstall onnx  # (可选)：可能会因为onnx版本过高导致转换失败
# pip install "onnx==1.14.1" # (可选)
python convert.py best.onnx rk3588

# 转换完成后，将模型文件复制到主机目录下
cd /root/workspace/rknn_model_zoo/examples/yolov8/model
sudo cp yolov8n.rknn /home/k/
```

[![转换yolov8n模型](_static/recovered/ylDXKH.png)](_static/recovered/ylDXKH.png)

[![转换完成的模型位置](_static/recovered/ylD3gX.png)](_static/recovered/ylD3gX.png)

## 4. 端侧测试

```{note}
下面环境为端侧环境，使用 RKNN-Toolkit-Lite2 python 接口测试模型。
```

```shell
# 1. 安装 RKNN-Toolkit-Lite2 python 包
pip3 install rknn-toolkit-lite2

# 2. 测试是否安装成功
python3 -c "from rknnlite.api import RKNNLite; print('RKNNLite 导入成功！版本：2.3.2')"

# 3. 上传模型文件和测试图片到端侧目录
mkdir -p /opt/rknn-toolkit2-lite
cd /opt/rknn-toolkit2-lite

cp /mnt/nfs/hy_ros/source/0.RK3588S/3.butter_yolov8n/yolov8.rknn /opt/rknn-toolkit2-lite/
cp /mnt/nfs/hy_ros/source/0.RK3588S/3.butter_yolov8n/butter.jpg /opt/rknn-toolkit2-lite/
cp /mnt/nfs/hy_ros/source/0.RK3588S/3.butter_yolov8n/test.py /opt/rknn-toolkit2-lite/

# 4. 运行测试脚本
python3 test.py
```

[![测试结果](_static/recovered/ylDiNF.jpg)](_static/recovered/ylDiNF.jpg)

## 5. 人手跟随(拓展)

### 5.1 基本原理

### 5.2 模型部署

# 六、yolov8n 目标检测部署

## 1. 基本原理

整个 **rk_camera** 到 **rk_inference** 再到 **rk_streamer** 的流程设计，充分利用了 RK3588S 的异构计算架构（ISP、RGA、NPU、VPU），实现了端到端的低延迟和高效处理。利用 ROS2 的组件设计，使用 UniquePtr 实现多个节点之间的零拷贝数据传输，避免了 CPU 拷贝带来的性能损失，可大幅提升系统吞吐量。

**节点 1: rk_camera (采集)**

| **流程描述**       | **关键代码 / 工具**          | **触发的硬件** | **硬件功能**                                                                                                                                        |
| ------------------ | ---------------------------- | -------------- | --------------------------------------------------------------------------------------------------------------------------------------------------- |
| **GStreamer 抓取** | v4l2src (GStreamer 插件)     | **ISP**        | 摄像头传感器进来的数据是 Raw 格式，**ISP** 硬件自动将其去噪、白平衡、去马赛克，输出 NV12 数据。                                                     |
| **数据传输**       | 内核驱动层 (V4L2 Driver)     | **DMA**        | **DMA** 硬件悄悄地把 ISP 处理好的数据搬运到内存（RAM）中，CPU 此时在休息。                                                                          |
| **拷贝到 ROS msg** | memcpy (C++ 标准库)          | **CPU**        | **这里是唯一的 CPU 重活**。CPU 把 GStreamer 的 Buffer 里的数据拷贝到 ROS 消息的 std::vector 中。（注：这是为了生成 ROS 消息，后续传输才是零拷贝）。 |
| **发布消息**       | pub->publish(std::move(msg)) | **(无)**       | 这是一个软件行为。std::move 转移了指针所有权，**避免了** CPU 进行节点间的数据拷贝。                                                                 |

**节点 2: rk_inference (推理与绘图 -- 核心)**

| **流程描述**        | **关键代码 / 工具**                              | **触发的硬件** | **硬件在做什么？**                                                                                                                           |
| ------------------- | ------------------------------------------------ | -------------- | -------------------------------------------------------------------------------------------------------------------------------------------- |
| **准备 Buffer**     | wrapbuffer_virtualaddr(...)                      | **(无)**       | 纯软件动作。只是告诉 RGA 驱动："数据在这个内存地址，你记一下"。                                                                              |
| **缩放 + 格式转换** | **imresize(src, dst)** 或 imcvtcolor (librga 库) | **RGA**        | **核心触发点！** 这行代码向 RGA 寄存器发送指令。**RGA 硬件**启动，瞬间把 1080P NV12 读入，处理成 640x640 RGB，写回内存。CPU 等待或做别的事。 |
| **NPU 推理**        | **rknn_run(ctx, ...)** (rknn_api 库)             | **NPU**        | **核心触发点！** 这行代码唤醒 **NPU 硬件**。NPU 加载模型权重，对 640x640 的数据进行数万亿次矩阵运算。                                        |
| **后处理**          | if (conf > 0.5) ... (C++ 逻辑)                   | **CPU**        | 解析 NPU 输出的浮点数，计算坐标 (x,y,w,h)。这是简单的逻辑判断，CPU 做这个很快。                                                              |
| **RGA 绘图**        | **imdrawrect(src, rect, ...)** (librga 库)       | **RGA**        | **核心触发点！** 代码告诉 **RGA 硬件**："在 src 这个 NV12 图片的 (x,y) 位置，把像素值改成红色"。RGA 直接修改内存，不用 CPU 逐个像素去画。    |
| **发布**            | pub->publish(std::move(msg))                     | **(无)**       | 指针传递，零拷贝。                                                                                                                           |

**节点 3: rk_streamer (推流)**

| **流程描述** | **关键代码 / 工具**                    | **触发的硬件**   | **硬件在做什么？**                                                                                                                                      |
| ------------ | -------------------------------------- | ---------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **写入管道** | writer.write(image) (OpenCV) -> appsrc | **CPU/DMA**      | 数据进入 GStreamer 的管道系统。                                                                                                                         |
| **硬件编码** | Pipeline 字符串中的 **mpph264enc**     | **VPU (RKVENC)** | **核心触发点！** mpph264enc 插件调用 MPP 库，MPP 库驱动 **VPU 硬件**。VPU 读取 NV12 原始数据，根据 H.264 算法进行帧内预测、运动估计，输出压缩后的码流。 |
| **网络发送** | rtspclientsink                         | **CPU/网卡**     | CPU 将压缩好的小数据包通过网络协议栈发给网卡。                                                                                                          |

## 2. 端侧依赖安装

在 RK3588S 端侧（Ubuntu 系统）上，需要安装以下基础库：

### 2.1 安装 GStreamer 和 rga

```bash
# 1. 安装 GStreamer 及其插件（包含 app 库）
sudo sed -i 's/mirrors.tuna.tsinghua.edu.cn/mirrors.ustc.edu.cn/g' /etc/apt/sources.list
sudo apt-get update

# 编译安装 Rockchip 多媒体组件
sudo apt-get install -y --fix-missing gcc g++ cmake meson ninja-build \
    libdrm-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
    gstreamer1.0-plugins-base gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly \
    gstreamer1.0-tools gstreamer1.0-rtsp

# 2. 创建工作目录
sudo mkdir -p /opt/rk_multimedia
sudo chown $USER:$USER /opt/rk_multimedia
cd /opt/rk_multimedia

# 3. 编译安装 MPP (Media Process Platform)
git clone https://github.com/rockchip-linux/mpp.git --depth=1
cd mpp
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
sudo make install
sudo ldconfig
cd ../..

# 4. 安装 librga (使用预编译库)
git clone https://gitee.com/airockchip/librga.git --depth=1
cd librga
sudo cp -r include/* /usr/local/include/
sudo cp -r libs/Linux/gcc-aarch64/* /usr/local/lib/

# 手动创建 pkgconfig 文件
sudo mkdir -p /usr/local/lib/pkgconfig
sudo tee /usr/local/lib/pkgconfig/librga.pc > /dev/null <<EOF
prefix=/usr/local
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include

Name: librga
Description: Rockchip RGA 2D graphics acceleration library
Version: 1.10.0
Libs: -L\${libdir} -lrga
Cflags: -I\${includedir}
EOF

sudo ldconfig
cd ..

# 5. 编译安装 gstreamer-rockchip 插件
git clone https://gitee.com/emancipator/gstreamer-rockchip.git gstreamer-rockchip
cd gstreamer-rockchip
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH
meson setup build --prefix=/usr --buildtype=release
ninja -C build
sudo ninja -C build install

# 6. 更新库缓存并验证
gst-inspect-1.0 | grep mpp
```

### 2.2 安装 RTSP 推流服务器

```bash
# 1. 安装 ROS2 依赖
sudo apt-get install -y ros-humble-image-transport ros-humble-cv-bridge

# 2. 安装 RTSP 推流服务器 (go2rtc)
wget https://github.com/AlexxIT/go2rtc/releases/latest/download/go2rtc_linux_arm64 -O /usr/local/bin/go2rtc
chmod +x /usr/local/bin/go2rtc

# 3. 创建配置文件 /etc/go2rtc.yaml
sudo bash -c 'cat <<EOF > /etc/go2rtc.yaml
streams:
  camera: null

api:
  listen: ":1984"

rtsp:
  listen: ":8554"

webrtc:
  listen: ":8555"
  candidates:
    - 192.168.22.219:8555 # 请修改为你的板卡实际 IP
EOF'

# 4. 设置go2rtc开机自启动 (Systemd 服务)
sudo bash -c 'cat <<EOF > /etc/systemd/system/go2rtc.service
[Unit]
Description=Go2RTC Media Server
After=network.target

[Service]
ExecStart=/usr/local/bin/go2rtc -c /etc/go2rtc.yaml
WorkingDirectory=/etc
Restart=always
User=root

[Install]
WantedBy=multi-user.target
EOF'

# 5. 启用并启动服务
sudo systemctl daemon-reload
sudo systemctl enable go2rtc
sudo systemctl start go2rtc
```

## 3. RKNN-NPU 部署

在进行 NPU 推理开发前，必须确保系统已启用 RGA 硬件加速驱动，并安装了用户态库文件 (`librga`)。
RGA (Raster Graphic Acceleration Unit)是一个独立的 2D 硬件加速器，可用于加速点/线绘制，执行图像缩放、旋转、bitBlt、alpha 混合等常见的 2D 图形操作。

[![测试结果](_static/recovered/ylMjpO.png)](_static/recovered/ylMjpO.png)

## 4. RTSP 推流

当 `rk_streamer` 正常运行后，它会主动将视频流推送到 `rtsp://127.0.0.1:8554/camera`。
在网页输入 `http://192.168.22.219:1984/stream.html?src=camera` 即可查看推流视频。

[![测试结果](_static/recovered/ylMbvL.png)](_static/recovered/ylMbvL.png)

# 七、SLAM 部署

本节介绍如何在端侧 RK3588S 上部署 SLAM 建图功能，包括在线建图、地图发布与雷达数据裁剪。

## 1. 安装依赖

在端侧（RK3588S）执行以下命令安装必要功能包：

```bash
sudo apt update
sudo apt install -y ros-humble-slam-toolbox ros-humble-navigation2 \
    ros-humble-nav2-bringup ros-humble-laser-filters \
    ros-humble-robot-localization
```

## 2. 配置功能包

为了实现"在线建图"、"发布 map->odom 变换"以及"雷达后方裁剪"的功能，需进行以下配置。

### 2.1 雷达数据裁剪 (Laser Filters)

为解决机器人后方遮挡问题（例如自身结构遮挡），需配置 `laser_filters` 对雷达数据进行裁剪。
新建配置文件 `src/hy_slam/config/laser_filter.yaml`：

```yaml
scan_to_scan_filter_chain:
  ros__parameters:
    filter1:
      name: rear_box_filter
      type: laser_filters/LaserScanBoxFilter
      params:
        box_frame: base_link
        max_x: -0.1
        min_x: -1.0
        max_y: 0.3
        min_y: -0.3
        max_z: 1.0
        min_z: -1.0
        invert: false
```

### 2.2 SLAM 配置 (Slam Toolbox)

配置 `slam_toolbox` 进行在线建图与地图发布。`slam_toolbox` 在 mapping 模式下会自动发布 `map` -> `odom` 的 TF 变换。
新建配置文件 `src/hy_slam/config/mapper_params_online_async.yaml`

```yaml
slam_toolbox:
  ros__parameters:
    odom_frame: odom
    map_frame: map
    base_frame: base_link
    scan_topic: /scan_filtered # 注意：订阅经过裁剪后的雷达话题
    mode: mapping

    # 更多参数参考模板...
    use_scan_matching: true
    map_update_interval: 5.0
    resolution: 0.05
```

### 2.3 定位参数配置 (Slam Toolbox)

配置 `slam_toolbox` 进行定位。`slam_toolbox` 在 localization 模式下会根据静态地图与实时雷达数据进行匹配，发布 `map` -> `odom` 的 TF 变换。
新建配置文件 `src/hy_slam/config/mapper_params_localization.yaml`

```yaml
slam_toolbox:
  ros__parameters:
    odom_frame: odom
    map_frame: map
    base_frame: base_link
    scan_topic: /scan_filtered
    mode: localization

    # 更多参数参考模板...
    use_scan_matching: true
    map_update_interval: 5.0
    resolution: 0.05
```

## 3. SLAM 在线建图/存图

- `slam.launch.py` 启动 SLAM 建图功能，实现在线建图。调用 `save_map.sh` 脚本保存地图。
- `localization.launch.py` 启动雷达数据裁剪+定位功能，根据静态地图与实时雷达数据进行匹配，发布 `map` -> `odom` 的 TF 变换。
- `laser_filter_only.launch.py` 仅启动雷达数据裁剪功能，将 `/scan` 话题裁剪为 `/scan_filtered`，依赖 Nav2 功能包的 AMCL + Map Server，发布 `map` -> `odom` 的 TF 变换。

1. 启动雷达功能包

```bash
cd ~/hy_linux/nfs/hy_ros

# 注意：若未配置udev规则，请先执行，然后重新插拔雷达
sudo bash src/ldlidar_driver_ros2/scripts/Ldlidar_udev.sh

# 1. 编译
colcon build --packages-select ldlidar_driver_ros2

# 2. 运行雷达
source install/setup.bash
ros2 launch ldlidar_driver_ros2 ldlidar_driver.launch.py
```

2. 启动 SLAM 建图功能

```bash
# 编译
cd ~/hy_linux/nfs/hy_ros
colcon build --packages-select hy_slam

# 运行
source install/setup.bash
ros2 launch hy_slam slam.launch.py
```

3. 保存地图

```{warning}
请在 `slam.launch.py` 仍在运行时执行保存脚本。若先终止 SLAM 程序，内存中的地图数据将丢失，导致无法保存。
```

完成建图后，打开一个新的终端执行以下脚本：

```bash
# 默认保存为 my_map
cd ~/hy_linux/nfs/hy_ros
./src/hy_slam/scripts/save_map.sh

# 或指定地图名
# ./src/hy_slam/scripts/save_map.sh room_1
```

地图将保存至 `install/hy_slam/share/hy_slam/map/` 目录下（包含 `.pgm` 和 `.yaml` 两个文件）。

## 4. 可视化调试 (Foxglove)

使用 Foxglove Studio 查看实时建图效果。

1. **客户端连接**：
   - 在 PC 上打开 Foxglove Studio。
   - 选择 **Open Connection** -> **Foxglove WebSocket**。
   - 地址输入：`ws://<机器人IP>:8765`。

2. **配置面板**：
   - 添加 **2D Panel**。
   - 在左侧 Topics 勾选：
     - `/map` (SLAM 地图)
     - `/scan` (原始雷达)
     - `/scan_filtered` (裁剪后雷达)
     - TF 树 (确保 `map` -> `odom` -> `base_link` -> `radar_Link` 连通)。

/scan 雷达数据可视化

[![历史雷达可视化截图](_static/recovered/ylMzsx.png)](_static/recovered/ylMzsx.png)

/map 地图可视化

> **历史截图说明**：旧版 Git 文档在 `/scan` 和 `/map` 两处引用同一张雷达截图。下图按历史位置恢复，不能作为独立的地图效果截图；地图截图仍待补充。

[![历史文档共用的雷达截图（非独立地图截图）](_static/recovered/ylMzsx.png)](_static/recovered/ylMzsx.png)

## 5. 定位模式

若已构建地图，可切换至纯定位模式运行（不再更新地图，仅进行定位）：

1. **方式一：使用 slam_toolbox 进行定位**

   该模式下，`slam_toolbox` 会加载静态地图，并将实时雷达数据与地图匹配，发布 `map` -> `odom` 变换。

   ```bash
   # 启动定位模式（默认加载 my_map）
   ros2 launch hy_slam localization.launch.py
   ```

2. **方式二：使用 Nav2 (AMCL) 进行定位**（推荐用于导航）

   若后续使用 Nav2 导航栈，通常由 `amcl` 节点负责定位，`map_server` 负责发布地图。此时 `hy_slam` 仅需提供雷达数据裁剪。

   - **步骤 1**：启动雷达滤波器（仅发布 /scan_filtered）

     ```bash
     ros2 launch hy_slam laser_filter_only.launch.py
     ```

   - **步骤 2**：启动 Nav2 (AMCL + Map Server)

     _(需在 Nav2 启动文件中配置加载 `install/hy_slam/share/hy_slam/map/my_map.yaml`)_

```{important}
无论哪种方式，定位模式下**必须**发布雷达数据 (`/scan_filtered`)。
```

# 八、Nav2 部署

本节介绍如何部署 Nav2 导航栈，实现"搜索黄油 → 导航抓取 → 返回起点"的自主任务。

## 1. 端侧安装依赖

在端侧（RK3588S）执行以下命令安装 Nav2 导航相关功能包：

```bash
sudo apt update
sudo apt install -y \
    ros-humble-navigation2 \
    ros-humble-nav2-bringup \
    ros-humble-nav2-simple-commander
```

```{important}
本章节依赖第七章 SLAM 部署的功能包（`slam-toolbox`、`robot-localization`、`laser-filters`），若未安装请先完成第七章配置。同时需要第六章的视觉检测系统、底盘通信功能包（`uart`）和里程计融合功能包（`ekf_odom`）正常运行。
```

### 1.2 前置条件检查

在开始 Nav2 部署前，请确认以下条件已满足：

- ✅ 已完成第七章 SLAM 建图，地图文件保存在 `install/hy_slam/share/hy_slam/map/`
- ✅ 已部署第六章视觉检测系统（YOLOv8 + RKNN NPU）
- ✅ 底盘串口通信正常（UART 与 ESP32S3）
- ✅ 里程计融合节点正常运行（EKF）
- ✅ 雷达数据正常发布（`/scan` 话题）

## 2. 系统架构设计

### 1.1 整体架构

导航系统采用分层设计，从上到下依次为：

```text
┌─────────────────────────────────────────────────────────┐
│  任务调度层 (Mission Layer)                              │
│  - 状态机管理 (搜索/导航/抓取/返回)                       │
│  - 目标点规划与发布                                       │
│  - 视觉检测结果订阅                                       │
└─────────────────────────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────┐
│  导航规划层 (Nav2 Stack)                                 │
│  - 全局路径规划 (NavFn)                                  │
│  - 局部轨迹规划 (DWB)                                    │
│  - 行为协调 (Behavior Tree)                              │
└─────────────────────────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────┐
│  定位感知层 (Localization & Perception)                  │
│  - AMCL 粒子滤波定位                                     │
│  - 代价地图构建 (静态层+障碍层+膨胀层)                    │
│  - 雷达数据过滤 (laser_filters)                          │
└─────────────────────────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────┐
│  执行控制层 (Control Layer)                              │
│  - 速度指令发布 (/cmd_vel)                               │
│  - 底盘运动控制 (UART → ESP32S3)                         │
│  - 舵机控制 (抓取动作)                                    │
└─────────────────────────────────────────────────────────┘
```

### 1.2 关键技术点

- **TF 坐标变换链**：`map` → `odom` → `base_link` → `laser_link`
- **零拷贝通信**：视觉检测节点与导航系统通过 ROS2 零拷贝传递目标位置
- **代价地图融合**：静态地图 + 实时雷达障碍 + 安全膨胀区域

## 2. 任务流程设计

### 2.1 状态机设计

任务执行采用有限状态机（FSM）管理，共 6 个状态：

|          状态          | 说明          | 触发条件     | 输出动作           |
| :--------------------: | :------------ | :----------- | :----------------- |
|        **IDLE**        | 初始空闲      | 启动时       | 记录起点位姿       |
|    **GO_TO_CENTER**    | 前往搜索中心  | 起点记录完成 | 发布导航目标       |
|     **SEARCHING**      | 360° 旋转搜索 | 到达中心点   | 发布旋转速度指令   |
| **NAVIGATE_TO_BUTTER** | 导航到黄油    | 检测到黄油   | 计算目标位姿并导航 |
|      **GRASPING**      | 执行抓取      | 距离 < 0.3m  | 发布舵机控制指令   |
|    **RETURN_HOME**     | 返回起点      | 抓取完成     | 导航回起点         |

### 2.2 完整任务流程

```text
[启动]
  ↓
[记录当前位置作为起点]
  ↓
[导航到预设中心点 (center_x, center_y)]
  ↓
[原地旋转 360° 搜索黄油]
  ↓
[检测到黄油?] ──否──→ [继续旋转]
  ↓ 是
[停止旋转，计算黄油全局坐标]
  ↓
[导航到黄油位置]
  ↓
[距离 < 0.3m?] ──否──→ [继续接近]
  ↓ 是
[停止移动，发送抓取指令]
  ↓
[等待 3 秒（抓取动作执行）]
  ↓
[导航返回起点]
  ↓
[任务完成]
```

## 3. Nav2 核心组件

### 3.1 定位模块 (AMCL)

**功能**：基于粒子滤波的蒙特卡洛定位算法，融合雷达数据与里程计实现机器人在地图中的定位。

**关键参数**：

- `max_particles: 2000`：粒子数量，影响定位精度和计算量
- `laser_model_type: likelihood_field`：激光模型类型
- `update_min_d: 0.25`：移动 0.25m 触发一次更新
- `update_min_a: 0.2`：旋转 0.2rad 触发一次更新

**输出**：发布 `map` → `odom` 的 TF 变换

### 3.2 全局规划器 (NavFn Planner)

**功能**：基于 Dijkstra 算法在静态地图上规划从起点到终点的最优路径。

**特点**：

- 考虑静态障碍物和膨胀区域
- 生成平滑的全局路径
- 路径更新频率：20Hz

### 3.3 局部规划器 (DWB Controller)

**功能**：动态窗口法（Dynamic Window Approach）局部轨迹规划，实时避障。

**工作原理**：

1. 在速度空间采样多条候选轨迹（vx, vy, vθ）
2. 对每条轨迹评分（路径跟随、目标接近、障碍避让）
3. 选择得分最高的轨迹执行

**关键参数**：

- `max_vel_x: 0.15`：最大线速度 (m/s)
- `max_vel_theta: 1.0`：最大角速度 (rad/s)
- `sim_time: 1.5`：轨迹预测时间 (s)

### 3.4 代价地图 (Costmap 2D)

**功能**：构建机器人周围环境的代价地图，用于路径规划和避障。

**三层结构**：

1. **静态层 (Static Layer)**：加载 SLAM 构建的静态地图
2. **障碍层 (Obstacle Layer)**：实时雷达检测的动态障碍物
3. **膨胀层 (Inflation Layer)**：障碍物周围膨胀安全区域

**全局 vs 局部代价地图**：

- 全局代价地图：覆盖整个地图，用于全局路径规划
- 局部代价地图：3m×3m 滚动窗口，用于局部避障

## 4. 功能包结构

### 4.1 hy_nav2 功能包目录结构

```text
hy_nav2/
├── config/
│   ├── nav2_params.yaml          # Nav2 核心参数配置
│   └── map_server_params.yaml    # 地图服务器配置
├── launch/
│   ├── nav2_bringup.launch.py    # Nav2 导航栈启动
│   └── butter_mission.launch.py  # 完整任务启动
├── hy_nav2/
│   ├── __init__.py
│   └── butter_mission_node.py    # 任务调度节点
├── package.xml
└── setup.py
```

### 4.2 核心文件说明

**nav2_params.yaml**

- 包含 AMCL、控制器、规划器、代价地图等所有 Nav2 组件的参数
- 需根据机器人实际尺寸调整 `robot_radius: 0.15`
- 速度限制需与底盘实际能力匹配

**butter_mission_node.py**

- 实现状态机逻辑
- 订阅 `/detections` 话题获取目标检测结果
- 发布 `/cmd_vel` 控制机器人运动
- 调用 Nav2 的 `NavigateToPose` Action 进行导航

**nav2_bringup.launch.py**

- 启动地图服务器（加载 SLAM 地图）
- 启动 AMCL 定位节点
- 启动 Nav2 导航栈（规划器、控制器、行为服务器）

## 5. 开发流程

### 5.1 配置文件编写

**步骤 1：创建 Nav2 参数文件**

在 `config/nav2_params.yaml` 中配置：

- AMCL 定位参数（粒子数、更新阈值）
- 控制器参数（速度限制、加速度限制）
- 规划器参数（路径规划算法）
- 代价地图参数（分辨率、膨胀半径）

```{tip}
可参考 Nav2 官方示例：`/opt/ros/humble/share/nav2_bringup/params/nav2_params.yaml`
```

**步骤 2：适配机器人参数**

根据黄油机器人实际情况调整：

- `robot_radius: 0.15`（机器人半径，单位：米）
- `max_vel_x: 0.15`（最大线速度，与底盘能力匹配）
- `inflation_radius: 0.35`（障碍物膨胀半径，确保安全距离）

### 5.2 任务调度节点开发

**核心功能实现**：

1. **状态机管理**：使用 Python Enum 定义状态，在定时器回调中切换状态
2. **Action Client**：调用 Nav2 的 `NavigateToPose` Action 发送导航目标
3. **视觉融合**：订阅 `/detections` 话题，解析黄油位置信息
4. **坐标转换**：将相机坐标系下的目标位置转换为地图坐标系

**关键接口**：

- 订阅：`/detections` (目标检测结果)、`/amcl_pose` (当前位姿)
- 发布：`/cmd_vel` (速度控制)、`/joint_commands` (舵机控制)
- Action：`/navigate_to_pose` (导航目标)

### 5.3 启动文件编写

**nav2_bringup.launch.py 职责**：

- 启动 Map Server（加载 `hy_slam` 保存的地图）
- 启动 AMCL 定位节点
- 启动 Nav2 导航栈（通过 `navigation_launch.py`）
- 启动 Lifecycle Manager 管理节点生命周期

**butter_mission.launch.py 职责**：

- 包含 `nav2_bringup.launch.py`
- 启动任务调度节点 `butter_mission_node`
- 传递参数：中心点坐标、搜索速度等

## 6. 编译与运行

### 6.1 编译功能包

```bash
cd ~/hy_linux/nfs/hy_ros
colcon build --packages-select hy_nav2
source install/setup.bash
```

### 6.2 启动系统

**前置条件**：确保已完成 SLAM 建图并保存地图文件到 `install/hy_slam/share/hy_slam/map/`

```bash
# 终端 1：启动基础系统（雷达+底盘+里程计融合）
ros2 launch ldlidar_driver_ros2 ldlidar_driver.launch.py &
ros2 launch uart uart.launch.py &
ros2 launch ekf_odom ekf_odom.launch.py &
ros2 launch hy_slam laser_filter_only.launch.py

# 终端 2：启动视觉检测（摄像头+YOLOv8+推流）
ros2 launch rk_camera rk_camera.launch.py &
ros2 launch rk_inference rk_inference.launch.py &
ros2 launch rk_streamer rk_streamer.launch.py

# 终端 3：启动 Nav2 导航栈
ros2 launch hy_nav2 nav2_bringup.launch.py

# 终端 4：启动任务调度节点
ros2 launch hy_nav2 butter_mission.launch.py
```

```{tip}
可使用 `tmux` 或 `screen` 管理多个终端，或参考 `all_launch` 功能包编写统一的 launch 文件整合所有节点。
```

## 7. 调试与验证

### 7.1 检查系统状态

**验证 TF 树完整性**

```bash
# 生成 TF 树图
ros2 run tf2_tools view_frames

# 检查关键坐标变换
ros2 run tf2_ros tf2_echo map base_link
```

应包含完整链路：`map` → `odom` → `base_link` → `laser_link`

**检查话题通信**

```bash
# 查看导航相关话题
ros2 topic list | grep -E 'cmd_vel|odom|scan|map|amcl'

# 查看目标检测结果
ros2 topic echo /detections

# 查看机器人位姿
ros2 topic echo /amcl_pose
```

**检查 Action 服务**

```bash
# 查看可用 Action
ros2 action list

# 查看导航 Action 状态
ros2 action info /navigate_to_pose
```

### 7.2 常见问题排查

**问题 1：机器人不移动**

- 检查 `/cmd_vel` 话题是否有数据：`ros2 topic hz /cmd_vel`
- 检查 TF 树是否完整（特别是 `map` → `odom` 变换）
- 确认底盘串口通信正常

**问题 2：定位漂移**

- 增加 AMCL 粒子数：`max_particles: 2000 → 3000`
- 检查里程计数据质量：`ros2 topic echo /odom`
- 确认地图与实际环境一致

**问题 3：路径规划失败**

- 检查代价地图：`ros2 topic echo /global_costmap/costmap --once`
- 减小膨胀半径：`inflation_radius: 0.35 → 0.25`
- 确认目标点在可达区域内

**问题 4：搜索时未检测到黄油**

- 检查视觉检测节点是否运行：`ros2 node list | grep inference`
- 查看检测结果：`ros2 topic echo /detections`
- 调整搜索旋转速度：`search_angular_speed: 0.5 → 0.3`

### 7.3 Foxglove 可视化

启动 Foxglove Bridge 进行实时监控：

```bash
ros2 run foxglove_bridge foxglove_bridge --ros-args -p port:=8765
```

在浏览器打开 [Foxglove Studio](https://foxglove.dev/studio)，连接到 `ws://<机器人IP>:8765`

**推荐查看的面板**：

- `/map`：静态地图
- `/global_costmap/costmap`：全局代价地图
- `/local_costmap/costmap`：局部代价地图（3m×3m 滚动窗口）
- `/plan`：全局路径
- `/local_plan`：局部轨迹
- `/amcl_pose`：定位位姿
- TF 树：坐标变换关系

## 8. 参数调优建议

### 8.1 速度参数

根据实际测试调整机器人运动速度：

- `max_vel_x: 0.15`：最大线速度，过大可能导致失控
- `max_vel_theta: 1.0`：最大角速度，影响转弯灵活性
- `acc_lim_x: 0.5`：线加速度限制，避免急加速
- `acc_lim_theta: 1.5`：角加速度限制

### 8.2 目标容差

调整到达目标的精度要求：

- `xy_goal_tolerance: 0.15`：位置容差（米），过小可能无法到达
- `yaw_goal_tolerance: 0.25`：角度容差（弧度）

### 8.3 代价地图参数

- `robot_radius: 0.15`：机器人半径，需实测确定
- `inflation_radius: 0.35`：膨胀半径，确保安全距离
- `obstacle_max_range: 2.5`：障碍物检测最大距离