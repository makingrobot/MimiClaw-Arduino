## 自定义工具

### 【控制硬件输出】如控制LED

1. 在Board继承类中实现硬件驱动封装，若应用框架未支持，需编写驱动代码，否则可直接实例化后使用，如Ws2812，
```cpp
led_ = new Ws2812Led(pin, num_of_pixels); //led_是成员变量
```
2. 编写工具执行程序
```cpp
bool ledToolExecute(const char* input_json, char* output, size_t output_size) {
    Led *led = Board::GetInstance().GetLed();
    // 解析input_json后，做相关操作
    led->TurnOn(); 
    snprintf(output, output_size, "工具执行结果");
    return true;
}
```
3. 注册工具到MimiClaw
```cpp
MimiTool ledTool = {
    .name = "led_tool",
    .description = "控制LED的开关",
    .input_schema_json = "{\"type\":\"object\",\"properties\":{\"param\":{\"type\":\"string\"}},\"required\":[\"param\"]}",
    .execute = ledToolExecute
};

application->registerTool(ledTool);
```
这一步的input_schema_json描述很关键，它决定了传入到工具函数内的参数，这些参数是由LLM根据文本输入提取出来的。

4. 编写工具使用指南（给LLM用）

这是重重要的一点，Agent通过学习这份指南后，要能懂得在何时调用工具、及如何调用。

### 【从硬件获取数据】如温度传感器数据
1. 在Board继承类中实现硬件驱动封装，若应用框架未支持，需编写驱动代码，否则可直接实例化后使用，如普通数字传感器，可使用实例化DigitalSensor或AnalogSensor后实例。
```cpp
std::shared_ptr<AnalogSensor> temp_ptr = std::make_shared<AnalogSensor>("temp_sensor", pin);
AddSensor(temp_ptr);
```
2. 编写工具执行程序
```cpp
bool tempToolExecute(const char* input_json, char* output, size_t output_size) {
    std::shared_ptr<Sensor> temp_ptr = Board::GetInstance().GetSensor("temp_sensor");
    if (temp_ptr->ReadData()) {  // 读取传感器数据 
        SensorValue *value = temp_ptr->value();
        // 其他处理
    } else {
      // 读取数据失败
    }
    snprintf(output, output_size, "工具执行结果");
    return true;
}
```
3. 注册工具到MimiClaw
```cpp
MimiTool tempTool = {
    .name = "temp_tool",
    .description = "描述这个工具的功能",
    .input_schema_json = "{\"type\":\"object\",\"properties\":{\"param\":{\"type\":\"string\"}},\"required\":[\"param\"]}",
    .execute = tempToolExecute
};

application->registerTool(tempTool);
```
4. 编写工具使用指南（给LLM用）
