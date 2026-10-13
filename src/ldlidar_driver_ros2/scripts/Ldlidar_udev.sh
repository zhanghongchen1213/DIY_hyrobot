# 功能：配置 USB 雷达设备的 udev 规则
# 作用：将 Vendor=10c4, Product=ea60 的 USB 设备绑定固定名称为 /dev/ldlidar_serial
# 权限：设置权限为 0777，用户组 dialout
# 注意：此脚本必须以 sudo 权限运行

echo  'KERNEL=="ttyUSB*", ATTRS{idVendor}=="10c4", ATTRS{idProduct}=="ea60",ATTRS{serial}=="0001", MODE:="0777", GROUP:="dialout", SYMLINK+="ldlidar_serial"' >/etc/udev/rules.d/ldlidar_driver.rules

service udev reload
sleep 2
service udev restart


