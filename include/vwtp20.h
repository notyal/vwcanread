#include "config.h"

#ifndef VWTP20_H
#define VWTP20_H

#include "vwtp20defs.h"
#include <Arduino.h>
#include <canwrapper.h>

typedef enum CONNECTION {
  ConnectionResponseError = -4,
  ConnectionTestError = -3,
  ConnectionTimingError = -2,
  ConnectionError = -1,
  NotConnected = 0,
  Connected = 1,
  ConnectedWithTiming = 2
} CONNECTION;

class VWTP20 {
public:
  VWTP20();
  void Connect();
  static void PrintPacket(tCanFrame);
  static void PrintPacketMs(tCanFrame);
  static unsigned int MsToMicros(float);

  uint8_t GetSequence();
  uint32_t GetClientID();
  uint32_t GetEcuID();
  CONNECTION GetConnected();
  float GetTxTimeoutMs(); // T1
  float GetTxMinTimeMs(); // T3
  bool ChannelTest();
  tCanFrame AwaitECUResponse();
  tCanFrame AwaitECUResponse(tCanFrame);
  tCanFrame AwaitECUResponseCmd(uint8_t);
  tCanFrame AwaitECUResponseCmd(tCanFrame, uint8_t);

private:
  uint8_t sequence;
  uint32_t clientID;
  uint32_t ecuID;
  CONNECTION connected;
  float txTimeoutMs; // T1
  float txMinTimeMs; // T3

  // int prepVwtpMsg(uint8_t *buf, tVWTP_MSG msg);
  void channelInit();
  void setTiming();
  float decodeTimingMs(uint8_t);
};

#endif
