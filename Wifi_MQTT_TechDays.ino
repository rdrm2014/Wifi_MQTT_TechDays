#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <ESP8266WebServer.h>
#include <IRremoteESP8266.h>
#include <DHT.h>

const char* idESP8266= "ESP8266Client1";

/**
 * Flags Pins
 */
const boolean flagRELAY1 = true;
const boolean flagRELAY2 = true;
const boolean flagRELAY3 = true;
const boolean flagRELAY4 = true;

const boolean flagTEMP = true;
const boolean flagHUM = true;

/**
 * Configurações de Pins
 */
const int pinRELAY1 = 15;
const int pinRELAY2 = 14;
const int pinRELAY3 = 12;
const int pinRELAY4 = 13;

const int pinHum = A0;

/* 
 * Configurações da Temperatura 
 */
#define DHTPIN  2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE, 11);

/**
 * Configurações de Rede
 */
const char* ssid = "agricultura";
const char* password = "Password!23";

//IPAddress local_ip(192, 168, 0, 11);
//IPAddress gateway(192, 168, 0, 1);
//IPAddress subnet(255, 255, 255, 0);

/**
 * Configurações de MQTT
 */
const char* mqtt_server = "192.168.0.10";
WiFiClient espClient;
PubSubClient client(espClient);
char msg[50];

/**
 * Tempos de Atualização
 */
long lastTemp = 0;
long lastHum = 0;

/**
 * Chars Result
 */
char resultRelay1[200];
char resultRelay2[200];
char resultRelay3[200];
char resultRelay4[200];

char resultTemp[200];
char resultHum[200];

/**
 * Variaveis
*/
float temp;
int hum;


/**
 * Setup
 */
void setup() {
  delay(1000);
  Serial.begin(115200);
  
  Serial.print("Configuring access point...");
  if(flagRELAY1){
    pinMode(pinRELAY1, OUTPUT);  
  }
  if(flagRELAY2){
    pinMode(pinRELAY2, OUTPUT);  
  }
  if(flagRELAY3){
    pinMode(pinRELAY3, OUTPUT);  
  }
  if(flagRELAY4){
    pinMode(pinRELAY4, OUTPUT);  
  }

  if(flagHUM){
    pinMode(pinHum, INPUT);  
  }
  
  setup_WIFI();
  setup_MQTT();
}

/**
 * Loop
 */
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if(flagTEMP){
    mqttTemp(resultTemp);
  }

  if(flagHUM){
    mqttHum(resultHum);
  }
}

/************************************ WIFI ************************************/

/**
 * Setup WIFI
 */
void setup_WIFI() {
  delay(10);
  
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  
  //WiFi.config(local_ip, gateway, subnet);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  IPAddress myIP = WiFi.localIP();

  
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(myIP);
}

/**
 * Relay
 * @param      {char*}   result
 */
void Relay(String code, char* result, String topic) {
  
  if (code =="HIGH") {
    if(flagRELAY1 && topic == "ESP8266_Relay1_send") {
      digitalWrite ( pinRELAY1, HIGH);
    } else if(flagRELAY2 && topic == "ESP8266_Relay2_send") {
      digitalWrite ( pinRELAY2, HIGH);
    } else if(flagRELAY3 && topic == "ESP8266_Relay3_send") {
      digitalWrite ( pinRELAY3, HIGH);
    } else if(flagRELAY4 && topic == "ESP8266_Relay4_send") {
      digitalWrite ( pinRELAY4, HIGH);
    }
      
    snprintf(result, 200, "{\"relay\": HIGH}");
  } else if (code == "LOW") {
    if(flagRELAY1 && topic == "ESP8266_Relay1_send") {
      digitalWrite ( pinRELAY1, LOW);
    } else if(flagRELAY2 && topic == "ESP8266_Relay2_send") {
      digitalWrite ( pinRELAY2, LOW);
    } else if(flagRELAY3 && topic == "ESP8266_Relay3_send") {
      digitalWrite ( pinRELAY3, LOW);
    } else if(flagRELAY4 && topic == "ESP8266_Relay4_send") {
      digitalWrite ( pinRELAY4, LOW);
    }    
    snprintf(result, 200, "{\"relay\": LOW}");
  }
}

/**
 * Temp
 * @param      {char*}   result
 */
void Temp(char* result) {  
  hum = dht.readHumidity();
  temp = dht.readTemperature();  
  
  snprintf(result, 200, "{\"hum\": %d.%d, \"temp\": %d.%d}", (int)hum, (int)((hum - (int)hum) * 100), (int)temp, (int)((temp - (int)temp) * 100));
}

/**
 * Hum
 * @param      {char*}   result
 */
void Hum(char* result) { 
  int moistureValue = analogRead(pinHum);
  if(moistureValue<250) moistureValue=250;
  if(moistureValue>750) moistureValue=750;
  //Serial.print("moistureValue: ");  
  //Serial.println(moistureValue);  
  hum = 100-(((moistureValue-250))*100/500);
  
  //Serial.print("hum: ");
  //Serial.println(hum);
  snprintf(result, 200, "{\"hum\": %d}", (int)hum);
}

/************************************ MQTT ************************************/
/**
 * Setup MQTT
 */
void setup_MQTT() {
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

/**
 * Callback MQTT
 */
void callback(char* top, byte* payload, unsigned int length) {
  String topic = top;
  String message;
  
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {    
    message += (char)payload[i];   
  }
  if  (flagRELAY1 && topic == "ESP8266_Relay1_send"){
    mqttRelay(message, resultRelay1, topic); 
  } else if(flagRELAY2 && topic == "ESP8266_Relay2_send"){
    mqttRelay(message, resultRelay2, topic);
  } else if(flagRELAY3 && topic == "ESP8266_Relay3_send"){
    mqttRelay(message, resultRelay3, topic); 
  } else if(flagRELAY4 && topic == "ESP8266_Relay4_send"){
    mqttRelay(message, resultRelay4, topic);    
  }
}

/**
 * Reconnect
 */
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(idESP8266)) {
      Serial.println("connected");
      
      if(flagRELAY1){
        client.subscribe("ESP8266_Relay1_send");
      }
      if(flagRELAY2){
        client.subscribe("ESP8266_Relay2_send");
      }
      if(flagRELAY3){
        client.subscribe("ESP8266_Relay3_send");
      }
      if(flagRELAY4){
        client.subscribe("ESP8266_Relay4_send");
      }
      
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

/**
 * mqttRelay
 */
void mqttRelay(String code, char* result, String topic) {
  Relay(code, result, topic);
  if (flagRELAY1 && topic == "ESP8266_Relay1_send"){
    client.publish("ESP8266_Relay1", result);
  }else if(flagRELAY2 && topic == "ESP8266_Relay2_send"){
    client.publish("ESP8266_Relay2", result);
  }else if(flagRELAY3 && topic == "ESP8266_Relay3_send"){
    client.publish("ESP8266_Relay3", result);
  }else if(flagRELAY4 && topic == "ESP8266_Relay4_send"){
    client.publish("ESP8266_Relay4", result);
  } 
}

/**
 * mqttTemp
 */
void mqttTemp(char* result) {
  long now = millis();
  if (now - lastTemp > 10000) {
    lastTemp = now;
    Temp(result); 
    client.publish("ESP8266_Temp", result);
  }
}

/**
 * mqttHum
 */
void mqttHum(char* result) {
  long now = millis();
  if (now - lastHum > 10000) {
    lastHum = now;
    Hum(result); 
    client.publish("ESP8266_Hum", result);
  }
}
