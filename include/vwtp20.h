#ifndef VWTP20_H
#define VWTP20_H

#include "vwtp20defs.h"
#include <Arduino.h>
#include <canwrapper.h>

class VWTP20 {
public:
  VWTP20();
  void Connect();
  static void PrintPacket(tCanFrame);
  static void PrintPacketMs(tCanFrame);

  uint8_t GetSequence();
  uint32_t GetClientID();
  uint32_t GetEcuID();
  int8_t GetConnected();
  float GetTxTimeoutMs(); // T1
  float GetTxMinTimeMs(); // T3

private:
  uint8_t sequence;
  uint32_t clientID;
  uint32_t ecuID;
  int8_t connected;
  float txTimeoutMs; // T1
  float txMinTimeMs; // T3

  // int prepVwtpMsg(uint8_t *buf, tVWTP_MSG msg);
  void channelInit();
  void channelTest();
  void setTiming();
  float decodeTimingMs(uint8_t);
};

#endif
