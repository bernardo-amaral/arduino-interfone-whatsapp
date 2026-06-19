#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <ESP8266HTTPClient.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <WiFiClient.h>
#include <UrlEncode.h>

#define SENSOR_PIN     A0
#define LED_PIN        D5
#define SENSITIVITY    185.0
#define NUM_SAMPLES    500
#define OFFSET_MANUAL  0.035
#define THRESHOLD      0.3
#define COOLDOWN_MS    30000
#define CONFIG_FILE    "/config.json"

#define FLASH_BUTTON   D3   // GPIO0
#define HOLD_TIME_MS   5000

unsigned long flashPressStart = 0;
bool flashHeld = false;

char wPhone[20]    = "";
char wApiKey[20]   = "";
char wMessage[100] = "Alguem esta tocando o seu interfone";

float offsetRaw = 0;
bool foiDisparado = false;
unsigned long ultimoEnvio = 0;

WiFiClient client;
HTTPClient http;

// ─── Salva parâmetros no LittleFS ────────────────────────────────────────────
void saveConfig() {
  StaticJsonDocument<256> doc;
  doc["phone"]   = wPhone;
  doc["apikey"]  = wApiKey;
  doc["message"] = wMessage;

  File f = LittleFS.open(CONFIG_FILE, "w");
  if (f) {
    serializeJson(doc, f);
    f.close();
    Serial.println("[Config] Salvo.");
  }
}

// ─── Carrega parâmetros do LittleFS ──────────────────────────────────────────
void loadConfig() {
  if (!LittleFS.exists(CONFIG_FILE)) return;

  File f = LittleFS.open(CONFIG_FILE, "r");
  if (!f) return;

  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, f);
  if (!err) {
    strlcpy(wPhone,   doc["phone"]   | "", sizeof(wPhone));
    strlcpy(wApiKey,  doc["apikey"]  | "", sizeof(wApiKey));
    strlcpy(wMessage, doc["message"] | "", sizeof(wMessage));
    Serial.println("[Config] Carregado.");
  } else {
    Serial.println("[Config] Erro ao ler JSON.");
  }

  f.close();
}

// ─── Limpa WiFi ao segurar FLASH ─────────────────────────────────────────────
void clearWifiSettings() {
  Serial.println("\n[WiFi] Limpando credenciais e configuracoes...");

  WiFiManager wm;
  wm.resetSettings();
  WiFi.disconnect(true);
  ESP.eraseConfig();

  Serial.println("[WiFi] Configuracoes apagadas. Reiniciando...");
  delay(1000);
  ESP.restart();
}

void checkFlashButton() {
  if (digitalRead(FLASH_BUTTON) == LOW) {
    if (flashPressStart == 0) {
      flashPressStart = millis();
    }

    if (!flashHeld && (millis() - flashPressStart >= HOLD_TIME_MS)) {
      flashHeld = true;
      digitalWrite(LED_PIN, HIGH);
      clearWifiSettings();
    }
  } else {
    flashPressStart = 0;
    flashHeld = false;
  }
}

// ─── WhatsApp ─────────────────────────────────────────────────────────────────
void sendWhatsappMessage(String message) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WA] Sem WiFi.");
    return;
  }

  if (strlen(wPhone) == 0 || strlen(wApiKey) == 0) {
    Serial.println("[WA] Phone ou API Key nao configurados.");
    return;
  }

  if (millis() - ultimoEnvio < COOLDOWN_MS) {
    Serial.println("[WA] Cooldown ativo.");
    return;
  }

  String encodedMessage = urlEncode(message);
  String url = "http://api.callmebot.com/whatsapp.php?phone=" + String(wPhone) +
               "&apikey=" + String(wApiKey) +
               "&text=" + encodedMessage;

  Serial.println("[WA] Enviando mensagem...");
  Serial.println("[WA] URL: " + url);

  http.begin(client, url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int httpResponseCode = http.POST(url);

  if (httpResponseCode == 200) {
    Serial.println("[WA] Mensagem enviada com sucesso.");ßåç
    Serial.println(http.getString());
    ultimoEnvio = millis();
  } else {
    Serial.print("[WA] Erro no envio. HTTP: ");
    Serial.println(httpResponseCode);
    Serial.println(http.getString());
  }

  http.end();
}

// ─── Setup ───────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  pinMode(FLASH_BUTTON, INPUT_PULLUP);

  Serial.println("Inicializando...");
  Serial.println("Segure FLASH por 5s para limpar o WiFi.");

  if (!LittleFS.begin()) {
    Serial.println("[FS] Falha ao iniciar LittleFS.");
  } else {
    loadConfig();
  }

  WiFiManagerParameter paramPhone(
    "phone",
    "Telefone (ex: 5551999999999)",
    wPhone,
    20
  );

  WiFiManagerParameter paramApiKey(
    "apikey",
    "CallMeBot API Key",
    wApiKey,
    20
  );

  WiFiManagerParameter paramMessage(
    "message",
    "Mensagem do WhatsApp",
    wMessage,
    100
  );

  WiFiManager wm;
  wm.addParameter(&paramPhone);
  wm.addParameter(&paramApiKey);
  wm.addParameter(&paramMessage);

  wm.setSaveParamsCallback([&]() {
    strlcpy(wPhone,   paramPhone.getValue(),   sizeof(wPhone));
    strlcpy(wApiKey,  paramApiKey.getValue(),  sizeof(wApiKey));
    strlcpy(wMessage, paramMessage.getValue(), sizeof(wMessage));
    saveConfig();
  });

  if (!wm.autoConnect("Interfone-Setup", "12345678")) {
    Serial.println("Falha no WiFi. Reiniciando...");
    ESP.restart();
  }

  Serial.println("WiFi conectado! IP: " + WiFi.localIP().toString());
  Serial.println("Phone: " + String(wPhone));
  Serial.println("API Key: " + String(wApiKey));
  Serial.println("Mensagem: " + String(wMessage));

  long soma = 0;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    soma += analogRead(SENSOR_PIN);
    delay(2);
  }

  offsetRaw = soma / (float)NUM_SAMPLES;
  Serial.print("Offset RAW: ");
  Serial.println(offsetRaw);
}

// ─── Loop ────────────────────────────────────────────────────────────────────
void loop() {
  checkFlashButton();

  int raw = analogRead(SENSOR_PIN);
  float deltaRaw = raw - offsetRaw;
  float deltaVolts = (deltaRaw / 1023.0) * 3.3;
  float corrente = (deltaVolts / (SENSITIVITY / 1000.0)) + OFFSET_MANUAL;

  Serial.print("Corrente: ");
  Serial.print(corrente, 3);
  Serial.println(" A");

  bool acimaDoLimiar = abs(corrente) > THRESHOLD;

  if (acimaDoLimiar && !foiDisparado) {
    digitalWrite(LED_PIN, HIGH);
    sendWhatsappMessage(String(wMessage));
    foiDisparado = true;
  }

  if (!acimaDoLimiar) {
    digitalWrite(LED_PIN, LOW);
    foiDisparado = false;
  }

  delay(100);
}
