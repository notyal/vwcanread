#include "vwcanread.h"
#include <Arduino.h>
#include <Canbus.h>
#include <defaults.h>
#include <global.h>
#include <mcp2515.h>
#include <mcp2515_defs.h>

void setup() {
  Serial.begin(115200);
  Serial.println();

  if (Canbus.init(CANSPEED_500))
    Serial.println("I] CAN Init OK");
  else
    Serial.println("I] CAN Init ERROR");

  delay(1000);
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
    Serial.println("I] Dumping canbus messages... [press any key to exit]");
    delay(500);
    while (!Serial.available())
      dumpMessages();
    break;

  default:
    helpCmd();
    break;
  }
}

// help cmd
void helpCmd() {
  Serial.println();
  Serial.println("I] Valid commands:");
  Serial.println("I] D  dump canbus data");
}

// D  dump canbus messages
void dumpMessages() {
  tCAN message;

  if (mcp2515_check_message() && mcp2515_get_message(&message) &&
      !(message.id == 0x280 || message.id == 0x284 || message.id == 0x288 ||
        message.id == 0x380 || message.id == 0x480 || message.id == 0x488 ||
        message.id == 0x580 || message.id == 0x588)) {

    Serial.print("D] 0x");
    Serial.print(message.id, HEX);
    Serial.print("  ");
    char buf[3] = {0};
    for (int i = 0; i < message.header.length; i++) {
      snprintf(buf, sizeof(buf), "%02x", message.data[i]);
      Serial.print(buf);
    }
    Serial.println("");
  }
}
