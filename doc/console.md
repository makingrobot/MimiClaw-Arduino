## 串口控制台

### 串口控制台端口设置
在mimiclaw/app_config.h中，通过设置 **CONFIG_USE_UART1** 为1，可将UART1口设置为串口控制台的端口，
避免与日志输出串口（UART0）混用。<br/>
通过 **CONFIG_UART1_RX** 和 **CONFIG_UART1_TX** 可自定义UART1口的引脚。

### 命令

1.配置命令
<ol>
<li>set_wifi：设置Wifi SSID和密码</li>
<li>set_feishu_creds：设置飞书API凭据</li>
<li>set_tg_token：设置Telegram bot </li>
<li>set_llm_apikey：设置大模型ApiKey</li>
<li>set_llm_model：设置模型</li>
<li>set_llm_provider：设置大模型提供商（anthropic | openai | deepseek）</li>
<li>set_search_provider：设置搜索提供商（brave | tavily）</li>
<li>set_brave_key：设置Brave key</li>
<li>set_tavily_key：设置Tavily key</li>
<li>set_proxy：设置网络代理</li>
<li>clear_proxy：清除网络代理</li>
<li>config_show：显示所有配置</li>
<li>config_reset：重置所有配置</li>
</ol>

2.AI代理命令
<ol>
<li>skill_list：技能列表</li>
<li>skill_show：显示技能详情</li>
<li>skill_search：搜索某个技能</li>
<li>memory_read：读取记忆</li>
<li>memory_write：写入记忆</li>
<li>session_list：会话列表</li>
<li>session_clear：清除会话</li>
<li>talk：对话</li>
</ol>

3.执行类命令
<ol>
<li>tool_exec：工具调用</li>
<li>web_search：Web搜索</li>
</ol>

4.系统命令
<ol>
<li>wifi_status：显示WiFi信号</li>
<li>heap_info：显示内存信号</li>
<li>heartbeat_trigger：发送心跳信号</li>
<li>cron_start：启动自动执行程序</li>
<li>restart：重启</li>
</ol>

5.外设控制
<ol>
<li>led：LED控制工具</li>
</ol>


