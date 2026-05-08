#include <WiFi.h>
#include <WebServer.h>

const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

constexpr uint8_t DIR_PIN = 5;
constexpr uint8_t STEP_PIN = 18;
constexpr uint8_t ENABLE_PIN = 19;

constexpr uint8_t LED_B_PIN = 25;
constexpr uint8_t LED_G_PIN = 26;
constexpr uint8_t LED_R_PIN = 27;

constexpr uint32_t LED_PWM_FREQ = 5000;
constexpr uint8_t LED_PWM_RESOLUTION = 8;

constexpr float STEPS_PER_REV = 200.0f;
constexpr float DEFAULT_ROTATIONS = 1.0f;
constexpr float MIN_ROTATIONS = 0.1f;
constexpr float MAX_ROTATIONS = 20.0f;

constexpr uint16_t DEFAULT_SPEED_RPM = 90;
constexpr uint16_t MIN_SPEED_RPM = 15;
constexpr uint16_t MAX_SPEED_RPM = 360;

WebServer server(80);

long motorPositionSteps = 0;
bool motorDirectionCw = true;
uint16_t currentSpeedRpm = DEFAULT_SPEED_RPM;

uint8_t baseRed = 255;
uint8_t baseGreen = 255;
uint8_t baseBlue = 255;
uint8_t currentRed = 255;
uint8_t currentGreen = 255;
uint8_t currentBlue = 255;
uint8_t currentBrightnessPct = 100;

String rgbToHex(uint8_t red, uint8_t green, uint8_t blue) {
  char buffer[8];
  snprintf(buffer, sizeof(buffer), "#%02X%02X%02X", red, green, blue);
  return String(buffer);
}

float currentPositionRotations() {
  return static_cast<float>(motorPositionSteps) / STEPS_PER_REV;
}

uint16_t rpmToStepsPerSec(uint16_t rpm) {
  return static_cast<uint16_t>((static_cast<uint32_t>(rpm) * STEPS_PER_REV) / 60.0f);
}

void setMotorDirection(bool clockwise) {
  motorDirectionCw = clockwise;
  digitalWrite(DIR_PIN, clockwise ? HIGH : LOW);
}

void enableDriver() {
  digitalWrite(ENABLE_PIN, LOW);  // A4988 enable activo en bajo
}

void pulseStep(uint32_t halfPeriodUs) {
  digitalWrite(STEP_PIN, HIGH);
  delayMicroseconds(halfPeriodUs);
  digitalWrite(STEP_PIN, LOW);
  delayMicroseconds(halfPeriodUs);
}

void moveMotorRotations(float rotations, bool clockwise, uint16_t speedRpm) {
  const long steps = lroundf(rotations * STEPS_PER_REV);
  const uint16_t speedStepsPerSec = rpmToStepsPerSec(speedRpm);
  const uint32_t halfPeriodUs = max<uint32_t>(150, 500000UL / speedStepsPerSec);

  setMotorDirection(clockwise);
  enableDriver();

  for (long i = 0; i < steps; ++i) {
    pulseStep(halfPeriodUs);
  }

  motorPositionSteps += clockwise ? steps : -steps;
}

void applyLedFromSelection() {
  currentRed = static_cast<uint8_t>((static_cast<uint16_t>(baseRed) * currentBrightnessPct) / 100);
  currentGreen = static_cast<uint8_t>((static_cast<uint16_t>(baseGreen) * currentBrightnessPct) / 100);
  currentBlue = static_cast<uint8_t>((static_cast<uint16_t>(baseBlue) * currentBrightnessPct) / 100);

  ledcWrite(LED_R_PIN, currentRed);
  ledcWrite(LED_G_PIN, currentGreen);
  ledcWrite(LED_B_PIN, currentBlue);
}

void setLedOff() {
  currentBrightnessPct = 0;
  applyLedFromSelection();
}

bool parseHexColor(const String& value, uint8_t& red, uint8_t& green, uint8_t& blue) {
  String hex = value;
  if (hex.startsWith("#")) {
    hex.remove(0, 1);
  }

  if (hex.length() != 6) {
    return false;
  }

  const long number = strtol(hex.c_str(), nullptr, 16);
  red = static_cast<uint8_t>((number >> 16) & 0xFF);
  green = static_cast<uint8_t>((number >> 8) & 0xFF);
  blue = static_cast<uint8_t>(number & 0xFF);
  return true;
}

int readIntArg(const char* name, int fallback, int minValue, int maxValue) {
  if (!server.hasArg(name)) {
    return fallback;
  }

  const long value = server.arg(name).toInt();
  if (value < minValue) {
    return minValue;
  }
  if (value > maxValue) {
    return maxValue;
  }
  return static_cast<int>(value);
}

float readFloatArg(const char* name, float fallback, float minValue, float maxValue) {
  if (!server.hasArg(name)) {
    return fallback;
  }

  const float value = server.arg(name).toFloat();
  if (value < minValue) {
    return minValue;
  }
  if (value > maxValue) {
    return maxValue;
  }
  return value;
}

void redirectHome() {
  server.sendHeader("Location", "/", true);
  server.send(303, "text/plain", "");
}

String buildPage() {
  String page;
  page.reserve(4200);

  const String pickerHex = rgbToHex(baseRed, baseGreen, baseBlue);
  const String currentHex = rgbToHex(currentRed, currentGreen, currentBlue);

  page += F(
    "<!doctype html><html lang='es'><head><meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<link rel='preconnect' href='https://fonts.googleapis.com'>"
    "<link rel='preconnect' href='https://fonts.gstatic.com' crossorigin>"
    "<link href='https://fonts.googleapis.com/css2?family=Source+Sans+3:wght@400;600;700&family=Space+Grotesk:wght@500;700&display=swap' rel='stylesheet'>"
    "<title>MT07 - Interfaz ESP32</title>"
    "<style>"
    ":root{--bg:#f4f1ea;--surface:rgba(255,252,246,.92);--surface-strong:#fffdf8;--surface-tint:#f7f3ec;--text:#1f2f37;--muted:#5d6d74;--accent:#0f766e;--accent-strong:#0a5c56;--accent-soft:#dbece7;--border:#d7ddd6;--shadow-soft:0 16px 40px rgba(24,38,44,.08);--shadow-card:0 18px 40px rgba(18,31,35,.10);}"
    "*{box-sizing:border-box;}"
    "body{font-family:'Source Sans 3',sans-serif;line-height:1.7;background:radial-gradient(circle at top left,rgba(15,118,110,.09),transparent 28%),radial-gradient(circle at top right,rgba(170,140,72,.08),transparent 26%),linear-gradient(180deg,#f7f3ec 0%,var(--bg) 32%,#f2eee7 100%);color:var(--text);margin:0;padding:24px;}"
    "body:before{content:'';position:fixed;inset:0;pointer-events:none;background-image:linear-gradient(rgba(255,255,255,.12) 1px,transparent 1px),linear-gradient(90deg,rgba(255,255,255,.12) 1px,transparent 1px);background-size:32px 32px;mask-image:linear-gradient(180deg,rgba(0,0,0,.2),transparent 70%);z-index:-1;}"
    ".shell{max-width:980px;margin:0 auto;display:grid;gap:18px;}"
    ".card{background:var(--surface);border:1px solid rgba(205,213,208,.92);border-radius:24px;padding:22px;box-shadow:var(--shadow-soft);backdrop-filter:blur(10px);}"
    ".hero{background:linear-gradient(180deg,rgba(255,252,248,.96),rgba(244,239,230,.92));}"
    ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(280px,1fr));gap:18px;}"
    ".status{display:grid;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));gap:12px;}"
    ".pill{background:rgba(255,252,248,.82);border:1px solid rgba(205,213,208,.88);border-radius:18px;padding:14px;box-shadow:0 10px 24px rgba(24,38,44,.05);}"
    ".preview{width:72px;height:72px;border-radius:20px;border:1px solid rgba(206,214,209,.92);box-shadow:inset 0 0 24px rgba(255,255,255,.22),0 10px 24px rgba(24,38,44,.06);background:var(--surface-strong);}"
    "form{display:grid;gap:12px;}"
    "label{font-size:1rem;color:#24373e;font-weight:600;}"
    "input,select,button{font-family:'Source Sans 3',sans-serif;font-size:1rem;}"
    "input,select{min-height:2.9rem;border-radius:14px;border:1px solid rgba(196,205,201,.92);background:rgba(255,252,248,.96);color:var(--text);padding:10px 12px;box-shadow:none;}"
    "input:focus,select:focus{outline:none;border-color:rgba(15,118,110,.55);box-shadow:0 0 0 .2rem rgba(15,118,110,.12);}"
    "input[type='color']{padding:6px;height:56px;background:var(--surface-strong);}"
    "input[type='range']{padding:0;accent-color:var(--accent);}"
    "button{min-height:2.9rem;border:none;border-radius:14px;background:linear-gradient(135deg,var(--accent),#146b65);color:#effcf9;font-weight:700;cursor:pointer;box-shadow:0 12px 24px rgba(15,118,110,.18);}"
    "button.warn{background:linear-gradient(135deg,#c98b19,#b96d0b);color:#fff8ea;box-shadow:0 12px 24px rgba(185,109,11,.16);}"
    "button:hover{transform:translateY(-1px);}"
    ".button-row{display:grid;grid-template-columns:1fr 1fr;gap:10px;align-items:end;}"
    "h1,h2{font-family:'Space Grotesk',sans-serif;letter-spacing:-.02em;color:#1c2c33;line-height:1.2;margin:0 0 .8rem;}"
    "h1{font-size:clamp(2rem,4vw,3rem);} h2{font-size:clamp(1.2rem,2vw,1.55rem);} p{margin:0;color:#30434a;} strong{color:#203039;} code{background:rgba(219,236,231,.7);color:#144743;padding:.12rem .38rem;border-radius:8px;}"
    "</style></head><body><div class='shell'>"
    "<div class='card hero'><h1>MT07 - Control web sobre ESP32 simulada</h1>"
    "<p>Interfaz simplificada para controlar el motor paso a paso y el LED RGB desde la web servida por la ESP32.</p></div>"
    "<div class='card'><h2>Estado actual</h2><div class='status'>"
    "<div class='pill'><strong>Posicion actual</strong><br>");
  page += String(currentPositionRotations(), 2);
  page += F(" rotaciones</div><div class='pill'><strong>Color actual</strong><br>");
  page += currentHex;
  page += F("<div class='preview' style='margin-top:10px;background:");
  page += currentHex;
  page += F(";'></div></div></div></div>"
    "<div class='grid'>"
    "<div class='card'><h2>Motor</h2>"
    "<form action='/motor' method='get'>"
    "<label id='speed-label' for='speed'>Velocidad (");
  page += String(currentSpeedRpm);
  page += F(" RPM, max ");
  page += String(MAX_SPEED_RPM);
  page += F(" RPM)</label>"
    "<input id='speed' name='speed' type='range' min='15' max='360' step='5' value='");
  page += String(currentSpeedRpm);
  page += F("' oninput='updateSpeedLabel(this.value)'>"
    "<label for='rotations'>Rotaciones</label>"
    "<input id='rotations' name='rotations' type='number' min='0.1' max='20' step='0.1' value='1.0'>"
    "<label for='dir'>Direccion</label>"
    "<select id='dir' name='dir'>"
    "<option value='cw'>Horario</option>"
    "<option value='ccw'>Antihorario</option>"
    "</select>"
    "<button type='submit'>Mover</button>"
    "</form></div>"
    "<div class='card'><h2>LED RGB</h2>"
    "<form action='/rgb' method='get'>"
    "<label for='color'>Color</label><input id='color' name='color' type='color' value='");
  page += pickerHex;
  page += F("'>"
    "<label for='brightness'>Brillo (");
  page += String(currentBrightnessPct);
  page += F("%)</label>"
    "<input id='brightness' name='brightness' type='range' min='0' max='100' value='");
  page += String(currentBrightnessPct);
  page += F("'>"
    "<div class='button-row'><button type='submit'>Aplicar</button></form>"
    "<form action='/off' method='get'><button class='warn' type='submit'>Apagar</button></form></div>"
    "</div></div></div>"
    "<script>"
    "function updateSpeedLabel(value){"
    "document.getElementById('speed-label').textContent='Velocidad ('+value+' RPM, max 360 RPM)';"
    "}"
    "</script></body></html>");

  return page;
}

void handleRoot() {
  server.send(200, "text/html; charset=utf-8", buildPage());
}

void handleMotor() {
  const uint16_t speedRpm = static_cast<uint16_t>(
    readIntArg("speed", DEFAULT_SPEED_RPM, MIN_SPEED_RPM, MAX_SPEED_RPM)
  );
  const float rotations = readFloatArg("rotations", DEFAULT_ROTATIONS, MIN_ROTATIONS, MAX_ROTATIONS);
  const bool clockwise = server.arg("dir") != "ccw";

  moveMotorRotations(rotations, clockwise, speedRpm);
  currentSpeedRpm = speedRpm;

  Serial.print("Motor movido ");
  Serial.print(clockwise ? "CW" : "CCW");
  Serial.print(" | rotaciones: ");
  Serial.print(rotations, 2);
  Serial.print(" | velocidad: ");
  Serial.print(speedRpm);
  Serial.print(" RPM | posicion: ");
  Serial.print(currentPositionRotations(), 2);
  Serial.println(" rotaciones");

  redirectHome();
}

void handleRgb() {
  uint8_t selectedRed = baseRed;
  uint8_t selectedGreen = baseGreen;
  uint8_t selectedBlue = baseBlue;

  if (server.hasArg("color")) {
    parseHexColor(server.arg("color"), selectedRed, selectedGreen, selectedBlue);
  }

  baseRed = selectedRed;
  baseGreen = selectedGreen;
  baseBlue = selectedBlue;
  currentBrightnessPct = static_cast<uint8_t>(readIntArg("brightness", currentBrightnessPct, 0, 100));
  applyLedFromSelection();

  Serial.print("RGB aplicado -> color base ");
  Serial.print(rgbToHex(baseRed, baseGreen, baseBlue));
  Serial.print(" | brillo ");
  Serial.print(currentBrightnessPct);
  Serial.print("% | color actual ");
  Serial.println(rgbToHex(currentRed, currentGreen, currentBlue));

  redirectHome();
}

void handleOff() {
  setLedOff();
  Serial.println("LED apagado");
  redirectHome();
}

void handleStatus() {
  String json = "{";
  json += "\"positionRotations\":" + String(currentPositionRotations(), 3) + ",";
  json += "\"color\":\"" + rgbToHex(currentRed, currentGreen, currentBlue) + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("WiFi conectado. IP local: ");
  Serial.println(WiFi.localIP());
}

void setupRgb() {
  ledcAttach(LED_B_PIN, LED_PWM_FREQ, LED_PWM_RESOLUTION);
  ledcAttach(LED_G_PIN, LED_PWM_FREQ, LED_PWM_RESOLUTION);
  ledcAttach(LED_R_PIN, LED_PWM_FREQ, LED_PWM_RESOLUTION);
  applyLedFromSelection();
}

void setupStepper() {
  pinMode(DIR_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);

  digitalWrite(STEP_PIN, LOW);
  setMotorDirection(motorDirectionCw);
  enableDriver();
}

void setupServer() {
  server.on("/", handleRoot);
  server.on("/motor", handleMotor);
  server.on("/rgb", handleRgb);
  server.on("/off", handleOff);
  server.on("/status", handleStatus);
  server.onNotFound(handleRoot);
  server.begin();
}

void setup() {
  Serial.begin(115200);
  delay(500);

  setupStepper();
  setupRgb();
  connectWiFi();
  setupServer();

  Serial.println("Servidor HTTP listo");
  Serial.println("Abrir / para la interfaz web o /status para estado JSON");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  server.handleClient();
  delay(2);
}
