#include <Arduino.h>
#include <SPI.h>
#include <EthernetUdp.h>
#include <Ethernet.h>

int count;
EthernetUDP Udp;
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 21};
IPAddress groundStation1(192, 168, 0, 70);
IPAddress ip(192, 168, 0, 42);
int port = 42069;

void setup()
{
  Serial.begin(921600);
  Ethernet.init(10);
  Ethernet.begin((uint8_t *)mac, ip);
  Udp.begin(port);
}

void loop()
{
  Serial.println("before begin");
  Serial.flush();
  Udp.beginPacket(groundStation1, port);
  Serial.println("before write");
  Serial.flush();
  char tosend[] = "test";
  Udp.write((unsigned char *) tosend, 4);
  Serial.println("before end");
  Serial.flush();
  Udp.endPacket();

  Serial.println(count);
  Serial.flush();
  count += 1;

  delay(1000);
}
