#include <Arduino.h>
#include "HX711.h"
#include <PubSubClient.h>  // MQTT 库
#include <WiFi.h>



// 配置 HX711 引脚
const int DOUT1 = 4;  // HX711的DT引脚
const int SCK1 = 16;  // HX711的SCK引脚
const int DOUT2 = 14; // HX711的DT引脚
const int SCK2 = 12;  // HX711的SCK引脚

HX711 scale1;
HX711 scale2;

// MQTT 配置
WiFiClient espClient;
PubSubClient client(espClient);
const char* ssid = "CX";
const char* password = "8888899999";
const char* mqtt_server = "8.138.6.2";
const int mqtt_port = 1883;
const char* mqtt_user = "paijiang";
const char* mqtt_pass = "paijiang";

// MQTT 连接回调函数
void callback(char* topic, byte* payload, unsigned int length) {
    String msg = "";
    for (int i = 0; i < length; i++) {
        msg += (char)payload[i];
    }
    
    if (msg == "tare") {
        // 执行去皮操作
        scale1.tare();
        scale2.tare();
        Serial.println("去皮操作已执行");
    }
}

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("连接中...");
    }
    Serial.println("连接成功");

    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);

    // 初始化 HX711
    scale1.begin(DOUT1, SCK1);
    scale2.begin(DOUT2, SCK2);

    // 校准传感器
    scale1.set_scale(478.19425);
    scale2.set_scale(417.5);
    scale1.tare();
    scale2.tare();
}

void reconnect() {
    // 如果断开连接，则重新连接 MQTT
    while (!client.connected()) {
        if (client.connect("ESP32_Client", mqtt_user, mqtt_pass)) {
            client.subscribe("tare");  // 订阅去皮命令
        } else {
            delay(5000);
        }
    }
}

void loop() {
  if (!client.connected()) {
      reconnect();
  }
  client.loop();  // 监听 MQTT 消息

  // 读取重量
  float left_scale = scale1.get_units();
  float right_scale = scale2.get_units();
  int result_scale = left_scale + right_scale;

  // 将数据发送到Flask后端
  String message = String(result_scale);
  client.publish("weight", message.c_str());

  delay(50);
}