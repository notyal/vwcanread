// vwcanread.cpp
#include "vwcanread.h"
#include "config.h"
#include "vwtp20.h"
#include <Arduino.h>
#include <canwrapper.h>
#include <vwtpchannel.h>

#ifdef DEBUG_MEMORY
#define DEBUG_MEMORY_MS 5000 // min ms between printing memory messages
#include <MemoryFree.h>
#endif

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println(F(__FILE__));
  Serial.println(F("  Compiled: " __DATE__ " " __TIME__));
  Serial.print(F("   Arduino: "));
  Serial.println(ARDUINO);
  Serial.print(F("PlatformIO: "));
  Serial.println(PLATFORMIO);
  Serial.println();

  if (CANInit()) {
    delay(1000);
    Serial.println(F("CAN Init"));
    helpCmd();
  } else {
    Serial.println(F("CAN Init Error"));
    delay(500);
  }
}

// main loop
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

    dumpRam();
    break;

    // case 'L':
    //   Serial.println(F("I] Live mode.. [use Q to exit]"));
    //   liveMode();
    //   break;

  case '\n':
    break;

  case '\r':
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
  // Serial.println(F("I] L  live mode"));
}

// D  dump canbus messages
void dumpMessages() {

  if (isCANAvail()) {
    tCanFrame f;
    CANReadMsg(&f);

    VWTP20::PrintPacketMs(f);
  }
}

// R  dump ecu ram FIXME
void dumpRam() {
  Serial.println(F("R] Dumping ram..."));
  VWTPCHANNEL vc;

  // while !Serial.available()
  // vc.RunWhile([]() -> bool { return !Serial.available(); });
  vc.RunCmds();
}

/*
void liveMode() {
  unsigned long t, t2;
  char buf[128] = {0};
  while (true) {
    // dumpMessages();

    if (Serial.available()) {
      char cmd = Serial.read();

      switch (cmd) {
      case 'Q':
        Serial.println(F("L] Exiting Live mode..."));
        return;
        break;

      case 'T':
        // TX

        Serial.print(" L] GOT TX:'");
        t = millis();
        Serial.readBytesUntil('\n', buf, sizeof(buf));
        Serial.print(buf);
        t2 = millis();
        Serial.println("'.");
        Serial.print(F("Time(ms)="));
        Serial.println(t2 - t);
        Serial.print(buf);
        break;

      case '\n':
        break;

      case '\r':
        break;

      default:
        break;
      }
    }
  }
}
*/
