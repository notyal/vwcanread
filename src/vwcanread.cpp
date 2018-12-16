// vwcanread.cpp
#include <Arduino.h>

// #define DEBUG_MEMORY
#include "vwcanread.h"
#include "vwtp20.h"
#include <CAN.h>
#include <SPI.h>

#ifdef DEBUG_MEMORY
#define DEBUG_MEMORY_MS 5000 // min ms between printing memory messages
#include <MemoryFree.h>
#endif

void setup() {
  Serial.begin(115200);
  Serial.println();

  CAN.begin(CAN_BPS_500K);
  delay(500);
  Serial.println(F("CAN Init"));
  helpCmd();
}

void loop() {
  while (Serial.available()) {
    char cmd = Serial.read();
    readCmd(cmd);
  }
}

// Handle cmd reading
void readCmd(char cmd) {
  switch (cmd) {
  case 'D':
    Serial.println(F("I] Dumping canbus messages... [press any key to exit]"));
    delay(500);

// Run while no serial is rx
#ifdef DEBUG_MEMORY
    unsigned long debug_memory_time = millis();
#endif
    while (!Serial.available()) {
#ifdef DEBUG_MEMORY
      if (millis() - debug_memory_time >= DEBUG_MEMORY_MS) {
        Serial.print(F("I] freeMemory()="));
        Serial.println(freeMemory());
        debug_memory_time = millis();
      }
#endif

      dumpMessages();
    }
    break;

  case 'R':
    Serial.println(F("I] Dumping ram... [press any key to exit]"));
    delay(500);

    // Run while no serial is rx
    // while (!Serial.available())
    dumpRam();
    break;

  default:
    helpCmd();
    break;
  }
}

// help cmd
void helpCmd() {
  Serial.println();
  Serial.println(F("I] Valid commands:"));
  Serial.println(F("I] D  dump canbus data"));
  Serial.println(F("I] R  dump ecu ram"));
}

// D  dump canbus messages
void dumpMessages() {
  CAN_Frame m;

  if (CAN.available()) {
    m = CAN.read();

    // return if m.id matches
    if (m.id == 0x280 || m.id == 0x284 || m.id == 0x288 || m.id == 0x380 ||
        m.id == 0x480 || m.id == 0x488 || m.id == 0x580 || m.id == 0x588)
      return;

    Serial.print(F("D] 0x"));
    Serial.print(m.id, HEX);
    Serial.print(F("  "));
    char mbuf[3] = {0};

    if (!m.rtr)
      for (byte i = 0; i < m.length; i++) {
        snprintf(mbuf, sizeof(mbuf), "%02x", m.data[i]);
        Serial.print(mbuf);
      }

    Serial.println();
  }
}

// R  dump ecu ram
void dumpRam() {
  VWTP20 v;
  v.Connect();

  Serial.print("GetClientID=");
  Serial.println(v.GetClientID(), HEX);

  Serial.print("GetEcuID=");
  Serial.println(v.GetEcuID(), HEX);
}
