# CardKB-driver

![](https://static-cdn.m5stack.com/resource/docs/products/unit/cardkb_1.1/cardkb_1.1_02.webp)

本驱动为M5STACK的卡片键盘 官网：https://docs.m5stack.com/en/unit/cardkb_1.1
设计，方便树莓派等linux嵌入式设备使用小巧的键盘而不使用usb，这会更加简洁

该键盘使用i2c通讯，不同于以往键盘一个按键一个键值由shift切换，该键盘每一个符号都对应一个键值，这超出了linux输入子系统所包含的键值，所以在驱动中使用了特殊处理，

## 使用教程
###1.下载源码
###2.配置并编译设备数
###3.设置overlays
###4.编译模块
###5.安装模块
