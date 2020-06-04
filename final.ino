
#include <Wire.h>
 #include <ESP8266WiFi.h>
#include <LiquidCrystal_I2C.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#include "DHT.h"        // including the library of DHT11 temperature and humidity sensor
#define DHTTYPE DHT11   // DHT 11

LiquidCrystal_I2C lcd(0x27, 20, 4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

#define WLAN_SSID       "SLT-4G_108644"
#define WLAN_PASS       "0767797972"

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    "Pravana"
#define AIO_KEY         "5f69041f36304bacaa9e9233be75ffb4"

#define dht_dpin D3
#define trigPin  D5
#define echoPin  D6
#define FLOAT_SENSOR  D7     // the number of the pushbutton pin

volatile int flow_frequency; // Measures flow sensor pulses
unsigned int onehour; // Calculated litres/hour
#define  flowsensor D0 // Sensor Input
unsigned long currentTime;
unsigned long cloopTime;

DHT dht(dht_dpin, DHTTYPE);
WiFiClient client;

Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

Adafruit_MQTT_Publish temp = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temp");
Adafruit_MQTT_Publish hum = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/hum");
Adafruit_MQTT_Publish water_level = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/water_level");
Adafruit_MQTT_Publish float_level = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/float_level");
Adafruit_MQTT_Publish flow_rate = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/flow_rate");

int t, h;
long duration;
int distance;
int alert;

void MQTT_connect();

void flow () {
  flow_frequency++;
}

void setup() {
  Serial.begin(115200);
  delay(10);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(1, 0);
  lcd.print("FLOOD DETECTION");
  lcd.setCursor(4, 1);
  lcd.print("SYSTEM");
  delay(500);

  dht.begin();

  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input


  pinMode(FLOAT_SENSOR, INPUT_PULLUP);
  pinMode(flowsensor, INPUT);
  digitalWrite(flowsensor, HIGH); // Optional Internal Pull-Up
  attachInterrupt(0, flow, RISING); // Setup Interrupt
  sei(); // Enable interrupts
  currentTime = millis();
  cloopTime = currentTime;
  Serial.println(F("Adafruit MQTT demo"));

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());

}

uint32_t x = 0;

void loop() {

  MQTT_connect();

  dh11();

  zonar();

  float_switch();

  water_flow();
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temperature: ");
  lcd.print(t);
  lcd.print(" C");
  lcd.setCursor(0, 1);
  lcd.print("Humidity: ");
  lcd.print(h);
  lcd.print("%");
  lcd.setCursor(0, 2);
  lcd.print("Water Level: ");
  lcd.print(distance);
  lcd.print(" cm");
  lcd.setCursor(0, 3);
  lcd.print("Flow rate: ");
  lcd.print(onehour);
  lcd.print(" L/hr");


  // tmp
  Serial.print(F("\nSending Temp "));
  Serial.print(t);
  Serial.print("...");
  if (! temp.publish(t)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }

  //humidity
  Serial.print(F("\nSending Hum  "));
  Serial.print(h);
  Serial.print("...");
  if (! hum.publish(h)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }

  //sonar
  Serial.print(F("\nSending Water Level  "));
  Serial.print(distance);
  Serial.print("...");
  if (! water_level.publish(distance)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }


  //float
  Serial.print(F("\nSending float Level  "));
  Serial.print(alert);
  Serial.print("...");
  if (! float_level.publish(alert)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }

   //water-flow
  Serial.print(F("\nSending water flow  "));
  Serial.print(onehour);
  Serial.print("...");
  if (! flow_rate.publish(onehour)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }
  
  delay(3000);
}


void water_flow() {
  currentTime = millis();
  // Every second, calculate and print litres/hour
  if (currentTime >= (cloopTime + 1000))
  {
    cloopTime = currentTime; // Updates cloopTime
    // Pulse frequency (Hz) = 7.5Q, Q is flow rate in L/min.
    onehour = (flow_frequency * 60 / 7.5); // (Pulse frequency x 60 min) / 7.5Q = flowrate in L/hour
    flow_frequency = 0; // Reset Counter
    Serial.print(onehour, DEC); // Print litres/hour
    Serial.println(" L/hour");
  }
  delay(1000);
}
void dh11() {
  h = dht.readHumidity();
  t = dht.readTemperature();
  Serial.print("Current humidity = ");
  Serial.print(h);
  Serial.print("%  ");
  Serial.print("temperature = ");
  Serial.print(t);
  Serial.println("C  ");
  delay(3000);
}

void zonar() {
  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  // Calculating the distance
  distance = duration * 0.034 / 2;
  // Prints the distance on the Serial Monitor
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println("cm");
  delay(2000);
}


void float_switch() {
  if (digitalRead(FLOAT_SENSOR) == LOW)
  {
    // turn LED on:
    Serial.println("on");
    alert = 0;
  }
  else
  {
    // turn LED off:
    Serial.println("off");
    alert = 1;
  }
}


void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds
    retries--;
    if (retries == 0) {
      // basically die and wait for WDT to reset me
      while (1);
    }
  }
  Serial.println("MQTT Connected!");
}
