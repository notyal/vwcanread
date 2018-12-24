#include "taskconfig.h"
#include "canwrapper.h"
#include "config.h"
#include "vwtp20.h"
#include <Arduino.h>

VWTP20 v;

// task vars
unsigned long lastMsgMs = 0; // millis when last message was sent

RingBuf<tCanFrame, 16> canRxFrameBuf; // 208 bytes

bool txReady = false; // true when can message is ready to be sent
tCanFrame txTaskSend;

bool serialReady = false; // true when serial is ready to be sent
char serialBuf[SERIAL_BUF_SIZE];

TASKCONFIG::TASKCONFIG() {
  tsLow.setHighPriorityScheduler(&tsHigh);
  tsHigh.setHighPriorityScheduler(&tsCrit);
}

void TASKCONFIG::Enable() { tsLow.enableAll(true); }

bool TASKCONFIG::Execute() { return tsLow.execute(); }

// PrintSerial : tsLow 10ms
void taskPrintSerialCallback() {
  if (serialReady) {
    Serial.print(serialBuf);
    serialReady = false;
  }
}

// async print to serial
void TASKCONFIG::Print(const char *msg) {
  // msg size cannot be bigger than serialbuf size or remaining buffer
  if (sizeof(msg) > SERIAL_BUF_SIZE ||
      (serialReady && sizeof(msg) > SERIAL_BUF_SIZE - strlen(serialBuf)))
    return;

  if (!serialReady) {
    // new msg
    strcpy(serialBuf, msg);
  } else {
    // existing msg
    strcat(serialBuf, msg);
  }

  serialReady = true;
}

// async println to serial
void TASKCONFIG::Println(const char *msg) {
  Print(msg);
  Print("\n");
}

// rxMsg : tsHigh TASK_IMMEDIATE
void taskRxMsgCallback() {
  // rx only if CAN is avail
  if (!isCANAvail())
    return;

  tCanFrame f;
  CANReadMsg(&f);

  if (!canRxFrameBuf.isFull()) {
    canRxFrameBuf.push(f);
  }

  // TODO
}

// txMsg : tsHigh 1ms
void taskTxMsgCallback() {
  // send only if no tx is ready
  // or sent in last 5 ms
  if (!txReady || millis() - v.GetTxMinTimeMs() < lastMsgMs)
    return;

  CANSendMsg(txTaskSend);
  lastMsgMs = millis();

  txReady = false;
}

// ChanTest : tsCrit 10ms
void taskChanTestCallback() {
  // only send chantest if connected with timing
  // or if tx timeout is soon
  if (v.GetConnected() < ConnectedWithTiming ||
      millis() - v.GetTxTimeoutMs() - (v.GetTxTimeoutMs() / 2) < lastMsgMs)
    return;

  lastMsgMs = millis();
  v.ChannelTest();
}