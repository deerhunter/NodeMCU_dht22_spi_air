#include <ESP8266WiFi.h>
#include <DHT.h>
#include <PubSubClient.h>
#include <SimpleTimer.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341esp.h>
//#include <TFT_ILI9163C.h>

//declare pins

#define CS D8
#define DC D4
#define RST D0
#define TFT_MOSI 
#define TFT_CLK
#define TFT_RST
#define TFT_MISO

#define airPin D3
#define relaypin D2
#define temppin D1


//declare colors

#define WHITE 0xFFFF
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0


// Declare an instance of the ILI9163
//TFT_ILI9163C tft = TFT_ILI9163C(CS, RST, DC);

Adafruit_ILI9341 tft = Adafruit_ILI9341(CS, DC);

// declaew variables
//FOR AIR
int prevVal = LOW;
long th, tl, q, l, ppm;
String co2_str;
char co2[50];

//FOR TEMPERATURE
float t;
float h;
char temp[50];
char hum[50];
String temp_str; //see last code block below use these to convert the float that you get back from DHT to a string =str
String hum_str;

// FOR WIFI
const char* mqtt_server = "192.168.0.186"; ///your URL or IP address
//IPAddress server(192, 168, 0, 197);

//FOR TIME
long previousMillis = 0;
long interval = 60000;
SimpleTimer timer;

WiFiClient espClient;
PubSubClient client(espClient);
DHT dht;

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "miss_diavolica";
char pass[] = "";


void setup()
{
  tft.begin();
  tft.fillScreen(BLACK);
  tft.setTextColor(RED,BLACK);    
  tft.setTextSize(2);
  Serial.begin(9600); // See the connection status in Serial Monitor
   WiFi.begin(ssid, pass); //insert here your SSID and password
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    // Serial.println("connecting to wifi");
   tft.println("connecting to wifi");
  }
  tft.fillScreen(BLACK);
  tft.println("connected");
  delay(5000);
  tft.fillScreen(BLACK);
   // Setup a function to be called every second
  timer.setInterval(1000L, mqttpub);
  dht.setup(temppin);
  client.connect("KitchenESP");

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  pinMode(relaypin, OUTPUT);
  pinMode(airPin, INPUT);
 
}


//CHECK RELAY COMMANDS

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  
// Allocate the correct amount of memory for the payload copy
  byte* p = (byte*)malloc(length);
  // Copy the payload to the new buffer
  memcpy(p,payload,length);
  // Republish the received message


  if ((char)payload[0] == '0' || (char)payload[0] == 'f') {
    Serial.println("recieved command to close blinds full");

digitalWrite(relaypin, LOW);

 //client.publish("kitchen/windowstate",state); 
  } else {
    Serial.println("recieved command to open blinds to some degree");
digitalWrite(relaypin, HIGH);
    
  } 

 boolean rc=client.publish("kitchen/windowstate",p,length);
  // Free the memory
  free(p);

if (rc= true){
Serial.println("sent status success");
} else {
Serial.println("did not send status");
client.disconnect();
reconnect();
}

}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "openhab2";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      // client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("kitchen/windowcommand");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}



void loop()
{
  timer.run();
    if (!client.connected()) {
      client.disconnect();
    reconnect();
  }
 // mqttpub();
  client.loop();
  airquality();
  testText();
  
}



//READ CO2 DATA

void airquality()
{
    
long tt = millis();
int myVal = digitalRead(airPin);

if (myVal == HIGH) {
//    digitalWrite(LedPin, HIGH);
    if (myVal != prevVal) {
      q = tt;
      tl = q - l;
      prevVal = myVal;
    }
  }  else {
//    digitalWrite(LedPin, LOW);
    if (myVal != prevVal) {
      l = tt;
      th = l - q;
      prevVal = myVal;
      ppm = 5000 * (th - 2) / (th + tl - 4);
      Serial.println("PPM = " + String(ppm));
    }
  }
}
  

void mqttpub()
{

  float h = dht.getHumidity();
  float t = dht.getTemperature();

temp_str = String(t); //converting ftemp (the float variable above) to a string 
temp_str.toCharArray(temp, temp_str.length() + 1); //packaging up the data to publish to mqtt whoa...

hum_str = String(h); //converting Humidity (the float variable above) to a string
hum_str.toCharArray(hum, hum_str.length() + 1); //packaging up the data to publish to mqtt whoa...

co2_str = String(ppm); //converting Humidity (the float variable above) to a string
co2_str.toCharArray(co2, co2_str.length() + 1); //packaging up the data to publish to mqtt whoa...
        
 //   client.publish("room/temperature", ntpServerName);
boolean rc=client.publish("kitchen/humidity",hum);
boolean rc2=client.publish("kitchen/temperature",temp);
boolean rc3=client.publish("kitchen/co2",co2);

if (rc == true){
Serial.print("published humidity = ");
Serial.println(hum);
} else {
client.disconnect();
  reconnect();
}

if (rc2 == true){
Serial.print("published temperature = ");
Serial.println(temp);
} else {
client.disconnect();
  reconnect();
}

if (rc3 == true){
Serial.print("published CO2 = ");
Serial.println(co2);
} else {
client.disconnect();
  reconnect();
}

}


// SCREEN OUTPUT

void testText() {
  tft.setCursor(35, 15);
  tft.setTextColor(RED,BLACK);    
  tft.setTextSize(2);
  tft.print("Humid");
  tft.println(hum);
  tft.setCursor(20, 35);
  tft.setTextColor(RED,BLACK);    
  tft.setTextSize(3);
//  tft.println(ppi);
  // tft.println();
  tft.setCursor(40, 60);
  tft.setTextColor(RED,BLACK);    
  tft.setTextSize(2);
  tft.print("Temp");
  tft.println(temp);
 tft.setCursor(60, 80);
  tft.setTextColor(RED,BLACK);    
  tft.setTextSize(2);
  tft.print("Air");
  tft.println(co2);
 // tft.fillScreen(WHITE); 
//  tft.println(t1);

}


