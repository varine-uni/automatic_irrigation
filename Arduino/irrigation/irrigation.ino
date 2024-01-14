#include "DHT.h"
#define DHT_DIGITAL_PIN 6
#define DHTTYPE DHT11
#define SOIL_MOISTURE_ANALOG_PIN 0
#define SOIL_MOISTURE_VCC_PIN 8
#define WATER_PUMP_DIGITAL_PIN 3

uint16_t minimumMoisture; // The minimum amount of moisture (%) to start watering
uint16_t moistureValue; // Current moisture
uint16_t waterDuration; // How long the pump will water the plant

DHT dht(DHT_DIGITAL_PIN, DHTTYPE);

bool turnWaterOn(uint16_t duration) {
  digitalWrite(WATER_PUMP_DIGITAL_PIN, HIGH);
  delay((duration*1000));
} 

bool turnWaterOff() {
  digitalWrite(WATER_PUMP_DIGITAL_PIN, LOW);
}

bool turnOnSoilSensor() {
  digitalWrite(SOIL_MOISTURE_VCC_PIN, HIGH);
}

bool turnOffSoilSensor() {
  digitalWrite(SOIL_MOISTURE_VCC_PIN, LOW);
}

void setPlantSpecs() {
  String str_1 = "";
  String str_2 = "";
  int comma_position = 0;
  char buffer[70];

  while (true) {
    if (Serial.available() > 0) {
      String data = Serial.readStringUntil("\n");

      comma_position = data.indexOf(",");
      str_1 = data.substring(0, comma_position);
      minimumMoisture = str_1.toInt();
      data = data.substring(comma_position + 1, data.length());

      comma_position = data.indexOf(",");
      str_2 = data.substring(0, comma_position);
      waterDuration = str_2.toInt();
      data = data.substring(comma_position + 1, data.length());

      sprintf(buffer, "Watering duration set to: %d\nMinimum moisture set to: %d", waterDuration, minimumMoisture);
      Serial.print(buffer);

      if (minimumMoisture != 0 && waterDuration != 0) {
        return;
      }
    }
    delay(1000);
  }
}

void setup() {
  Serial.begin(115200); // set baud
  pinMode(WATER_PUMP_DIGITAL_PIN, OUTPUT);
  dht.begin(); // initialize the sensor
  setPlantSpecs(); // set watering requirements
}

void loop() {
  
  if (Serial.available() > 0) {
    // flag to check if the pump was turned on during this loop.
    String waterFlag = "false"; 
    // DHT11 sensor 
    // read humidity
    float humidity  = dht.readHumidity();
    delay(100);
    // read temperature as Celsius
    float tempCelsius = dht.readTemperature();
    delay(100);

    // check if any reads failed
    if (isnan(humidity) || isnan(tempCelsius)) {
      Serial.println("Failed to read from DHT sensor!");
    } else {
      Serial.print("Humidity: ");
      Serial.print(humidity);
      Serial.print("%\n");
      Serial.print("Temperature: ");
      Serial.print(tempCelsius);
      Serial.print("C\n");
    }

    // soil moisture sensor
    // read moisture levels
    // these turn on and off soil sensor functions are very important as water damages the sensor (when it's on) so it's important to only turn it on when needed.
    turnOnSoilSensor();
    delay(1000);
    moistureValue = analogRead(SOIL_MOISTURE_ANALOG_PIN);
    moistureValue = map(moistureValue, 145, 218, 100, 0);
    Serial.print("Moisture: ");
    Serial.print(moistureValue);
    Serial.print("%\n");
    turnOffSoilSensor();

    if (moistureValue <= minimumMoisture) {
      waterFlag = "true";
      turnWaterOn(waterDuration);
      turnWaterOff();
    }

    Serial.print("Water: ");
    Serial.print(waterFlag);
    Serial.print("\n");

    // wait a few seconds between measurements
    delay(10000);
  }
}
