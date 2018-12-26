// handle sending data in vwtp channel

#include "taskconfig.h"
#include "canwrapper.h"
#include "config.h"
#include "vwtp20.h"
#include <Arduino.h>

VWTP20 v;
TASKCONFIG tc;

// task vars
unsigned long lastMsgMs = 0; // millis when last message was sent

RingBuf<tCanFrame, 16> canRxFrameBuf; // 208 bytes

tCanBuf txTaskSend;
tCanBuf txTaskSendInternal; // for responding to ack messages
tSerialBuf serialBuf;

TASKCONFIG::TASKCONFIG() {
  tsLow.setHighPriorityScheduler(&tsHigh);
  tsHigh.setHighPriorityScheduler(&tsCrit);
}

void TASKCONFIG::Enable() { tsLow.enableAll(true); }

bool TASKCONFIG::Execute() { return tsLow.execute(); }

// PrintSerial : tsLow 10ms
void taskPrintSerialCallback() {
  if (serialBuf.ready) {
    Serial.print(serialBuf.data);
    serialBuf.ready = false;
  }
}

// async print to serial
void TASKCONFIG::Print(const char *msg) {
  // msg size cannot be bigger than serialbuf size or remaining buffer
  if (sizeof(msg) > SERIAL_BUF_SIZE ||
      (serialBuf.ready &&
       sizeof(msg) > SERIAL_BUF_SIZE - strlen(serialBuf.data)))
    return;

  if (!serialBuf.ready) {
    // new msg
    strcpy(serialBuf.data, msg);
  } else {
    // existing msg
    strcat(serialBuf.data, msg);
  }

  serialBuf.ready = true;
}

// async println to serial
void TASKCONFIG::Println(const char *msg) {
  Print(msg);
  Print("\n");
}

void TASKCONFIG::TX(tCanFrame f) {
  // skip if already has message
  if (txTaskSend.ready) {
    tc.Println("w] txts OVERFLOW");
    return;
  }

  txTaskSend.ready = true;
  txTaskSendInternal.frame = f;
}

void TASKCONFIG::TXInternal(tCanFrame f) {
  // skip if already has message
  if (txTaskSendInternal.ready) {
    tc.Println("w] txtsI OVERFLOW");
    return;
  }

  txTaskSendInternal.ready = true;
  txTaskSendInternal.frame = f;
}

// taskMain : tsLow TASK_IMMEDIATE
void taskMainCallback() {
  // TODO
}

// rxMsg : tsHigh TASK_IMMEDIATE
void taskRxMsgCallback() {
  // rx only if CAN is avail
  if (!isCANAvail())
    return;

  tCanFrame f;
  CANReadMsg(&f);

  // prepare response if ack is required
  if (v.CheckDataACK(&f)) {
    tCanFrame f;
    v.PrepareDataACKResponse(true, &f);
    tc.TXInternal(f);
  }

  if (!canRxFrameBuf.isFull()) {
    canRxFrameBuf.push(f);
  }
}

// txMsg : tsHigh 1ms
void taskTxMsgCallback() {
  // send ACK response first
  if (txTaskSendInternal.ready) {
    CANSendMsg(txTaskSendInternal.frame);
    lastMsgMs = millis();
    txTaskSendInternal.ready = false;
  }

  // send only if tx is ready
  if (txTaskSend.ready) {
    CANSendMsg(txTaskSend.frame);
    lastMsgMs = millis();
    txTaskSend.ready = false;
  }
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