#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

const char* WIFI_SSID = "nombre_wifi";
const char* WIFI_PASSWORD = "contraseña_wifi";

const char* MQTT_SERVER = "broker.hivemq.com";
const uint16_t MQTT_PORT = 1883;
const char* MQTT_TOPIC = "EFDI/networking";

#define SERVO_PIN 27
#define LED_OPEN_PIN 25
#define LED_CLOSED_PIN 26

#define SERVO_OPEN_ANGLE 90
#define SERVO_CLOSED_ANGLE 0
#define MESSAGE_TIMEOUT_MS 2000

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
Servo barrierServo;

bool puertaAbierta = false;
unsigned long ultimoTiempoMensaje = 0;

void writePinIfValid(int pin, bool estado) {
  if (pin >= 0) {
    digitalWrite(pin, estado ? HIGH : LOW);
  }
}

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Conectando WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi conectado. IP: ");
  Serial.println(WiFi.localIP());
}

void aplicarSalidas() {
  bool mensajeActivo = ultimoTiempoMensaje > 0 && (millis() - ultimoTiempoMensaje) <= MESSAGE_TIMEOUT_MS;
  bool puertaAbiertaEfectiva = mensajeActivo && puertaAbierta;

  barrierServo.write(puertaAbiertaEfectiva ? SERVO_OPEN_ANGLE : SERVO_CLOSED_ANGLE);
  writePinIfValid(LED_OPEN_PIN, puertaAbiertaEfectiva);
  writePinIfValid(LED_CLOSED_PIN, !puertaAbiertaEfectiva);
}

bool parsePayload(const char* payload, bool& abrirPuerta) {
  if (strcmp(payload, "ABIERTO") == 0) {
    abrirPuerta = true;
    return true;
  }

  if (strcmp(payload, "CERRADO") == 0) {
    abrirPuerta = false;
    return true;
  }

  return false;
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char message[80];

  if (length >= sizeof(message)) {
    length = sizeof(message) - 1;
  }

  memcpy(message, payload, length);
  message[length] = '\0';

  bool nuevoEstadoPuerta = false;

  if (!parsePayload(message, nuevoEstadoPuerta)) {
    Serial.print("Payload invalido recibido en ");
    Serial.print(topic);
    Serial.print(": ");
    Serial.println(message);
    return;
  }

  puertaAbierta = nuevoEstadoPuerta;
  ultimoTiempoMensaje = millis();

  aplicarSalidas();

  Serial.print("Recibido -> puerta: ");
  Serial.println(puertaAbierta ? "ABIERTO" : "CERRADO");
}

void connectMQTT() {
  while (!mqttClient.connected()) {
    String clientId = "ESP32_ACTUATOR_";
    clientId += String((uint32_t)(ESP.getEfuseMac() & 0xFFFFFFFF), HEX);

    Serial.print("Conectando a MQTT...");
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println(" conectado");
      mqttClient.subscribe(MQTT_TOPIC);
    } else {
      Serial.print(" fallo, rc=");
      Serial.print(mqttClient.state());
      Serial.println(". Reintentando en 2 s");
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_OPEN_PIN, OUTPUT);
  pinMode(LED_CLOSED_PIN, OUTPUT);

  barrierServo.setPeriodHertz(50);
  barrierServo.attach(SERVO_PIN, 500, 2400);
  barrierServo.write(SERVO_CLOSED_ANGLE);

  connectWiFi();

  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);

  aplicarSalidas();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (!mqttClient.connected()) {
    connectMQTT();
  }

  mqttClient.loop();
  aplicarSalidas();
}
