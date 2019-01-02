#include "config.h"

#ifndef VWTPCHANNEL_H
#define VWTPCHANNEL_H

#include "canwrapper.h"

typedef enum tCANRxState {
  READY,
  MSG_WAIT,
  CHAN_TEST,
} tCANRxState;

typedef struct {
  bool ready = false; // true when can message is ready to be sent
  tCanFrame frame;
} tCanBuf;

typedef struct {
  bool ready = false; // true when serial is ready
  char data[SERIAL_BUF_SIZE];
} tSerialBuf;

class VWTPCHANNEL {
public:
  VWTPCHANNEL();
  void RunWhile(bool (*)());
  void RunCmds();
  void Enable();
  bool Execute();
  void Print(const char *);
  void Printf(const char *, ...);
  void Println();
  void Println(const char *);
  void TX(tCanFrame);
  void TXInternal(tCanFrame);
  void DebugPrintPacket(tCanFrame, const char *);
  void SendKWP2000PacketStr(const char *, uint8_t);

private:
};

#endif /* VWTPCHANNEL_H */
