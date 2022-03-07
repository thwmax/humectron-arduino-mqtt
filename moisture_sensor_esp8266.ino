/*
 Humectron
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Update these with values suitable for your network.

const char* ssid = "";
const char* password = "";
const char* mqtt_server = "";

unsigned long lastMsg = 0;
WiFiClient espClient;
PubSubClient client(espClient);

#define MQTT_KEEPALIVE 10000
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];

#define PLANT_1_TOPIC_INPUT "humectron/charmander_cmd"
#define PLANT_1_TOPIC_OUTPUT "humectron/charmander_measures"
#define PLANT_1_TOPIC_EVENTS "humectron/charmander_events"

const int AirValue = 766;   //you need to replace this value with Value_1
const int WaterValue = 523;  //you need to replace this value with Value_2
int intervals = (AirValue - WaterValue)/3;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  char data[length] = {};
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    data[i] = (char)payload[i];
  }
  
  Serial.println(data);  
  // Switch on the LED if an 1 was received as first character
  if (strcmp(data, "read") == 0) {
    sendMoistureLevel();
  }
}

void reconnect() {
  // Loop until we're reconnected
  client.setKeepAlive( MQTT_KEEPALIVE );
  
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(PLANT_1_TOPIC_EVENTS, "Wemos D1 connected");
      // ... and resubscribe
      client.subscribe(PLANT_1_TOPIC_INPUT);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

float readMoisture() {
  int soilMoistureValue = analogRead(A0);
  Serial.print("analog read=");
  Serial.println(soilMoistureValue);
  //put Sensor insert into soil
  float moisture = 100 - (soilMoistureValue * 100 / AirValue);
  return moisture;
}

void sendMoistureLevel() {
  float moisture = readMoisture();
  snprintf (msg, MSG_BUFFER_SIZE, "%.4f", moisture);
  Serial.print("sending moisture=");
  Serial.println(moisture);
  client.publish(PLANT_1_TOPIC_OUTPUT, msg);
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(9600);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 180000) {
    lastMsg = now;
    sendMoistureLevel();
  }
  delay(5000);
}
