#ifndef VWTP20_H
#define VWTP20_H

#include "vwtp20defs.h"
#include <Arduino.h>
#include <canwrapper.h>

class VWTP20 {
public:
  VWTP20();
  void Connect();
  void PrintPacket(tCanFrame);

  uint8_t GetSequence();
  uint32_t GetClientID();
  uint32_t GetEcuID();
  uint8_t GetConnected();
  uint8_t GetTxTimeout();
  uint8_t GetTxMinTime();

private:
  uint8_t sequence;
  uint32_t clientID;
  uint32_t ecuID;
  uint8_t connected;
  uint8_t txTimeout;
  uint8_t txMinTime;

  // int prepVwtpMsg(uint8_t *buf, tVWTP_MSG msg);
  void init();
  void channelTest();
  void setTiming();
};

#endif
