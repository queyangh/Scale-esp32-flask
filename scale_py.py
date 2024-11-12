from flask import Flask, render_template, jsonify
import paho.mqtt.client as mqtt

app = Flask(__name__)

# 初始化变量
current_weight = 0
max_weight = 0

# MQTT配置
mqtt_broker = "8.138.6.2"
mqtt_port = 1883
mqtt_user = "paijiang"
mqtt_password = "paijiang"
mqtt_topic = "weight"


# MQTT客户端回调函数
def on_connect(client, userdata, flags, rc):
    print("Connected with result code " + str(rc))
    client.subscribe(mqtt_topic)


def on_message(client, userdata, msg):
    global current_weight, max_weight
    # 获取消息内容
    current_weight = int(float(msg.payload.decode()))

    # 更新最大重量
    if current_weight > max_weight:
        max_weight = current_weight


# 初始化MQTT客户端
mqtt_client = mqtt.Client()
mqtt_client.username_pw_set(mqtt_user, mqtt_password)
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message

mqtt_client.connect(mqtt_broker, mqtt_port, 60)


@app.route('/')
def index():
    return render_template('index.html', current_weight=current_weight, max_weight=max_weight)


@app.route('/get_weight', methods=['GET'])
def get_weight():
    return jsonify(current_weight=current_weight, max_weight=max_weight)


@app.route('/tare', methods=['POST'])
def tare():
    global max_weight
    max_weight = 0  # 清空最大重量

    # 发送去皮命令到 ESP32
    mqtt_client.publish("tare", "tare")  #

    return render_template('index.html', current_weight=current_weight, max_weight=max_weight)

@app.route('/tare_only', methods=['POST'])
def tare_only():
    # 发送去皮命令给ESP32
    mqtt_client.publish("tare_only_topic", "tare")
    return jsonify({"message": "Tare only executed"})


if __name__ == "__main__":
    # 启动MQTT客户端线程
    mqtt_client.loop_start()
    app.run(debug=True, host='0.0.0.0', port=3323)
