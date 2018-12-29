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
  void Enable();
  bool Execute();
  void Print(const char *);
  void Printf(const char *, ...);
  void Println(const char *);
  void TX(tCanFrame);
  void TXInternal(tCanFrame);
  void DebugPrintPacket(tCanFrame, const char *);

private:
};

#endif /* VWTPCHANNEL_H */
