# CardKB-driver
##linux driver for a i2c card_size keyboard
###适用于M5STACK家的卡片键盘，官方网站：https://docs.m5stack.com/en/unit/cardkb_1.1 可用于树莓派等嵌入式设备的简易键盘而不通过usb,使得设计更加简单
此款键盘使用i2c通讯，不同于以往键盘的每一个按键对应一个键值，通过shift切换，这款键盘每一个符号都对应一个键值，所以需要在驱动中添加特殊处理



