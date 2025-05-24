#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>
#include <NewPing.h>

// WiFi
const char* ssid = "kuyheetad";
const char* password = "11111111";

// HiveMQ Cloud
const char* mqtt_server = "8df4daee0ae8481a8b0539623beb8aa0.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_user = "hivemq.webclient.1747899908435";
const char* mqtt_pass = "Wh$:0,ygQObw69AlZ.7H";
const char* mqtt_client_id = "ESP32_Client";

// MQTT Topics
const char* servoTopic = "esp32/servo";
const char* buzzerTopic = "esp32/buzzer";
const char* sensorTopic = "esp32/sensors";

// PIN definitions
#define PIR_PIN 14
#define BUZZER_PIN 27
#define SERVO_PIN 12
#define TRIG_PIN 5
#define ECHO_PIN 18
#define SOIL_DIGITAL_PIN 33  // Soil sensor digital out to GPIO 33

// Objects
Servo myServo;
NewPing sonar(TRIG_PIN, ECHO_PIN, 200);
WiFiClientSecure espClient;
PubSubClient client(espClient);

// Variables
unsigned long lastMsg = 0;

void setup_wifi() {
  delay(10);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  if (String(topic) == servoTopic) {
    int angle = message.toInt();
    myServo.write(angle);
  } else if (String(topic) == buzzerTopic) {
    if (message == "on") digitalWrite(BUZZER_PIN, HIGH);
    else digitalWrite(BUZZER_PIN, LOW);
  }
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect(mqtt_client_id, mqtt_user, mqtt_pass)) {
      client.subscribe(servoTopic);
      client.subscribe(buzzerTopic);
    } else {
      delay(2000);
    }
  }
}

void setup() {
  pinMode(PIR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(SOIL_DIGITAL_PIN, INPUT);
  digitalWrite(BUZZER_PIN, LOW);

  myServo.attach(SERVO_PIN);
  myServo.write(0);

  setup_wifi();

  espClient.setInsecure();  // Skip certificate validation
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;

    int motion = digitalRead(PIR_PIN);
    unsigned int distance = sonar.ping_cm();
    int soil_status = digitalRead(SOIL_DIGITAL_PIN);  // 0 = wet, 1 = dry

    String payload = String("{\"motion\":") + motion +
                     ",\"distance\":" + distance +
                     ",\"soil_status\":" + soil_status + "}";

    client.publish(sensorTopic, payload.c_str());
  }
}
