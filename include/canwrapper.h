#ifndef CANWRAPPER_H
#define CANWRAPPER_H

#include <Arduino.h>
#include <mcp_can.h>
#include <mcp_can_dfs.h>

typedef struct {
  uint32_t id;
  uint8_t length;
  uint8_t data[8];
} tCanFrame;

bool CANInit();
bool isCANAvail();
uint8_t CANReadMsg(tCanFrame *);
uint8_t CANSendMsg(tCanFrame);
void CANPacketPrint(const String &, tCanFrame);

#endif // CANWRAPPER_H