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
```
系统支持各种文件系统，如SPIFFS、FatFS、SDFS等，可根据硬件情况选择和配置<br/>
若简单使用可选SPIFFS，用上传工具将 `persist-data` 文件夹上传至设备<br/>
若需要更多存储，可使用SD卡，配置CONFIG_USE_SDFS=1，将 `persist-data` 文件夹上传SD卡内


## LLM请求上下文构成（16kb）
角色：/config/SOUL.md<br/>
身份：/config/USER.md<br/>
记忆：<br/>
-- 长期记忆（4kb）：/memory/MEMORY.md <br/>
-- 短期记忆（2kb）：/memory/yyyyMMdd.md <br/>
技能（2kb）：<br/>
-- /skills/xxx.md<br/>


## SPIFFS镜像生成和烧录步骤

* 1.生成镜像

使用spiffsgen.py

```shell
# 命令                大小      数据目录    镜像文件
python  spiffsgen.py  0x200000    persist_data     persist_data.bin
```

使用mkspiffs.exe
```shell  
# 命令         数据目录   块大小    页大小    镜像总大小    镜像文件
mkspiffs.exe  -c persist_data   -b 4096   -p 256   -s 0x200000   persist_data.bin
```
<i>mkspiffs.exe 在 ${HOME}\AppData\Local\Arduino15\packages\esp32\tools\mkspiffs 文件夹内</i>

* 2.烧录镜像

```shell
# 命令                串口名称                 起始地址    镜像文件
esptool.exe  --port /dev/ttyUSB0  write_flash  0xc90000   persist_data.bin
```
<i>esptool.exe 在 ${HOME}\AppData\Local\Arduino15\packages\esp32\tools\esptool-py 文件夹内</i>


