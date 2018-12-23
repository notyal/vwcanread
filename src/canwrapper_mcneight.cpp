#include "config.h"

#ifdef CANLIB_MCNEIGHT

#include <Arduino.h>
#include <CAN.h>
#include <SPI.h>
#include <canwrapper.h>

// Init CAN and setup masks/filters
bool CANInit() {
  CAN.begin(CAN_BPS_500K, MCP2515_MODE_CONFIG);
  CAN_Filter mask;

  // rx ecu mask FIXME
  mask.id = ~(0x201 | 0x300) & 0x7FF;
  CAN.setMask(0, mask);

  // tx client mask FIXME
  mask.id = ~(0x201 | 0x300) & 0x7FF;
  CAN.setMask(1, mask);

  CAN.begin(CAN_BPS_500K);

  return true;
}

// Check if a CAN message has been rx
bool isCANAvail() { return CAN.available(); }

// Read CAN rx message buffer into tCanFrame
void CANReadMsg(tCanFrame *f) { CAN.read(&f->id, &f->length, f->data); }

// Send tCanFrame to CAN tx message buffer
void CANSendMsg(tCanFrame f) {
  CAN.write(f.id, CAN_STANDARD_FRAME, f.length, f.data);

#ifdef DEBUG_CANSEND
  CANPacketPrint(F("TX]"), f);
#endif
}

// Print tCanFrame to serial
void CANPacketPrint(const String &msg, tCanFrame m) {
  Serial.print(msg + F(" 0x"));
  Serial.print(m.id, HEX);
  Serial.print(F("  "));

  char mbuf[3];

  for (byte i = 0; i < m.length; i++) {
    snprintf(mbuf, sizeof(mbuf), "%02x", m.data[i]);
    Serial.print(mbuf);
  }

  Serial.println();
}

#endif