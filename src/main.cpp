#include "Comms.h"

#include <Arduino.h>
#include <SPI.h>

uint8_t result;

void setup() {
  Serial.begin(921600);

  result = Comms::init(42);
}

void loop() {
  Serial.println("test");
  Serial.println(Comms::getLinkStatus());

  Comms::setReceiverIPandPort(70, 42069);
  uint8_t tosend[] = "test";
  Comms::sendRawPacket(tosend, 4);

  uint8_t buff[4];
  Comms::readSUBR(buff);
  Serial.println(buff[0]);
  Serial.println(buff[1]);
  Serial.println(buff[2]);
  Serial.println(buff[3]);
  
  delay(1000);
}

