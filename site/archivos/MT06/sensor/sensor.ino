#include <WiFi.h>
#include <PubSubClient.h>
const char* WIFI_SSID = "nombre_wifi";
const char* WIFI_PASSWORD = "contraseña_wifi";

const char* MQTT_SERVER = "broker.hivemq.com";
const uint16_t MQTT_PORT = 1883;
const char* MQTT_TOPIC = "EFDI/networking";

#define TRIG_PIN 26
#define ECHO_PIN 27
#define BOTON_PIN 25

#define REBOTE_BOTON_MS 60
#define PUBLISH_INTERVAL_MS 500
#define ECHO_TIMEOUT_US 30000
#define SENSOR_MIN_VALID_CM 2.0f
#define DISTANCE_WINDOW_SIZE 5
#define OPEN_DISTANCE_THRESHOLD_CM 15.0f

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

bool estadoBotonEstable = HIGH;
bool ultimaLecturaBoton = HIGH;
unsigned long ultimoTiempoRebote = 0;
unsigned long ultimoTiempoPublicacion = 0;
bool lastPublishedPuertaAbierta = false;
bool puertaAbiertaPorBoton = false;
float muestrasDistancia[DISTANCE_WINDOW_SIZE] = {0.0f};
uint8_t cantidadMuestrasDistancia = 0;
uint8_t indiceMuestraDistancia = 0;

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

void connectMQTT() {
  while (!mqttClient.connected()) {
    String clientId = "ESP32_SENSOR_";
    clientId += String((uint32_t)(ESP.getEfuseMac() & 0xFFFFFFFF), HEX);

    Serial.print("Conectando a MQTT...");
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println(" conectado");
    } else {
      Serial.print(" fallo, rc=");
      Serial.print(mqttClient.state());
      Serial.println(". Reintentando en 2 s");
      delay(2000);
    }
  }
}

float readDistanceCmRaw() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duracion = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);
  if (duracion == 0) {
    return -1.0f;
  }

  float distanciaCm = duracion * 0.0343f * 0.5f;
  if (distanciaCm < SENSOR_MIN_VALID_CM) {
    return -1.0f;
  }

  return distanciaCm;
}

void ordenarMuestras(float* valores, uint8_t cantidad) {
  for (uint8_t i = 0; i < cantidad; i++) {
    for (uint8_t j = i + 1; j < cantidad; j++) {
      if (valores[j] < valores[i]) {
        float temporal = valores[i];
        valores[i] = valores[j];
        valores[j] = temporal;
      }
    }
  }
}

float updateFilteredDistanceCm() {
  float distanciaCrudaCm = readDistanceCmRaw();
  if (distanciaCrudaCm > 0.0f) {
    muestrasDistancia[indiceMuestraDistancia] = distanciaCrudaCm;
    indiceMuestraDistancia = (indiceMuestraDistancia + 1) % DISTANCE_WINDOW_SIZE;

    if (cantidadMuestrasDistancia < DISTANCE_WINDOW_SIZE) {
      cantidadMuestrasDistancia++;
    }
  }

  if (cantidadMuestrasDistancia == 0) {
    return -1.0f;
  }

  float muestrasOrdenadas[DISTANCE_WINDOW_SIZE];
  for (uint8_t i = 0; i < cantidadMuestrasDistancia; i++) {
    muestrasOrdenadas[i] = muestrasDistancia[i];
  }

  ordenarMuestras(muestrasOrdenadas, cantidadMuestrasDistancia);

  if (cantidadMuestrasDistancia % 2 == 1) {
    return muestrasOrdenadas[cantidadMuestrasDistancia / 2];
  }

  uint8_t indiceSuperior = cantidadMuestrasDistancia / 2;
  uint8_t indiceInferior = indiceSuperior - 1;
  return (muestrasOrdenadas[indiceInferior] + muestrasOrdenadas[indiceSuperior]) * 0.5f;
}

void manejarBoton() {
  bool lectura = digitalRead(BOTON_PIN);

  if (lectura != ultimaLecturaBoton) {
    ultimoTiempoRebote = millis();
    ultimaLecturaBoton = lectura;
  }

  if ((millis() - ultimoTiempoRebote) > REBOTE_BOTON_MS && lectura != estadoBotonEstable) {
    estadoBotonEstable = lectura;

    if (estadoBotonEstable == LOW) {
      puertaAbiertaPorBoton = !puertaAbiertaPorBoton;
      Serial.print("Boton presionado. Nuevo estado de puerta por boton: ");
      Serial.println(puertaAbiertaPorBoton ? "ABIERTO" : "CERRADO");
    }
  }
}

bool debePublicar(bool puertaAbierta) {
  if (millis() - ultimoTiempoPublicacion >= PUBLISH_INTERVAL_MS) {
    return true;
  }

  if (puertaAbierta != lastPublishedPuertaAbierta) {
    return true;
  }

  return false;
}

void publicarEstado() {
  float distanciaCm = updateFilteredDistanceCm();
  bool puertaAbiertaPorDistancia = distanciaCm > 0.0f &&
                                   distanciaCm <= OPEN_DISTANCE_THRESHOLD_CM;
  bool puertaAbierta = puertaAbiertaPorDistancia || puertaAbiertaPorBoton;

  if (!debePublicar(puertaAbierta)) {
    return;
  }

  const char* payload = puertaAbierta ? "ABIERTO" : "CERRADO";

  bool ok = mqttClient.publish(MQTT_TOPIC, payload, true);
  if (ok) {
    ultimoTiempoPublicacion = millis();
    lastPublishedPuertaAbierta = puertaAbierta;

    Serial.print("Publicado: ");
    Serial.println(payload);
    Serial.print("Distancia filtrada: ");
    Serial.print(distanciaCm);
    Serial.print(" cm | Distancia: ");
    Serial.print(puertaAbiertaPorDistancia ? "ABIERTO" : "CERRADO");
    Serial.print(" | Boton: ");
    Serial.println(puertaAbiertaPorBoton ? "ABIERTO" : "CERRADO");
  } else {
    Serial.println("No se pudo publicar el mensaje MQTT");
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BOTON_PIN, INPUT_PULLUP);

  connectWiFi();
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (!mqttClient.connected()) {
    connectMQTT();
  }

  mqttClient.loop();
  manejarBoton();
  publicarEstado();
}
