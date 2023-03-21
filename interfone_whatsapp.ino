#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>

#include "WiFiManager.h"  //https://github.com/tzapu/WiFiManager

void configModeCallback(WiFiManager *myWiFiManager) {
    Serial.println("Entered config mode");
    Serial.println(WiFi.softAPIP());
    Serial.println(myWiFiManager->getConfigPortalSSID());
}

void setup() {
    Serial.begin(115200);

    WiFiManager wifiManager;
    // wifiManager.resetSettings();

    wifiManager.setAPCallback(configModeCallback);

    if (!wifiManager.autoConnect("esp8266 interfone")) {
        Serial.println("failed to connect and hit timeout");
        ESP.reset();
        delay(1000);
    }

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
    // put your main code here, to run repeatedly:
}