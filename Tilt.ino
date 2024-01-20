#include <ESP32Encoder.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define CLK 22
#define DT 23
#define SERVO_PIN 18
#define Volt 34

// WiFi credentials
const char* ssid = "Swann Nest";
const char* password = "Magn0lia";

// MQTT Broker settings
const char* mqtt_broker = "homeassistant.local";
const char* mqtt_username = "mqtt-user";
const char* mqtt_password = "RockSell35";
const char* mqtt_topic = "home/esp32/servo_position";
const char* mqtt_receive_topic = "home/esp32/set_servo";

WiFiClient espClient;
PubSubClient client(espClient);

ESP32Encoder encoder;
Servo myservo;
int oldPosition = 0;

void setupWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP32Client", mqtt_username, mqtt_password )) {
      Serial.println("Connected to MQTT Broker!");
      client.subscribe(mqtt_receive_topic);
    } else {
      Serial.print("Failed to connect to MQTT Broker. rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

float readVoltage() {
  int sensorValue = analogRead(Volt);
  float arduinoVoltage = sensorValue * (5.00 / 1023.00) * 2; // Initial conversion
  float calibrationFactor = 7.96 / 40.03; // Replace with your measured values
  return arduinoVoltage * calibrationFactor; // Apply calibration factor
}


void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0'; // Ensure payload is null-terminated
  String message = String((char *)payload);

  if (String(topic) == mqtt_receive_topic) {
    if (message == "volt") {
      float voltage = readVoltage();
      char voltageMsg[50];
      sprintf(voltageMsg, "voltage: %.2fV", voltage);
      client.publish(mqtt_topic, voltageMsg);
    } else {
      int newPosition = message.toInt();
      if (newPosition >= 0 && newPosition <= 100) {
        int servoPosition = map(newPosition, 0, 100, 0, 180);
        myservo.write(servoPosition);
        encoder.setCount(newPosition);
        oldPosition = newPosition;
      }
    }
  }
}

void setup() {
  encoder.attachHalfQuad(DT, CLK);
  encoder.setCount(0);
  Serial.begin(115200);
  myservo.attach(SERVO_PIN);
  setupWiFi();
  client.setServer(mqtt_broker, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  int newPosition = encoder.getCount();
  if (newPosition > 100) {
    newPosition = 100;
    encoder.setCount(100);
  } else if (newPosition < 0) {
    newPosition = 0;
    encoder.setCount(0);
  }

  if (newPosition != oldPosition) {
    int servoPosition = map(newPosition, 0, 100, 0, 180);
    myservo.write(servoPosition);
    char msg[50];
    sprintf(msg, "%d", newPosition);
    client.publish(mqtt_topic, msg);
    oldPosition = newPosition;
  }
}
