from flask import Flask, render_template, jsonify, request
import paho.mqtt.client as mqtt

app = Flask(__name__)

# --- 全局变量 ---
current_weight = 0
max_weight = 0

# --- MQTT 配置 ---
MQTT_BROKER = "mqtt.5678537.xyz"
MQTT_PORT = 1883
MQTT_USER = "paijiang"
MQTT_PASS = "paijiang"

# --- 主题定义 ---
TOPIC_WEIGHT = "scale/weight"
TOPIC_TARE = "scale/tare"
TOPIC_TARE_ONLY = "scale/tare_only"


# --- MQTT 回调 ---
def on_connect(client, userdata, flags, rc):
    print(f"MQTT Connected with result code {rc}")
    client.subscribe(TOPIC_WEIGHT)


def on_message(client, userdata, msg):
    global current_weight, max_weight
    try:
        payload = msg.payload.decode()
        # 转换为整数，处理可能的小数点字符串
        val = float(payload)
        current_weight = int(val)

        # 简单的滤波：只有大于某个阈值（例如2g）才开始记录最大值，避免零点抖动
        if current_weight > max_weight:
            max_weight = current_weight

    except ValueError:
        print("Received invalid weight data")


# 初始化 MQTT 客户端
mqtt_client = mqtt.Client()
mqtt_client.username_pw_set(MQTT_USER, MQTT_PASS)
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message

# 连接 MQTT
try:
    mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
    mqtt_client.loop_start()  # 开启后台线程处理网络循环
except Exception as e:
    print(f"MQTT Connection Failed: {e}")


# --- Flask 路由 ---

@app.route('/')
def index():
    # 首次加载页面
    return render_template('index.html', current_weight=current_weight, max_weight=max_weight)


@app.route('/get_weight', methods=['GET'])
def get_weight():
    # 前端定时轮询接口
    return jsonify(current_weight=current_weight, max_weight=max_weight)


@app.route('/tare', methods=['POST'])
def tare():
    """
    下一组：清空最大重量，并命令ESP32去皮
    """
    global max_weight, current_weight

    # 1. 业务逻辑：重置最大重量
    max_weight = 0
    # current_weight 会随下一次MQTT消息更新，但为了UI即使响应，暂时置0
    current_weight = 0

    # 2. 发送命令给 ESP32
    mqtt_client.publish(TOPIC_TARE, "cmd_tare")

    # 3. 返回 JSON (必须!)
    return jsonify(success=True, current_weight=0, max_weight=0)


@app.route('/tare_only', methods=['POST'])
def tare_only():
    """
    仅去皮：保留最大重量，仅命令ESP32当前归零
    """
    global current_weight
    # 不需要重置 max_weight
    current_weight = 0

    # 发送命令给 ESP32
    mqtt_client.publish(TOPIC_TARE_ONLY, "cmd_tare_only")

    return jsonify(success=True, current_weight=0, max_weight=max_weight)


if __name__ == "__main__":
    app.run(debug=True, host='0.0.0.0', port=3323)