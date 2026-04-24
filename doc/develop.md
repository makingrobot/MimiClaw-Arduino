
## 快速开始

1. 安装依赖库（ArduinoJson、WebSockets、ESP32Console等）
2. 将 `persist-data/` 内的文件上传到存储中
3. 在mimi_secrets.h中填写你的 WiFi、Feishu、Telegram 和 LLM API 凭据
4. 在boards文件夹下创建你的开发板目录，新建开发板类（从Board类继承），实现板上相关硬件驱动
5. 编译、上传到 ESP32-S3 开发板


## 开发板设置（Arduino IDE）

| 设置项 | 值 |
|--------|------|
| 开发板 | ESP32S3 Dev Module |
| PSRAM | OPI PSRAM |
| Flash 大小 | 16MB (128Mb) |
| 分区方案 | Default with large SPIFFS 或 其它类似方案 |


## 系统架构

![架构图](../assets/arch.png)


## 依赖库

通过 Arduino 库管理器安装

基础依赖库：

- **ArduinoJson** v7.0+，https://github.com/bblanchon/ArduinoJson
- **WebSockets** v2.6+，https://github.com/Links2004/arduinoWebSockets
- **ESP32Console** v1.3, https://github.com/jbtronics/ESP32Console

使用GUI界面依赖库：

- **lvgl** v9.0 
- **FT6336** 
