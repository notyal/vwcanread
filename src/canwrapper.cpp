#include <Arduino.h>
#include <canwrapper.h>
#include <mcp_can.h>
#include <mcp_can_dfs.h>

#define DEBUG_CANSEND

MCP_CAN CAN(10);

bool CANInit() { return CAN_OK == CAN.begin(CAN_500KBPS); }

bool isCANAvail() { return CAN_MSGAVAIL == CAN.checkReceive(); }

uint8_t CANReadMsg(tCanFrame *f) {
  uint8_t ret = CAN.readMsgBuf(&f->length, f->data);
  f->id = CAN.getCanId();
  return ret;
}

uint8_t CANSendMsg(tCanFrame f) {
  uint8_t ret = CAN.sendMsgBuf(f.id, CAN_STDID, f.length, f.data);

#ifdef DEBUG_CANSEND
  CANPacketPrint(F("TX write]"), f);
#endif

  return ret;
}

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