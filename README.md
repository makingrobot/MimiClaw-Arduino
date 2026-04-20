# MimiClaw Arduino

在 ESP32-S3（带 PSRAM）上运行 MimiClaw AI Agent 的 Arduino 程序。<br/>
基于ESP32-Arduino-Framework开发框架。

## 功能特性

- **ReAct 智能体循环** — 自主 AI 智能体，支持工具调用（最多 10 次迭代）
- **双 LLM 支持** — 同时支持 Anthropic Claude 和 OpenAI/Deepseek GPT 模型
- **Telegram 机器人** — 长轮询方式集成 Telegram Bot
- **Feishu 机器人** — 长轮询方式集成 飞书 Bot
- **串口控制台** — 集成串口交互控制台界面
- **WebSocket 服务器** — 实时通信网关
- **持久化记忆** — 基于 SPIFFS 的长期记忆与每日笔记
- **会话管理** — 按聊天分组的对话历史（JSONL 格式）
- **九个内置工具** — 网页搜索、文件读写、时间获取、定时任务调度
- **技能系统** — 从 SPIFFS 加载的可扩展技能文件
- **定时任务调度** — 支持循环和一次性定时任务
- **心跳检测** — 定期检查 HEARTBEAT.md 文件
- **HTTP/SOCKS5 代理** — 所有 API 调用可选代理支持
- **网页配置** — 通过HTML网页配置运行参数


## 硬件要求

- 带有 **PSRAM** 的 ESP32-S3 开发板（推荐：8MB 以上 PSRAM，16MB Flash）
- WiFi 网络连接
- SD卡模块（可选）


## 文档

- [串口控制台](doc/console.md)
- [持久化文件](doc/persist_file.md)
- [开发](doc/develop.md)
- [自定义工具](doc/tools.md)


## 计划（不分先后）

- 飞书机器人支持（已实现）
- SD卡及其他文件系统支持（已实现）
- 短期记忆压缩到长期记忆
- 语音输入
- 语音输出
- 拍照输入（摄像头支持）
- TFT-LCD显示
- OTA 升级


## 许可证

MIT — 详见原始 MimiClaw 项目。


## 赞赏

![赞赏码](assets/like.png)