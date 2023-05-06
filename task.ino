#define BLINKER_WIFI
#include <Blinker.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_BMP085.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Arduino.h>

#define DHTPIN 15 // 温湿度传感器引脚
#define LDRPIN 36 // 光敏电阻引脚
#define SOILPIN1 32 // 土壤湿度传感器1引脚
#define SOILPIN2 33 // 土壤湿度传感器2引脚
#define FLAMEPIN 16 // 火焰传感器引脚
#define SMOKEPIN 34 //烟雾传感器引脚
#define BUZZERPIN 26 // 蜂鸣器引脚
#define LEDPIN 27 // LED引脚
#define LED 12 //1号LED补光灯针脚
#define LED1 14 //2号LED补光灯针脚
#define V_PIN 39 //测量设备电压要用到的针脚
#define RAIN_SENSOR_PIN 35 //雨滴传感器针脚
#define ONE_WIRE_BUS 4 // 定义DS18B20的引脚为4
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
#define jidianqi1 25 //定义一个控制小风扇的继电器1号，板载的继电器用于水泵

LiquidCrystal_I2C lcd(32,16,2);  // 设置I2C地址和屏幕行列数

Adafruit_BMP085 bmp; //设置BMP180传感器的型号

#define DHTTYPE DHT11 // 温湿度传感器型号

#define SMOKE_THRESHOLD 1200 // 设置烟雾传感器阈值

char auth[] = "40a629d880e0"; // Blinker授权码
char ssid[] = "JinTai"; // WIFI名称
char pswd[] = "j12345678"; // WIFI密码

BlinkerNumber HUMI("humi"); // 湿度数据流
BlinkerNumber TEMP("temp"); // 温度数据流
BlinkerNumber LIGHT("light"); // 光照数据流
BlinkerNumber SOIL1("soil1"); // 土壤湿度数据流1
BlinkerNumber SOIL2("soil2"); // 土壤湿度数据流2
BlinkerNumber VOLTAGE("voltage"); //测量设备的电压
BlinkerNumber PRE("pressure"); //大气压强
BlinkerNumber ALTITUDE("altitude"); //海拔高度
BlinkerNumber SOILTEMP("soil_temp"); //土壤温度

BlinkerButton Button1("btn2");
BlinkerButton Button2("btn1");
BlinkerButton Button3("btn3");
BlinkerButton Button4("btn4");

DHT dht(DHTPIN, DHTTYPE); // 温湿度传感器对象

float humi_read = 0, temp_read = 0, light_read = 0, soil_read1 = 0, soil_read2 = 0;
unsigned long previousMillis = 0;
const unsigned long interval = 2000;
int counter = 0;

const int THRESHOLD = 15;  // 设定的光照强度阈值

// 按下按键即会执行该函数
void button1_callback(const String & state) {
BLINKER_LOG("get button state: ", state);
digitalWrite(LED, !digitalRead(LED));
}

// 按下按键即会执行该函数
void button2_callback(const String & state) {
BLINKER_LOG("get button state: ", state);
digitalWrite(LED1, !digitalRead(LED1));
}

// 按下按键即会执行该函数
void button3_callback(const String & state) {
BLINKER_LOG("get button state: ", state);
digitalWrite(jidianqi1, !digitalRead(jidianqi1));
}

void heartbeat()
{
HUMI.print(humi_read); // 发送湿度数据流
TEMP.print(temp_read); // 发送温度数据流
LIGHT.print(light_read); // 发送光照数据流
SOIL1.print(soil_read1); // 发送土壤湿度数据流1
SOIL2.print(soil_read2); // 发送土壤湿度数据流2
}

void dataStorage()
{
Blinker.dataStorage("temp", temp_read); // 存储温度数据
Blinker.dataStorage("humi", humi_read); // 存储湿度数据
Blinker.dataStorage("light", light_read); // 存储光照数据
Blinker.dataStorage("soil1", soil_read1); // 存储土壤湿度数据1
Blinker.dataStorage("soil2", soil_read2); // 存储土壤湿度数据2
}

void setup()
{

Serial.begin(115200);// 初始化串口通信
BLINKER_DEBUG.stream(Serial);// 设置调试信息输出流
BLINKER_DEBUG.debugAll();// 输出所有调试信息

pinMode(FLAMEPIN, INPUT_PULLUP);
pinMode(SMOKEPIN, INPUT_PULLUP);
pinMode(BUZZERPIN, OUTPUT);
pinMode(LEDPIN, OUTPUT);

Blinker.begin(auth, ssid, pswd);// 启动Blinker库的功能
Blinker.attachHeartbeat(heartbeat);// 设置心跳函数
Blinker.attachDataStorage(dataStorage);// 设置数据存储函数
dht.begin();// 初始化DHT传感器

lcd.begin(); // 初始化液晶屏

if (!bmp.begin(0x76)) {
    Serial.println("Could not find a valid BMP085 sensor, check wiring!");
    while (1) {}
  }

sensors.begin();

// 初始化有LED的IO
pinMode(LED, OUTPUT);
digitalWrite(LED, HIGH);
Button1.attach(button1_callback);
// 初始化有LED的IO
pinMode(LED1, OUTPUT);
digitalWrite(LED1, HIGH);
Button2.attach(button2_callback);

pinMode(jidianqi1,OUTPUT);
digitalWrite(jidianqi1, HIGH);
Button3.attach(button3_callback);
}

bool checkFlame() {
  int flameValue = digitalRead(FLAMEPIN);
  if (flameValue == LOW) {  // 火焰检测到，返回真
    return true;
  } else {  // 火焰未检测到，返回假
    return false;
  }
}

void alert(bool flameDetected) {
  if (flameDetected) {  // 火焰检测到
    Blinker.notify("发现火焰！！");
    Blinker.wechat("发现火焰，请注意安全情况！"); 
    digitalWrite(BUZZERPIN, HIGH);  // 打开蜂鸣器
    digitalWrite(LEDPIN, HIGH);     // 打开LED灯
  } else {  // 火焰未检测到
    digitalWrite(BUZZERPIN, LOW);   // 关闭蜂鸣器
    digitalWrite(LEDPIN, LOW);      // 关闭LED灯
  }
}

void smokeAlarm() {
  uint32_t adcValue = analogRead(34); // 读取烟雾传感器的模拟值
  if (adcValue > SMOKE_THRESHOLD) { // 判断是否超过阈值
    Blinker.notify("发现烟雾！！");
    Blinker.wechat("发现烟雾，请注意安全情况！"); 
    digitalWrite(27, HIGH); // 开启红色LED灯
    tone(26, 2000); // 发出蜂鸣器警报
    delay(500); // 等待500毫秒
    noTone(26); // 停止蜂鸣器警报
  } else {
    digitalWrite(27, LOW); // 关闭红色LED灯
  }
}

// 检测雨滴传感器是否有水滴降落
bool check_rain_sensor() {
  // 将A0口配置为模拟输入模式
  pinMode(RAIN_SENSOR_PIN, INPUT);

  // 读取A0口输入电压值
  int sensor_value = digitalRead(RAIN_SENSOR_PIN);

  // 判断输入电压是否超过阈值，若超过则说明有水滴降落，返回true，否则返回false
  if (sensor_value == LOW) {
    return true;
  } else {
    return false;
  }

}

void loop()
{
// 检测烟雾
  smokeAlarm();
//检测是否存在火焰
bool flameDetected = checkFlame();  // 检测火焰状态
  alert(flameDetected);  // 控制蜂鸣器和LED灯报警
  delay(100);  // 等待一段时间后继续进行检测

Blinker.run();// 运行Blinker库

// 读取DHT传感器、LDR传感器和土壤湿度传感器的数据
unsigned long currentMillis = millis();
if (currentMillis - previousMillis >= interval) {
previousMillis = currentMillis;

float h = dht.readHumidity();
float t = dht.readTemperature();
int l = analogRead(LDRPIN);
int s1 = analogRead(SOILPIN1);
int s2 = analogRead(SOILPIN2);
float temperature = bmp.readTemperature();
float pressure = bmp.readPressure();
float altitude = bmp.readAltitude();

  lcd.setCursor(0,0);              // 将光标移动到第一行第一列
  lcd.print("Temp:");              // 打印"Temp:"
  lcd.print(t);                    // 打印温度值
  lcd.print((char)223);            // 打印温度单位 "°"
  lcd.print("C");                  // 打印温度单位 "C"
  lcd.setCursor(0,1);              // 将光标移动到第二行第一列
  lcd.print("Humidity:");          // 打印"Humidity:"
  lcd.print(h);                    // 打印湿度值
  lcd.print("%");                  // 打印湿度单位 "%"
  
  delay(1000);                     // 延迟5秒

// 如果无法读取DHT传感器的数据
if (isnan(h) || isnan(t)) {
BLINKER_LOG("Failed to read from DHT sensor!");
} else {
// 打印湿度和温度数据到串口
BLINKER_LOG("Humidity: ", h, " %");
BLINKER_LOG("Temperature: ", t, " *C");

humi_read = h;
temp_read = t;
}

// 将LDR传感器的数据映射到0-100的范围内
light_read = map(l, 0, 4095, 100, 0);
// 将土壤湿度传感器1的数据映射到0-100的范围内
soil_read1 = map(s1, 4095, 0, 0, 100);
// 将土壤湿度传感器2的数据映射到0-100的范围内
soil_read2 = map(s2, 4095, 0, 0, 100);

// 打印光照和土壤湿度1的数据到串口
BLINKER_LOG("Light: ", light_read, " %");
BLINKER_LOG("Soil Moisture 1: ", soil_read1, " %");
BLINKER_LOG("Soil Moisture 2: ", soil_read2, " %");

// 读取ESP32的电压值
  int sensorValue = analogRead(39);

  // 将读取到的ADC值转换成电压值
  float voltage = sensorValue / 4095.0 * 3.3;

VOLTAGE.print(voltage);   //将电压值传输到Blinker云平台

PRE.print(pressure);
ALTITUDE.print(altitude);

// 将湿度、温度、光照和土壤湿度1的数据发送到点灯APP中
HUMI.print(humi_read);
TEMP.print(temp_read);
LIGHT.print(light_read);
SOIL1.print(soil_read1);
SOIL2.print(soil_read2);

// 检测雨滴传感器是否有水滴降落
  if (check_rain_sensor()) {
    Serial.println("下雨了哦！");
    Blinker.wechat("下雨了哦！"); 
  } else {
  }

sensors.requestTemperatures();
float soil_temp = sensors.getTempCByIndex(0);
SOILTEMP.print(soil_temp);

if (l < THRESHOLD) {  // 如果光照强度低于阈值
    digitalWrite(LED, HIGH);   // 打开第一个 LED 灯
    digitalWrite(LED1, HIGH);  // 打开第二个 LED 灯
  } else {  // 如果光照强度高于等于阈值
    digitalWrite(LED, LOW);   // 关闭第一个 LED 灯
    digitalWrite(LED1, LOW);  // 关闭第二个 LED 灯
  }

Blinker.delay(500);
delay(500);
}
}