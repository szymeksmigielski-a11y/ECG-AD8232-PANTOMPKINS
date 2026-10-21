#include "driver/adc.h"

#define PIN_LO_PLUS  25
#define PIN_LO_MINUS 26
#define PIN_SDN      27

void setup() {
  Serial.begin(115200);

  pinMode(PIN_SDN,      OUTPUT);
  digitalWrite(PIN_SDN, HIGH);

  pinMode(PIN_LO_PLUS,  INPUT);
  pinMode(PIN_LO_MINUS, INPUT);

  // Wyłącz uśrednianie — 1 pomiar zamiast 64
  // To skraca czas analogRead z ~2ms do ~50µs
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11); // GPIO36
}

void loop() {
  static unsigned long lastTime = micros();
  unsigned long now = micros();

  if (now - lastTime >= 5000) { // 5000µs = 200Hz
    lastTime += 5000; // staly krok - czestotliwosc nie dryfuje ponizej 200Hz

    int adcValue = 0; // 0 = elektroda odlaczona (LO+ lub LO-)
    if (!digitalRead(PIN_LO_PLUS) && !digitalRead(PIN_LO_MINUS)) {
      adcValue = adc1_get_raw(ADC1_CHANNEL_0);
    }

    // krotka: (wartosc, timestamp_ms)
    Serial.printf("(%d, %lu)\n", adcValue, millis());
  }
}