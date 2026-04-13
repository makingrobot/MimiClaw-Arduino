### SPIFFS分区

spiffs_data文件夹内的文件是程序中要用到的，存放到spiffs分区。<br/>
spiffs分区还用于存储对话信息，记忆信息。

#### SPIFFS镜像生成和烧录步骤

* 1.生成镜像

使用spiffsgen.py

```shell
# 命令                大小      数据目录    镜像文件
python  spiffsgen.py  0x200000    spiffs_data     spiffs_data.bin
```

使用mkspiffs.exe
```shell  
# 命令         数据目录   块大小    页大小    镜像总大小    镜像文件
mkspiffs.exe  -c spiffs_data   -b 4096   -p 256   -s 0x200000   spiffs_data.bin
```
<i>mkspiffs.exe 在 ${HOME}\AppData\Local\Arduino15\packages\esp32\tools\mkspiffs 文件夹内</i>

* 2.烧录镜像

```shell
# 命令                串口名称                 起始地址    镜像文件
esptool.exe  --port /dev/ttyUSB0  write_flash  0xc90000   spiffs_data.bin
```
<i>esptool.exe 在 ${HOME}\AppData\Local\Arduino15\packages\esp32\tools\esptool 文件夹内</i>