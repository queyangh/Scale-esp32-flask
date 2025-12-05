#include <Arduino.h>
#include "HX711.h"
#include <PubSubClient.h>
#include <WiFi.h>

// --- 引脚配置 ---
const int DOUT1 = 4;
const int SCK1 = 16;
const int DOUT2 = 14;
const int SCK2 = 12;

HX711 scale1;
HX711 scale2;

// --- WiFi 和 MQTT 配置 ---
const char* ssid = "HUAWEI-22Y9N5";
const char* password = "05678537";
const char* mqtt_server = "mqtt.5678537.xyz";
const int mqtt_port = 1883;
const char* mqtt_user = "paijiang";
const char* mqtt_pass = "paijiang";

// --- MQTT 主题 ---
const char* TOPIC_WEIGHT = "scale/weight";
const char* TOPIC_TARE = "scale/tare";
const char* TOPIC_TARE_ONLY = "scale/tare_only";

WiFiClient espClient;
PubSubClient client(espClient);

// --- ⏱️ 时间控制变量 ---
unsigned long lastMsgTime = 0;
const long interval = 500; // <--- 修改这里：设置为 500ms 发送一次

// --- 回调函数 ---
void callback(char* topic, byte* payload, unsigned int length) {
    String topicStr = String(topic);
    // 收到命令立即执行，不受发送间隔限制
    if (topicStr == TOPIC_TARE || topicStr == TOPIC_TARE_ONLY) {
        Serial.println(">>> 执行去皮归零");
        scale1.tare();
        scale2.tare();
    }
}

void setup() {
    Serial.begin(115200);
    
    // 连接 WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi Connected");

    // 设置 MQTT
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
    // 设置更大的 KeepAlive 时间 (默认15秒，改为60秒容错率更高)
    client.setKeepAlive(60); 

    // 初始化 HX711
    scale1.begin(DOUT1, SCK1);
    scale2.begin(DOUT2, SCK2);
    scale1.set_scale(478.19425);
    scale2.set_scale(417.5);
    scale1.tare();
    scale2.tare();
}

void reconnect() {
    // 循环直到连接，但加入简易的退避策略
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        if (client.connect("ESP32_Scale_Client", mqtt_user, mqtt_pass)) {
            Serial.println("connected");
            client.subscribe(TOPIC_TARE);
            client.subscribe(TOPIC_TARE_ONLY);
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

void loop() {
    // 1. 保持 MQTT 心跳 (最重要，必须全速运行)
    if (!client.connected()) {
        reconnect();
    }
    client.loop(); 

    // 2. 非阻塞式发送：每 500ms 执行一次
    unsigned long now = millis();
    if (now - lastMsgTime > interval) {
        lastMsgTime = now;

        // 检查传感器状态
        if (scale1.is_ready() && scale2.is_ready()) {
            float left = scale1.get_units(1); 
            float right = scale2.get_units(1);
            int total = (int)(left + right);

            // 发送数据
            String msg = String(total);
            client.publish(TOPIC_WEIGHT, msg.c_str());
        }
    }
}