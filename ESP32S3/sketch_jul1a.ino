#include <Wire.h>
#include <Adafruit_BME280.h>
#include <BH1750.h>

// I2C Bus 0 (BME1)   -> SDA: 8,  SCL: 9
// I2C Bus 1 (BME2)   -> SDA: 17, SCL: 18
// I2C Bus 2 (BH1750) -> SDA: 19, SCL: 20
// Note: Bus 0 HW is reused (end+begin with different pins) to read BH1750
#define SDA1_PIN 8
#define SCL1_PIN 9
#define SDA2_PIN 17
#define SCL2_PIN 18
#define SDA3_PIN 19
#define SCL3_PIN 20

#define BME1_ADDR 0x76
#define BME2_ADDR 0x76 

Adafruit_BME280 bme1;
Adafruit_BME280 bme2;

TwoWire I2Ctwo   = TwoWire(1);
TwoWire I2Cthree = TwoWire(0); // Reuse bus 0 hw, but with different pins for BH1750

// Soil Sensors
#define SOIL_SENSOR_1 12
#define SOIL_SENSOR_2 13

BH1750 lightMeter;

// Relays
#define RELAY1 4
#define RELAY2 5
#define RELAY3 15
#define RELAY4 16
#define RELAY5 35
#define RELAY6 36
#define RELAY7 21
#define RELAY8 38

bool bme1_ok = false;
bool bme2_ok = false;

void initBME1() {
  Wire.begin(SDA1_PIN, SCL1_PIN);
  bme1_ok = bme1.begin(BME1_ADDR, &Wire);
  if (!bme1_ok) bme1_ok = bme1.begin(0x77, &Wire);
}

void initBME2() {
  bme2_ok = bme2.begin(BME2_ADDR, &I2Ctwo);
  if (!bme2_ok) bme2_ok = bme2.begin(0x77, &I2Ctwo);
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(50);

  // Init BME2 on Bus 1 first (stays permanently on GPIO 17/18)
  I2Ctwo.begin(SDA2_PIN, SCL2_PIN);
  initBME2();

  // Init BME1 on Bus 0 (GPIO 8/9)
  initBME1();

  if (bme1_ok) Serial.println("OK: BME1");
  else Serial.println("FAIL: BME1");

  if (bme2_ok) Serial.println("OK: BME2");
  else Serial.println("FAIL: BME2");

  // Setup Relays
  int relays[] = {RELAY1, RELAY2, RELAY3, RELAY4, RELAY5, RELAY6, RELAY7, RELAY8};
  for (int pin : relays) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
  }

  Serial.println("READY");
}

float readBH1750() {
  // Temporarily take over Bus 0 hardware with BH1750 pins
  Wire.end();
  delay(20);
  Wire.begin(SDA3_PIN, SCL3_PIN); // GPIO 6/7
  delay(50);

  bool ok = lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire);
  float lux = -1;
  if (ok) {
    delay(200);
    lux = lightMeter.readLightLevel();
  }

  // Restore Bus 0 back to BME1 (GPIO 8/9)
  Wire.end();
  delay(20);
  initBME1();

  return lux;
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "get_temp_all") {
      if (bme1_ok) {
        float t1 = bme1.readTemperature();
        float h1 = bme1.readHumidity();
        Serial.printf("BME1 TEMP: %.2f C, HUMID: %.2f %%\n", t1, h1);
      } else {
        Serial.println("BME1 ERROR");
      }
      delay(10);
      if (bme2_ok) {
        float t2 = bme2.readTemperature();
        float h2 = bme2.readHumidity();
        Serial.printf("BME2 TEMP: %.2f C, HUMID: %.2f %%\n", t2, h2);
      } else {
        Serial.println("BME2 ERROR");
      }
    }

    else if (cmd == "get_soil") {
      int soil1 = analogRead(SOIL_SENSOR_1);
      int soil2 = analogRead(SOIL_SENSOR_2);
      Serial.printf("SOIL1: %d\n", soil1);
      delay(10);
      Serial.printf("SOIL2: %d\n", soil2);
    }

    else if (cmd == "get_light") {
      float lux = readBH1750();
      if (lux >= 0) {
        Serial.printf("LIGHT: %d lx\n", (int)lux);
      } else {
        Serial.println("BH1750 ERROR");
      }
    }

    else if (cmd.startsWith("relay_on")) {
      int num = cmd.substring(9).toInt();
      controlRelay(num, LOW);
      Serial.printf("RELAY %d ON\n", num);
    }

    else if (cmd.startsWith("relay_off")) {
      int num = cmd.substring(10).toInt();
      controlRelay(num, HIGH);
      Serial.printf("RELAY %d OFF\n", num);
    }

    else if (cmd.length() > 0) {
      Serial.printf("UNKNOWN: %s\n", cmd.c_str());
    }
  }
}

void controlRelay(int num, int state) {
  int pin = -1;
  switch (num) {
    case 1: pin = RELAY1; break;
    case 2: pin = RELAY2; break;
    case 3: pin = RELAY3; break;
    case 4: pin = RELAY4; break;
    case 5: pin = RELAY5; break;
    case 6: pin = RELAY6; break;
    case 7: pin = RELAY7; break;
    case 8: pin = RELAY8; break;
  }
  if (pin != -1) {
    digitalWrite(pin, state);
  }
}
