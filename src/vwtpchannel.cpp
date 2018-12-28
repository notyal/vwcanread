// handle sending data in vwtp channel

#include "vwtpchannel.h"
#include "canwrapper.h"
#include "config.h"
#include "vwtp20.h"
#include <Arduino.h>

VWTP20 v;
VWTPCHANNEL vc;
tCANRxState rxstate;

// task vars
unsigned long lastMsgMs = 0; // millis when last message was sent

RingBuf<tCanFrame, 16> canRxFrameBuf; // 208 bytes

tCanBuf txTaskSend;
tCanBuf txTaskSendInternal; // for responding to ack messages
tSerialBuf serialBuf;

VWTPCHANNEL::VWTPCHANNEL() {
  tsLow.setHighPriorityScheduler(&tsHigh);
  tsHigh.setHighPriorityScheduler(&tsCrit);
  rxstate = READY;
}

// run channel until given function is false
// Warning: function must not have any delay()
void VWTPCHANNEL::RunWhile(bool (*func)()) {
  Enable();
  while (func())
    Execute();

  v.Disconnect();
}

// setup channel
void VWTPCHANNEL::Enable() {
  v.Connect();
  tsLow.enableAll(true);
}

// execute channel tasks
bool VWTPCHANNEL::Execute() { return tsLow.execute(); }

// PrintSerial : tsLow 10ms
void taskPrintSerialCallback() {
  if (serialBuf.ready) {
    Serial.print(serialBuf.data);
    serialBuf.ready = false;
  }
}

// async print to serial
void VWTPCHANNEL::Print(const char *msg) {
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
void VWTPCHANNEL::Println(const char *msg) {
  Print(msg);
  Print("\n");
}

// prepare transmitting message
void VWTPCHANNEL::TX(tCanFrame f) {
  // skip if already has message
  if (txTaskSend.ready) {
    vc.Println("w] txts OVERFLOW");
    return;
  }

  txTaskSend.ready = true;
  txTaskSendInternal.frame = f;
}

// prepare transmitting message for internal
void VWTPCHANNEL::TXInternal(tCanFrame f) {
  // skip if already has message
  if (txTaskSendInternal.ready) {
    vc.Println("w] txtsI OVERFLOW");
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
// CAN RECEIVE
void taskRxMsgCallback() {
  // rx only if CAN is avail
  if (!isCANAvail())
    return;

  tCanFrame f;
  CANReadMsg(&f);

  // handle channel test response
  if (rxstate == CHAN_TEST && f.length == 6) {
    if (f.data[0] == VWTP_TPDU_CONNACK) {
      v.SetConnected(ConnectedWithTiming);
      rxstate = READY;
      return;
    } else {
      v.SetConnected(ConnectionTestError);
      rxstate = READY;
      return;
    }
  }

  // prepare response if ack is required
  if (v.CheckDataACK(&f)) {
    tCanFrame f;
    v.PrepareDataACKResponse(true, &f);
    vc.TXInternal(f);
  }

  if (!canRxFrameBuf.isFull()) {
    canRxFrameBuf.push(f);
  }
}

// txMsg : tsHigh 1ms
// CAN TRANSMIT
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

// ChanTest : tsCrit TASK_IMMEDIATE
void taskChanTestCallback() {
  // only send chantest if connected with timing
  // or if tx timeout is soon
  if (rxstate != READY || v.GetConnected() < ConnectedWithTiming ||
      millis() - v.GetTxTimeoutMs() - (v.GetTxTimeoutMs() / 2) < lastMsgMs)
    return;

  tCanFrame f;
  f.id = v.GetClientID();
  f.length = 1;
  f.data[0] = VWTP_TPDU_CONNTEST;

  lastMsgMs = millis();
  rxstate = CHAN_TEST;
  vc.TX(f);
}