#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <string>
#include "key.h"

// WiFi credentials
// const char *ssid = "aaabbbccc";             // Replace with your WiFi name
// const char *password = "abcabcabc";   // Replace with your WiFi password

// MQTT Broker settings
// const char *mqtt_broker = "qwerty.io.net";
// const char *mqtt_topic = "asdfgh";
// const char *mqtt_username = "zxcvbnm";
// const char *mqtt_password = "nm,./";
// const int mqtt_port = 8883;

// パン用チャネル
#define PAN 14
// チルト用チャネル
#define TILT 15

// WiFi and MQTT client initialization
WiFiClientSecure esp_client;
PubSubClient mqtt_client(esp_client);

// // Root CA Certificate
// const char *ca_cert = R"EOF(
// -----BEGIN CERTIFICATE-----
// MIIDTzCl2UNvqc/Ep5lXtcaGtkJ0/8pzWaLSxvr3uvlgXfeawhvsnpl/1NvCAjcC
// CzAJBgNVBAYTAkpQMRMwEQYDVQQIDApTb21lLVN0YXRlMSEwHwYDVQQKDBhJbnRl
// cm5ldXuiOBTs3NHktbzWmVB4dEiuoEdKflCBXaWRnaXRzIFB0eSBMdGQxHDAaBgN
// ZXQwIBcNMjQwNDA3MTMzNTU3WhgPMjEyNDAzMTQxMzM1NTdaMGMxCzAJBgNVBAYT
// AkpQMRMwEQYDVQQIDApTb21lLVN0YXRlMSEwHwYDVQQKDBhJbnRlcm5ldCBXaWRn
// aXRzIFB0eSBMdGQxHDAaBgNVBAMME2tha2l0dWJhdGEuaW9iYi5uZXQwggEiMA0G
// CSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDdzuJnbR4F0/btSqxF9Q/mmATD+rVJ
// 1HbGi32IsrGIxmeikt6oE5bqksZPekoYaw3DB2FuZ+oUrs4nRXaOAc+f0/Saz4Db
// sif7qcq8AWd+YqB09mScwUWGK5VCKFlYLXC8hwRxWyXcuPz7H7g8+itHAgMBAAEw
// 9mzcdwaB3IpBE0eqiIXUmZsXBzSmdHEoChNLHMN+jNkUT4Anh61wnqEfUXtaOnbM
// bWojRsrm/kn3FpL5lFEd/N66EkN1v6fVAoWFsTYNYWfNC7RsbZfnXq8gumIdRSnU
// 4s7RttaVBAMME2tha2l0dWJhdGEuaW9iYi5usp0XygUIZwIsZ2rFlB45V45q6ock
// DQYJKoZIhvcNAQELBQADggEBANA01VXOVMHD9YkNnLtl2imPoFZpAex/kiEdi4Ar
// IsSBfrlF3GOFEb4HufO0ECLkW6X3HxkReSCXZhPoMrpAuVmSRmp2xSjCtnjUppFT
// zRx7nhVIh0623vyjOMeAFoSN7SMXkX6UIxIlExAdeWGRLjXV299MX2Q4AFvHsPfu
// 6PeNVmljJIGZFHqTboGzrqtz/ikRlGlDI3rpywYvMA0GCSqGSIb3DQEBCwUAMGMx
// 5r9DYCPD86Ksfi6hqOvQissKt/DyOFJEt2Hv4gyyIaMtD2qfG2w6tQZtSBRxZMx+
// vz0ym8fwLRRzE0Y4ToB7TGHisRhZqil85Q/AgGjDRKuSP2s=
// -----END CERTIFICATE-----
// )EOF";

// Function Declarations
void connectToWiFi();

void connectToMQTT();

void mqttCallback(char *topic, byte *payload, unsigned int length);


void setup() {
    Serial.begin(115200);
    connectToWiFi();

    // サーボ初期化
    Serial.println("servo begin");
    ledcSetup(PAN, 50, 10);  // (チャネル, 周波数, 分解能)
    ledcAttachPin(GPIO_NUM_4, PAN);   // (ピン番号, チャネル)
    ledcSetup(TILT, 50, 10);  // (チャネル, 周波数, 分解能)
    ledcAttachPin(GPIO_NUM_13, TILT);   // (ピン番号, チャネル)
    stop_servo();//角度の初期化

    // Set Root CA certificate
    esp_client.setCACert(ca_cert);

    mqtt_client.setServer(mqtt_broker, mqtt_port);
    mqtt_client.setKeepAlive(60);
    mqtt_client.setCallback(mqttCallback);
    connectToMQTT();
}

void connectToWiFi() {
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi");
}

void connectToMQTT() {
    while (!mqtt_client.connected()) {
        String client_id = "esp32-client-" + String(WiFi.macAddress());
        Serial.printf("Connecting to MQTT Broker as %s...\n", client_id.c_str());
        if (mqtt_client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
            Serial.println("Connected to MQTT broker");
            mqtt_client.subscribe(mqtt_topic);
            mqtt_client.publish(mqtt_topic, "Hi EMQX I'm ESP32 ^^");  // Publish message upon connection
        } else {
            Serial.print("Failed to connect to MQTT broker, rc=");
            Serial.print(mqtt_client.state());
            Serial.println(" Retrying in 5 seconds.");
            delay(5000);
        }
    }
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
    Serial.print("Message received on topic: ");
    Serial.println(topic);
    Serial.print("Message: ");
    for (unsigned int i = 0; i < length; i++) {
        Serial.print((char) payload[i]);
    }
    Serial.println("\n-----------------------");

    int pan;
    int tilt;

    //文字列化
    String payload_str;
    for (unsigned int i = 0; i < length; i++) {
      payload_str+=(char)payload[i];  // Convert *byte to string
    }
    Serial.println("payload_str :" + payload_str);

    //JSON読み込み
    StaticJsonDocument<200> doc;
    static char outputtext[100]=""; 


    DeserializationError error = deserializeJson(doc, payload_str);
    if(error) Serial.println("Deserialization error.");
    else {
      pan=doc["test_data1"];
      tilt=doc["test_data2"];
      Serial.println("pan :" + String(pan));
      Serial.println("tilt:" + String(tilt));
    }

    //サーボを動かす
    pan_servo(pan);
    tilt_servo(tilt);
}

// パン用のサーボを動かす
static int pan_angle = 0;
static void pan_servo(int num) {
  Serial.println("pan_handler");

  pan_angle = num;
  if (pan_angle > 90) {
    pan_angle = 90;
  }
  if (pan_angle < -90) {
    pan_angle = -90;
  }
  ledcWrite(PAN, map(pan_angle, -90, 90, 26, 121));  // -90°〜90°を26〜123に比例計算
}

// チルト用のサーボを動かす
static int tilt_angle = 0;
static void tilt_servo(int num) {
  Serial.println("tilt_handler");

  tilt_angle = num;
  if (tilt_angle > 90) {
    tilt_angle = 90;
  }
  if (tilt_angle < -90) {
    tilt_angle = -90;
  }
  ledcWrite(TILT, map(tilt_angle, -90, 90, 26, 121));  // -90°〜90°を26〜123に比例計算
}

// サーボ停止
static void stop_servo() {
  Serial.println("stop_handler");

  pan_angle = 0;
  ledcWrite(PAN, map(pan_angle, -90, 90, 26, 121));  // -90°〜90°を26〜123に比例計算
  tilt_angle = 0;
  ledcWrite(TILT, map(tilt_angle, -90, 90, 26, 121));  // -90°〜90°を26〜123に比例計算
}

void loop() {
    if (!mqtt_client.connected()) {
        connectToMQTT();
    }
    mqtt_client.loop();
}