#include <M5Stack.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include <ArduinoJson.h>

// MQTT の接続先のIP
const char *endpoint = "10.0.1.4";  // 環境に合わせて編集すること

// MQTT のポート
const int port = 1883;
// デバイス ID. 機器ごとにユニークにします
// const char *deviceID = ("M5Stack-" + String(random(0xffff), HEX)).c_str();
const char *deviceID = "M5Stack";

// NODE=RED メッセージを知らせるトピック
const char *pubTopic = "/pub/M5Stack";
// NODE=RED メッセージを待つトピック
const char *subTopic = "/sub/M5Stack";

////////////////////////////////////////////////////////////////////////////////

WiFiClient  netClient;
PubSubClient mqttClient;
uint16_t bg_color = TFT_BLACK;

void setup() {
  Serial.begin(115200);

  // Initialize the M5Stack object
  M5.begin();

  // START
  M5.Lcd.fillScreen(bg_color);
  M5.Lcd.setCursor(10, 10);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(3);
  M5.Lcd.printf("START");

  setup_network();

  connectMQTT();
}

void setup_network() {
  // Wi-Fi config store
  Preferences preferences;
  preferences.begin("wifi-config");
  String wifi_ssid = preferences.getString("WIFI_SSID");
  String wifi_password = preferences.getString("WIFI_PASSWD");

  Serial.println("Connecting to ");
  Serial.print(wifi_ssid);
  WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // WiFi Connected
  Serial.println("\nWiFi Connected.");
  M5.Lcd.setCursor(10, 40);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(3);
  M5.Lcd.printf("WiFi Connected.");
}

void connectMQTT() {
  mqttClient.setServer(endpoint, port);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setClient(netClient);

  while (!mqttClient.connected()) {
    Serial.println("Connected. MQTT");
    if (mqttClient.connect(deviceID)) {
      Serial.println("Connected. device");
      int qos = 0;
      mqttClient.subscribe(subTopic, qos);
      Serial.println("Subscribed.");
    } else {
      Serial.print("Failed. Error state=");
      Serial.print(mqttClient.state());
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

long messageSentAt = 0;
int count = 0;
char pubMessage[128];
int led, red, green, blue;

void mqttCallback (char* topic, byte* payload, unsigned int length) {

  String str = "";
  Serial.print("Received. topic=");
  Serial.println(topic);
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    str += (char)payload[i];
  }
  Serial.print("\n");

  StaticJsonDocument<200> jsonDoc;
  auto error = deserializeJson(jsonDoc, str);

  // パースが成功したか確認する。失敗なら終了
  if (error) {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(error.c_str());
    return;
  }

  // JSONデータを割りあて
  const char* message = jsonDoc["message"];
  led = jsonDoc["led"];
  red = jsonDoc["r"];
  green = jsonDoc["g"];
  blue = jsonDoc["b"];

  Serial.print("red = ");
  Serial.print(red);
  Serial.print(" green = ");
  Serial.println(green);
  Serial.print(" blue = ");
  Serial.println(blue);

  if ( led == 1 ) {
    // RGBカラー uint16_t に変換
    // uint16_t RGB = red << 11 | green << 5 | blue;
    bg_color = ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3);
    // 背景カラー反映
    M5.Lcd.fillRect(0, 0, 320, 240, bg_color);
    // テキスト反映
    M5.Lcd.setCursor(10, 120);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(5);
    M5.Lcd.printf(message);
  } else {
    // 消灯
    bg_color = TFT_BLACK;
    M5.Lcd.fillScreen(bg_color);
  }

  delay(300);
}

void mqttLoop() {
  if (!mqttClient.connected()) {
    connectMQTT();
  }
  mqttClient.loop();
}

void show_and_send_message(const char * message) {
  Serial.println(message);
  M5.Lcd.fillRect(0, 90, 320, 100, bg_color);
  M5.Lcd.setTextSize(5);
  M5.Lcd.print(message);

  sprintf(pubMessage, "{\"count\": %d, \"button\": \"%s\"}", count++, message);
  mqttClient.publish(pubTopic, pubMessage);
  Serial.println("Published.");
  Serial.println(pubMessage);
}

void loop() {
  // 常にチェックして切断されたら復帰できるように
  mqttLoop();

  // update button state
  M5.update();
  M5.Lcd.setTextColor(WHITE);

  String button_status = "";
  M5.Lcd.setCursor(150, 150);
  // if you want to use Releasefor("was released for"), use .wasReleasefor(int time) below
  if (M5.BtnA.wasReleased()) {
    button_status = "A";
    show_and_send_message(button_status.c_str());
  } else if (M5.BtnB.wasReleased()) {
    button_status = "B";
    show_and_send_message(button_status.c_str());
  } else if (M5.BtnC.wasReleased()) {
    button_status = "C";
    show_and_send_message(button_status.c_str());
  } else if (M5.BtnB.wasReleasefor(700)) {
    button_status = "";
  }

  // 5秒ごとにメッセージを飛ばす
  long now = millis();
  if (now - messageSentAt > 5000) {
    messageSentAt = now;
    sprintf(pubMessage, "{\"count\": %d}", count++);
    Serial.print("Publishing message to topic ");
    Serial.println(pubTopic);
    Serial.println(pubMessage);
    mqttClient.publish(pubTopic, pubMessage);
    Serial.println("Published.");
  }
}
