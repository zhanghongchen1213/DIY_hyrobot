# 主机环境配置

准备一台 Ubuntu 22.04 主机，并让主机与机器人的 RK3588S 板卡连接同一局域网。本节先准备代码与共享目录，安装 ROS 2 Humble；端侧完成[环境配置](device_setup.md)后，再检查双端通信。

## 准备代码与模型

若已完成[资料获取](../7_Resources.md)中的克隆与下载，跳过本小节，直接检查下面的目录。

在主机执行：

```bash
mkdir -p "$HOME/hy_linux/nfs"
cd "$HOME/hy_linux/nfs"
git clone https://github.com/zhanghongchen1213/DIY_hyrobot.git
cd DIY_hyrobot
python3 hy_ros/tools/fetch_source.py --group all
cd ..
ln -s DIY_hyrobot/hy_ros hy_ros
```

下载完成后检查 `~/hy_linux/nfs/hy_ros/src/` 和 `~/hy_linux/nfs/hy_ros/source/` 是否存在。模型与测试资料的分组下载方法见[资料获取](../7_Resources.md)。已完成这一步时，后续直接使用该目录。

## 配置 NFS 共享

已在资料准备时配置 NFS 的主机，只需运行 `sudo exportfs -v` 检查共享规则，并记下主机 IP 和共享路径，无需重复写入配置。

将命令中的示例网段替换为主机与机器人所在的局域网网段：

```bash
sudo apt install -y nfs-kernel-server
printf '%s 192.168.22.0/24(rw,sync,no_subtree_check)\n' "$HOME/hy_linux/nfs" | sudo tee /etc/exports.d/diy_hyrobot.exports
sudo exportfs -ra
sudo systemctl restart nfs-kernel-server
sudo exportfs -v
hostname -I
```

记下主机 IP 和共享目录的绝对路径，端侧挂载时会用到它们。`exportfs -v` 应显示 `hy_linux/nfs` 的共享规则。

## 安装 ROS 2 Humble

参考教程：[鱼香 ROS2 一键安装](https://blog.csdn.net/weixin_71683006/article/details/150465016?ops_request_misc=elastic_search_misc&request_id=dd0d7d3fd479c65031b7b2767c112822&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-150465016-null-null.142^v102^pc_search_result_base3&utm_term=%E9%B1%BC%E9%A6%99ros2%E4%B8%80%E9%94%AE%E5%AE%89%E8%A3%85&spm=1018.2226.3001.4187)

```shell
cd /opt
sudo wget --no-check-certificate https://fishros.com/install -O fishros
sudo chmod +x fishros
sudo ./fishros
# 选择一键安装 ROS，按提示配置软件源并安装 Humble 桌面版。
```

[![ROS2 安装界面](../_static/recovered/ylDjRa.png)](../_static/recovered/ylDjRa.png)

## 安装语音管理依赖

需要在主机上创建或管理音色时，安装以下 Python 库：

```shell
sudo apt-get install python3-aiohttp python3-websockets
```

## 完成后检查

端侧也安装 ROS 2 后，在两端终端执行 `source /opt/ros/humble/setup.bash`，再分别运行：

- 端侧：`ros2 run demo_nodes_cpp talker`
- 主机侧：`ros2 run demo_nodes_cpp listener`

主机持续收到 `Hello World` 消息，即完成双端通信检查。若没有消息，先检查两端 IP、网络连通性及 `ROS_DOMAIN_ID` 是否一致。

[![通信测试](../_static/recovered/ylDFP4.png)](../_static/recovered/ylDFP4.png)

接着完成[端侧环境配置](device_setup.md)。
