#include "config.h"

#ifndef CANWRAPPER_H
#define CANWRAPPER_H

#include <Arduino.h>
#include <RingBuf.h>

#ifdef CANLIB_SEEED
#include <mcp_can.h>
#include <mcp_can_dfs.h>
#endif

#ifdef CANLIB_MCNEIGHT
#include <CAN.h>
#include <SPI.h>
#endif

typedef struct {
  uint32_t id;
  uint8_t length;
  uint8_t data[8];
} tCanFrame; // 13 bytes

RingBuf<tCanFrame, 16> canRxFrameBuf; // 208 bytes

bool CANInit();
bool isCANAvail();
void CANReadMsg(tCanFrame *);
void CANSendMsg(tCanFrame);
void CANPacketPrint(const String &, tCanFrame);

#endif /* CANWRAPPER_H */