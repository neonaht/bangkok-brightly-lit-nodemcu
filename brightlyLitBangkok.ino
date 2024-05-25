#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <SoftwareSerial.h>

#define DEBUG 0
#define SSID "your-ssid"
#define PASSWORD "your-password"

const char* ssid = SSID;
const char* password = PASSWORD;

// Set API
const char* serverName = "https://iuvf2ngq8i.execute-api.ap-south-1.amazonaws.com/prod/brightly";

// Set checkpoint time
unsigned long lastTime = 0;

// Set timer to 2 seconds (2000)
unsigned long timerDelay = 2000;

EspSoftwareSerial::UART StmSerial;

int ptr, prev = -1;
int PIR_value, LDR_value;
bool PIR_state, LDR_state;

void setup() {
  StmSerial.begin(115200, EspSoftwareSerial::SWSERIAL_8N1, D7, D8, false, 95, 11);
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Timer set to 5 seconds (timerDelay variable), it will take 5 seconds before publishing the first reading.");
}

void loop() {

  while (StmSerial.available() > 0) {
    ptr = StmSerial.read();

    int dataType = (ptr >> 7) & 1;

    if(dataType) {
      prev = (ptr >> 6) & 1;
      int data = (ptr & 7) << 7;
      if(prev) {
        LDR_value = data;
        LDR_state = false;
      }
      else {
        PIR_value = data;
        PIR_state = false;
      }
    }
    else if(prev != -1) {
      int data = ptr;
      if(prev) {
        LDR_value |= data;
        LDR_state = true;
      }
      else {
        PIR_value |= data;
        PIR_state = true;
      }
      prev = -1;
    }

    if(DEBUG && LDR_state && PIR_state)
      Serial.printf("-- Receive --\nPIR : %d\nLDR : %d\n", PIR_value, LDR_value);
  }
  // Send an HTTP POST request every 10 minutes
  if ((millis() - lastTime) > timerDelay && PIR_state && LDR_state) {
    //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      BearSSL::WiFiClientSecure client;
      HTTPClient https;

      client.setInsecure();

      // Your Domain name with URL path or IP address with path
      https.begin(client, serverName);

      // If you need Node-RED/server authentication, insert user and password below
      //http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");

      // Specify content-type header
      // http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      https.addHeader("Content-Type", "application/json");
      // Data to send with HTTP POST
      char str[100];
      sprintf(str, "{\"pir\":%d,\"light\":%d,\"place\":\"siam\"}", PIR_value, LDR_value);
      String httpsRequestData = str;
      // Send HTTP POST request
      int httpsResponseCode = https.POST(httpsRequestData);
      // If you need an HTTP request with a content type: application/json, use the following:
      // http.addHeader("Content-Type", "application/json");
      // int httpResponseCode = http.POST("{\"api_key\":\"tPmAT5Ab3j7F9\",\"PIR_sensor\":\"BME280\",\"value1\":\"24.25\",\"value2\":\"49.54\",\"value3\":\"1005.14\"}");

      // If you need an HTTP request with a content type: text/plain
      //http.addHeader("Content-Type", "text/plain");
      //int httpResponseCode = http.POST("Hello, World!");

      StmSerial.print("HTTP Response code: ");
      StmSerial.println(httpsResponseCode);

      // Free resources
      https.end();
    }
    else {
      StmSerial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }
}