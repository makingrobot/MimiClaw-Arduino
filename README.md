# MimiClaw Arduino

在 ESP32-S3（带 PSRAM）上运行 MimiClaw AI Agent 的 Arduino 程序。<br/>
基于ESP32-Arduino-Framework开发框架。

## 功能特性

- **ReAct 智能体循环** — 自主 AI 智能体，支持工具调用（最多 10 次迭代）
- **双 LLM 支持** — 同时支持 Anthropic Claude 和 OpenAI/Deepseek GPT 模型
- **Telegram 机器人** — 长轮询方式集成 Telegram Bot
- **Feishu 机器人** — 长轮询方式集成 飞书 Bot
- **WebSocket 服务器** — 实时通信网关
- **持久化记忆** — 基于 SPIFFS 的长期记忆与每日笔记
- **会话管理** — 按聊天分组的对话历史（JSONL 格式）
- **九个内置工具** — 网页搜索、文件读写、时间获取、定时任务调度
- **技能系统** — 从 SPIFFS 加载的可扩展技能文件
- **定时任务调度** — 支持循环和一次性定时任务
- **心跳检测** — 定期检查 HEARTBEAT.md 文件
- **HTTP/SOCKS5 代理** — 所有 API 调用可选代理支持

## 硬件要求

- 带有 **PSRAM** 的 ESP32-S3 开发板（推荐：8MB 以上 PSRAM，16MB Flash）
- WiFi 网络连接

## 依赖库

通过 Arduino 库管理器安装：

- **ArduinoJson** v7.0+，作者 Benoit Blanchon
- **WebSockets** v2.4+，作者 Markus Sattler (links2004)

## 快速开始

1. 安装依赖库（ArduinoJson、WebSockets）
2. 使用 "ESP32 Sketch Data Upload" 工具将 `spiffs-data/` 文件夹上传到 SPIFFS
3. 在mimi_secrets.h中填写你的 WiFi、Feishu、Telegram 和 LLM API 凭据
4. 在boards文件夹下创建你的开发板目录，新建开发板类（从Board类继承），实现板上相关硬件驱动
5. 编写相关自定义工具
6. 编译、上传到 ESP32-S3 开发板

### 开发板设置（Arduino IDE）

| 设置项 | 值 |
|--------|------|
| 开发板 | ESP32S3 Dev Module |
| PSRAM | OPI PSRAM |
| Flash 大小 | 16MB (128Mb) |
| 分区方案 | Default with large SPIFFS 或 其它类似方案 |


### 自定义工具

#### 1.控制硬件输出的工具，如控制LED
```cpp
bool myOutToolExecute(const char* input_json, char* output, size_t output_size) {
    // 解析 input_json，执行操作，将结果写入 output
    snprintf(output, output_size, "工具执行结果");
    return true;
}
```

#### 2.从硬件获取数据的工具，如温湿度传感器数据
```cpp
bool myInToolExecute(const char* input_json, char* output, size_t output_size) {
    // 解析 input_json，执行操作，将结果写入 output
    snprintf(output, output_size, "工具执行结果");
    return true;
}
```

#### 3.注册工具到MimiClaw
```cpp
MimiTool myTool = {
    .name = "my_tool",
    .description = "描述这个工具的功能",
    .input_schema_json = "{\"type\":\"object\",\"properties\":{\"param\":{\"type\":\"string\"}},\"required\":[\"param\"]}",
    .execute = myToolExecute
};

mimi.tools().registerTool(&myTool);
```

## 系统架构
![架构图](assets/arch.png)


## SPIFFS 数据结构

使用 SPIFFS 上传工具将 `spiffs-data` 文件夹上传至设备：

```
/spiffs-data/
  config/
    SOUL.md          — AI 人格定义
    USER.md          — 用户信息（自动填充）
  memory/
    MEMORY.md        — 长期持久化记忆
    daily/           — 每日笔记（自动创建）
  sessions/          — 聊天会话文件（自动创建）
  skills/            — 技能文件（自动创建）
  cron.json          — 定时任务配置（自动创建）
```

## 许可证

MIT — 详见原始 MimiClaw 项目。



【&#x1f44d;赞赏&#x1f44d;】

![赞赏码](assets/like.png)