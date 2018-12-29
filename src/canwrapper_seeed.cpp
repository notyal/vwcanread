#include "config.h"

#ifdef CANLIB_SEEED

#include <Arduino.h>
#include <canwrapper.h>
#include <mcp_can.h>
#include <mcp_can_dfs.h>

MCP_CAN CAN(10);

// Init CAN and setup masks/filters
bool CANInit() {
  uint8_t ret = CAN.begin(CAN_500KBPS);
  CAN.init_Mask(0, CAN_STDID, ~(0x201 | 0x300) & 0x7FF); // rx ecu mask
  CAN.init_Mask(1, CAN_STDID, ~(0x200 | 0x740) & 0x7FF); // tx client mask

  return ret == CAN_OK && CAN.checkError() == CAN_OK;
}

// Check if a CAN message has been rx
bool isCANAvail() { return CAN.checkReceive() == CAN_MSGAVAIL; }

// Read CAN rx message buffer into tCanFrame
void CANReadMsg(tCanFrame *f) {
  CAN.readMsgBuf(&f->length, f->data);
  f->id = CAN.getCanId();
}

// Send tCanFrame to CAN tx message buffer
void CANSendMsg(tCanFrame f) {
  CAN.sendMsgBuf(f.id, CAN_STDID, f.length, f.data);

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