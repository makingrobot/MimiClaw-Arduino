## 持久化数据

MimiClaw需要文件系统来存储一些信息，数据结构如下：

```
/folder/
  config/
    SOUL.md              — AI 人格定义
    USER.md              — 用户信息（自动填充）
  memory/
    MEMORY.md            — 长期持久化记忆
    yyyyMMdd.md          — 每日笔记（自动创建）
  sessions/              
    sess_xxx.jsonl       — 聊天会话文件（按chat_id自动创建）
  skills/                
    weather.md           — 天气查询
    daily-briefing.md    — 每日简报
    skill-creator.md     — 技能创建
    xxxxxx.md            — 自定义技能
  HEARTBEAT.md           — 心跳（定时任务）
  cron.json              — 定时任务配置（自动创建）
  preferences.json       — 配置文件（不使用nvs时）
```
系统支持各种文件系统，如SPIFFS、FatFS、SDFS等，可根据硬件情况选择和配置<br/>
若简单使用可选SPIFFS，配置CONFIG_USE_SPIFFS=1，分区表中要包含SPIFFS分区<br/>
若需要更多存储，可使用SD卡，配置CONFIG_USE_SDFS=1


## LLM请求上下文构成（16kb）
角色：/config/SOUL.md<br/>
身份：/config/USER.md<br/>
记忆：<br/>
-- 长期记忆（4kb）：/memory/MEMORY.md <br/>
-- 短期记忆（2kb）：/memory/yyyyMMdd.md <br/>
技能（2kb）：<br/>
-- /skills/xxx.md<br/>

