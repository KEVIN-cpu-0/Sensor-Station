#include <SPI.h>
#include <LoRa.h>

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // Clearing Previous Data
  Serial.write(12);
  for(int i =0; i < 10; i++){
    Serial.println();
  } 

  Serial.println("--- Multi Sensor LoRa Reciever ---");

  if (!LoRa.begin(915E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
}

void loop() {
  // try to parse packet
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String payload = "";

    // read packet
    while (LoRa.available()) {
      payload += (char)LoRa.read();
    }

    int firstComma = payload.indexOf(',');
    int secondComma = payload.indexOf(',', firstComma + 1);

    float co2Val = payload.substring(0, firstComma). toFloat();
    float tempVal = payload.substring(firstComma + 1, secondComma).toFloat();
    float humVal = payload.substring(secondComma + 1).toFloat();

    float phLevel = 6.35 + log10(alkalinity / (0.0301 * (co2Val / 1000.0)));
    
    // print sensor readings
    Serial.print("\n--- [ DATA RECEIVED ] ---");
    Serial.print("CO2 Concentration: "); Serial.print(co2Val); Serial.println(" ppm");
    Serial.print("Estimated pH:      "); Serial.println(phLevel,2);
    Serial.print("Temperature:       "); Serial.print(tempVal); Serial.println(" °C");
    Serial.print("Humidity:          "); Serial.print(humVal); Serial.println(" °%");
    Serial.print("Signal Strength:   "); Serial.print(LoRa.packetRssi()); Serial.println(" dBm");
    Serial.print("---------------------------");
  }
}