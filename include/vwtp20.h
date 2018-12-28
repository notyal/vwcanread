#include "config.h"

#ifndef VWTP20_H
#define VWTP20_H

#include "vwtp20defs.h"
#include <Arduino.h>
#include <canwrapper.h>

typedef enum CONNECTION {
  ConnectionResponseError,
  ConnectionTestError,
  ConnectionTimingError,
  ConnectionError,
  NotConnected,
  Connected,
  ConnectedWithTiming,
} CONNECTION;

typedef enum VWTPMsgType {
  BCAST,     // length: 7 bytes
  CHANSETUP, // length: 7 bytes
  CHANPARAM, // length: 1 or 6 bytes
  DATA,      // length: 2-8 bytes
} VWTPMsgType;

class VWTP20 {
public:
  VWTP20();
  void Connect();
  void Disconnect();
  static void PrintPacket(tCanFrame);
  static void PrintPacketMs(tCanFrame);
  static unsigned int MsToMicros(float);

  uint8_t GetSequence();
  uint8_t NextSequence();
  uint8_t ResetSequence();
  uint32_t GetClientID();
  uint32_t GetEcuID();
  CONNECTION GetConnected();
  void SetConnected(CONNECTION c);
  float GetTxTimeoutMs(); // T1
  float GetTxMinTimeMs(); // T3
  // bool ChannelTest();
  // tCanFrame AwaitECUResponse();
  // tCanFrame AwaitECUResponse(tCanFrame);
  // tCanFrame AwaitECUResponseCmd(uint8_t);
  // tCanFrame AwaitECUResponseCmd(tCanFrame, uint8_t);

  bool CheckDataACK(tCanFrame *);
  void PrepareDataACKResponse(bool, tCanFrame *);

private:
  uint8_t sequence;
  uint32_t clientID;
  uint32_t ecuID;
  CONNECTION connected;
  float txTimeoutMs; // T1
  float txMinTimeMs; // T3
  bool isLastPacket;

  // int prepVwtpMsg(uint8_t *buf, tVWTP_MSG msg);
  void channelInit();
  void setTiming();
  float decodeTimingMs(uint8_t);
};

#endif
