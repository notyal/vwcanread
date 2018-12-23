// vwcanread.cpp
#include "vwcanread.h"
#include "config.h"
#include "vwtp20.h"
#include <Arduino.h>
#include <canwrapper.h>

#ifdef DEBUG_MEMORY
#define DEBUG_MEMORY_MS 5000 // min ms between printing memory messages
#include <MemoryFree.h>
#endif

void setup() {
  Serial.begin(115200);
  Serial.println();

  if (CANInit()) {
    // delay(500);
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

    // Run while no serial is rx
    // while (!Serial.available())
    dumpRam();
    break;

  case 'L':
    Serial.println(F("I] Live mode.. [use Q to exit]"));
    liveMode();
    break;

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
  Serial.println(F("I] L  live mode"));
}

// D  dump canbus messages
void dumpMessages() {

  if (isCANAvail()) {
    tCanFrame f;
    CANReadMsg(&f);

    VWTP20::PrintPacketMs(f);
  }
}

// R  dump ecu ram
void dumpRam() {
  Serial.println(F("R] Dumping ram..."));
  VWTP20 v;
  v.Connect();
  unsigned int chantestMicros = v.MsToMicros(v.GetTxTimeoutMs());
  unsigned int mintimeMicros = v.MsToMicros(v.GetTxMinTimeMs());
  unsigned long t1;
  uint16_t mseq = 0;
  tCanFrame f, resp;

  while (v.GetConnected() >= ConnectedWithTiming) {
    t1 = micros();
    // send chantest at least mintimeMicros before timeout
    if (micros() >= (t1 + chantestMicros - mintimeMicros))
      v.ChannelTest();

    if (mseq++ == 0) {
      f.id = v.GetClientID();
      f.length = 5;
      f.data[0] = 0x10;
      f.data[1] = 0x00;
      f.data[2] = 0x02;
      f.data[3] = 0x10;
      f.data[4] = 0x89;

      resp = v.AwaitECUResponseCmd(f, 0xB1);
      if (resp.length == 1 && resp.data[0] == 0xB1) {
        tCanFrame r2 = v.AwaitECUResponseCmd(0x10);
        delayMicroseconds(mintimeMicros);
        if (r2.length == 5 && r2.data[0] == 0x10) {
          f.length = 1;
          f.data[0] = 0xB1;
          CANSendMsg(f);
          v.PrintPacketMs(r2);
        }
      }
    }

    if (Serial.available() && Serial.read() == 'Q') {
      f.id = v.GetClientID();
      f.length = 1;
      f.data[0] = VWTP_TPDU_DISCONN;
      v.AwaitECUResponse(f);
      return;
    }

    // delay between messages
    delayMicroseconds(mintimeMicros);
  }

  if (v.GetConnected() < ConnectedWithTiming) {
    Serial.println(F("R] Error connecting to ECU."));

    Serial.print(F("E] GetClientID="));
    Serial.println(v.GetClientID(), HEX);

    Serial.print(F("E] GetEcuID="));
    Serial.println(v.GetEcuID(), HEX);

    Serial.print(F("E] GetConnected="));
    Serial.println(v.GetConnected());

    Serial.print(F("E] GetTxMinTimeMs="));
    Serial.println(v.GetTxMinTimeMs());

    Serial.print(F("E] GetTxTimeoutMs="));
    Serial.println(v.GetTxTimeoutMs());

    return;
  }

  Serial.println(F("R] Done."));
  Serial.print(F("E] GetConnected="));
  Serial.println(v.GetConnected());
}

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
