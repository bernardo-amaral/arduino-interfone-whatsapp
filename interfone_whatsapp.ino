#include <Arduino.h>
#include <DNSServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiClient.h>
#include <string.h>

#include "WiFiManager.h"  //https://github.com/tzapu/WiFiManager

ESP8266WiFiMulti WiFiMulti;

const char *WIFI_SSID = "Nome da Sua Rede";
const char *WIFI_PASSWORD = "Senha da Sua Rede";
const char *SERVER_URL = "https://api.callmebot.com/whatsapp.php?";
const char *PHONE = "phone=5511999999999";
const char *MESSAGE = "&text=Sua+mensagem+de+teste";
const char *API_KEY = "&apikey=999999999";
char REQUEST_URL[200];  // URL de requisição

const int VP_PIN = 36;  // Pino VP do ESP32

void configModeCallback(WiFiManager *myWiFiManager) {
    Serial.println("Entered config mode");
    Serial.println(WiFi.softAPIP());
    Serial.println(myWiFiManager->getConfigPortalSSID());
}

void setup() {
    Serial.begin(115200);

    strcpy(REQUEST_URL, SERVER_URL);
    strcat(REQUEST_URL, PHONE);
    strcat(REQUEST_URL, MESSAGE);
    strcat(REQUEST_URL, API_KEY);

    WiFiManager wifiManager;
    // wifiManager.resetSettings();

    wifiManager.setAPCallback(configModeCallback);

    if (!wifiManager.autoConnect("esp8266 interfone")) {
        Serial.println("failed to connect and hit timeout");
        ESP.reset();
        delay(1000);
    }

    WiFi.mode(WIFI_STA);
    WiFiMulti.addAP(wifiManager.getWiFiSSID().c_str(),
                    wifiManager.getWiFiPass().c_str());

    Serial.println(F("WIFIManager connected!"));
    Serial.print(F("IP --> "));
    Serial.println(WiFi.localIP());
    Serial.print(F("GW --> "));
    Serial.println(WiFi.gatewayIP());
    Serial.print(F("SM --> "));
    Serial.println(WiFi.subnetMask());
    Serial.print(F("DNS 1 --> "));
    Serial.println(WiFi.dnsIP(0));
    Serial.print(F("DNS 2 --> "));
    Serial.println(WiFi.dnsIP(1));
}

void loop() {
    HTTPClient http;
    int voltage = analogRead(VP_PIN);

    float voltageValue = voltage / 4095.0 * 12;

    if ((WiFiMulti.run() == WL_CONNECTED)) {
        WiFiClient client;
        HTTPClient http;

        if (voltageValue > 6) {
            Serial.print("[HTTP] begin...\n");
            if (http.begin(client, REQUEST_URL)) {
                Serial.print("[HTTP] GET...\n");
                int httpCode = http.GET();
                if (httpCode > 0) {
                    Serial.printf("[HTTP] GET... code: %d\n", httpCode);
                    if (httpCode == HTTP_CODE_OK ||
                        httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
                        String payload = http.getString();
                        Serial.println(payload);
                    }
                } else {
                    Serial.printf("[HTTP] GET... failed, error: %s\n",
                                  http.errorToString(httpCode).c_str());
                }
                http.end();
            } else {
                Serial.println("[HTTP] Unable to connect");
            }
        }
    }
    delay(200);
}