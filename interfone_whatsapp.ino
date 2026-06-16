#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <ESP8266HTTPClient.h>

#define SENSOR_PIN A0
#define LED_PIN D5
#define SENSITIVITY 185.0
#define NUM_SAMPLES 500
#define OFFSET_MANUAL 0.035
#define THRESHOLD 4.0 //A
#define COOLDOWN_MS 30000  

float offsetRaw = 0;
unsigned long ultimoEnvio = 0;

const char *PHONE = "00000000000";
const char *MESSAGE = "Olá!+alguém+está+tocando+o+seu+interfone";
const char *API_KEY = "00000000";

WiFiClient client;
HTTPClient http;

void sendWhatsappMessage() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[HTTP] Sem WiFi — mensagem não enviada.");
    return;
  }

  unsigned long agora = millis();
  if (agora - ultimoEnvio < COOLDOWN_MS) {
    Serial.println("[HTTP] Cooldown ativo — aguardando...");
    return;
  }

  String url = String("http://api.callmebot.com/whatsapp.php?phone=")
               + PHONE
               + "&text=" + MESSAGE
               + "&apikey=" + API_KEY;

  Serial.println("[HTTP] Enviando para: " + url);

  if (http.begin(client, url)) {
    int httpCode = http.GET();

    if (httpCode > 0) {
      Serial.printf("[HTTP] Código: %d\n", httpCode);
      if (httpCode == HTTP_CODE_OK) {
        Serial.println(http.getString());
      }
    } else {
      Serial.printf("[HTTP] Erro: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
    ultimoEnvio = millis();
  } else {
    Serial.println("[HTTP] Não foi possível conectar.");
  }
}

void setup() {
  Serial.begin(115200);

  WiFiManager wm;
  // wm.resetSettings(); // descomente para forçar reconfiguração

  bool ok = wm.autoConnect("PhoneWhatsapp", "1234");
  if (!ok) {
    Serial.println("Falha ao conectar. Reiniciando...");
    ESP.restart();
  }

  Serial.println("WiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  long soma = 0;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    soma += analogRead(SENSOR_PIN);
    delay(2);
  }

  offsetRaw = soma / (float)NUM_SAMPLES;
  Serial.print("Offset RAW calibrado: ");
  Serial.println(offsetRaw);
}

void loop() {
  int raw = analogRead(SENSOR_PIN);

  float deltaRaw = raw - offsetRaw;
  float deltaVolts = (deltaRaw / 1023.0) * 3.3;
  float corrente = (deltaVolts / (SENSITIVITY / 1000.0)) + OFFSET_MANUAL;

  Serial.println(corrente, 3);

  if (abs(corrente) > THRESHOLD) {
    digitalWrite(LED_PIN, HIGH);
    sendWhatsappMessage();  
  } else {
    digitalWrite(LED_PIN, LOW);
  }

  delay(100);
}
