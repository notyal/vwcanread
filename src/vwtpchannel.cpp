// handle sending data in vwtp channel

#include "vwtpchannel.h"
#include "canwrapper.h"
#include "config.h"
#include "vwtp20.h"
#include <Arduino.h>
#include <stdarg.h>

// *****************************************************************************
// BEGIN TaskScheduler Configuration
// *****************************************************************************
// clang-format off

// #define _TASK_TIMECRITICAL      // Enable monitoring scheduling overruns
// #define _TASK_SLEEP_ON_IDLE_RUN // Enable 1 ms SLEEP_IDLE powerdowns between tasks if no callback methods were invoked during the pass
// #define _TASK_STATUS_REQUEST    // Compile with support for StatusRequest functionality - triggering tasks on status change events in addition to time only
// #define _TASK_WDT_IDS           // Compile with support for wdt control points and task ids
// #define _TASK_LTS_POINTER       // Compile with support for local task storage pointer
#define _TASK_PRIORITY          // Support for layered scheduling priority
// #define _TASK_MICRO_RES         // Support for microsecond resolution
// #define _TASK_STD_FUNCTION      // Support for std::function (ESP8266 and ESP32 ONLY)
// #define _TASK_DEBUG             // Make all methods and variables public for debug purposes
// #define _TASK_INLINE            // Make all methods "inline" - needed to support some multi-tab, multi-file implementations
// #define _TASK_TIMEOUT           // Support for overall task timeout
// #define _TASK_OO_CALLBACKS      // Support for dynamic callback method binding

#include <TaskScheduler.h>

// clang-format on
// *****************************************************************************
// END TaskScheduler Configuration
// *****************************************************************************

// Schedulers:
// tsLow  -- low priority
//   * printSerial 10ms
// tsHigh -- high priority
//   * rxMsg TASK_IMMEDIATE
// tsCrit -- critical priority
//   * channelTest TASK_IMMEDIATE
Scheduler tsLow, tsHigh, tsCrit;

// callbacks
void taskMainCallback();
void taskPrintSerialCallback();
void taskRxMsgCallback();
void taskTxMsgCallback();
void taskChanTestCallback();

// tasks
Task taskMain(TASK_IMMEDIATE, TASK_FOREVER, &taskMainCallback, &tsLow);
Task taskPrintSerial(10, TASK_FOREVER, &taskPrintSerialCallback, &tsLow);
Task taskRxMsg(TASK_IMMEDIATE, TASK_FOREVER, &taskRxMsgCallback, &tsHigh);
Task taskChanTest(TASK_IMMEDIATE, TASK_FOREVER, &taskChanTestCallback, &tsCrit);

VWTP20 v;
VWTPCHANNEL vc;
tCANRxState rxstate;

// task vars
unsigned long lastMsgMs = 0;          // millis when last message was sent
RingBuf<tCanFrame, 16> canRxFrameBuf; // 208 bytes
tCanBuf txTaskSend;
tCanBuf txTaskSendInternal; // for responding to ack messages
tSerialBuf serialBuf;
char printBuf[128];
char packetToCharBuf[60];

////////////////////////////////////////////////////////////////////////
VWTPCHANNEL::VWTPCHANNEL() {
  tsLow.setHighPriorityScheduler(&tsHigh);
  tsHigh.setHighPriorityScheduler(&tsCrit);
  rxstate = READY;

  txTaskSend.ready = false;
  txTaskSendInternal.ready = false;
}

////////////////////////////////////////////////////////////////////////
// run channel until given function is false
// Warning: function must not have any delay()
void VWTPCHANNEL::RunWhile(bool (*func)()) {
  Enable();
  while (func())
    Execute();

  v.Disconnect();
  Serial.println(F("c] Exit Channel."));
}

////////////////////////////////////////////////////////////////////////
// setup channel
void VWTPCHANNEL::Enable() {
  v.Connect();
  tsLow.enableAll(true);
}

////////////////////////////////////////////////////////////////////////
// execute channel tasks
bool VWTPCHANNEL::Execute() { return tsLow.execute(); }

////////////////////////////////////////////////////////////////////////
// prepare transmitting message
void VWTPCHANNEL::TX(tCanFrame f) {
  lastMsgMs = millis();
  CANSendMsg(f);
  vc.DebugPrintPacket(f, "TX");
}

////////////////////////////////////////////////////////////////////////
// prepare transmitting message for internal
void VWTPCHANNEL::TXInternal(tCanFrame f) {
  lastMsgMs = millis();
  CANSendMsg(f);
  // vc.DebugPrintPacket(f, "TXI");
}

// *****************************************************************************
// Print functions
// *****************************************************************************

////////////////////////////////////////////////////////////////////////
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

////////////////////////////////////////////////////////////////////////
// async printf to serial
void VWTPCHANNEL::Printf(const char *fmt, ...) {
  va_list va;
  va_start(va, fmt);
  vsprintf(printBuf, fmt, va);
  va_end(va);

  Print(printBuf);
}

////////////////////////////////////////////////////////////////////////
// async println to serial
void VWTPCHANNEL::Println(const char *msg) {
  Print(msg);
  Print("\r\n");
}

// *****************************************************************************
// Callback functions
// *****************************************************************************

////////////////////////////////////////////////////////////////////////
// taskMain : tsLow TASK_IMMEDIATE
void taskMainCallback() {
  if (rxstate != READY || v.GetConnected() < ConnectedWithTiming)
    return;

  static uint16_t count = 0;
  if (count == 0) {
    // TODO need to count sequence
    // Start debug 1000021089
    // tCanFrame f;
    // f.id = v.GetClientID();
    // f.length == ;
    // f.data[0] == 0x10;

    // vc.TX(f);
    count++;
  }
  // TODO
  while (!canRxFrameBuf.isEmpty()) {
    // TODO Count for sequence
    tCanFrame f;
    canRxFrameBuf.pop(f);
    vc.DebugPrintPacket(f, "MDATA");
  }
}

////////////////////////////////////////////////////////////////////////
// PrintSerial : tsLow 10ms
void taskPrintSerialCallback() {
  if (serialBuf.ready) {
    Serial.print(serialBuf.data);
    serialBuf.ready = false;
  }
}

////////////////////////////////////////////////////////////////////////
// rxMsg : tsHigh TASK_IMMEDIATE
// CAN RECEIVE
void taskRxMsgCallback() {
  // rx only if CAN is avail
  if (!isCANAvail())
    return;

  tCanFrame f;
  CANReadMsg(&f);

  // handle data stream
  // TODO

  // send channel test response
  if (f.length == 1 && f.data[0] == VWTP_TPDU_CONNTEST) {
    vc.Printf("c] ct %5lu tx resp\r\n", millis());
    tCanFrame f;
    f.id = v.GetClientID();
    f.length = 6;

    // a1 0f 8a ff 4a ff
    f.data[0] = VWTP_TPDU_CONNACK;
    f.data[1] = 0x0F; // Block size (num packets before ACK)
    f.data[2] = 0x8A; // T1 tx timeout
    f.data[3] = 0xFF; // T2 unused
    f.data[4] = 0x4A; // T3 min time for tx packet
    f.data[5] = 0xFF; // T4 unused

    vc.TXInternal(f);
  }

  // handle channel test response
  if (rxstate == CHAN_TEST && f.length == 6) {
    // vc.Printf("c] ct %5lu rx resp\r\n", millis());
    if (f.data[0] == VWTP_TPDU_CONNACK)
      v.SetConnected(ConnectedWithTiming);
    else
      v.SetConnected(ConnectionTestError);

    rxstate = READY;
    return;
  } else if (rxstate == CHAN_TEST) {
    vc.Println("c] rxstate CT");
  }

  // send response if ack is required
  if (v.CheckDataACK(&f)) {
    tCanFrame f;
    v.PrepareDataACKResponse(true, &f);
    vc.TXInternal(f);
  }

  // push message to buffer
  if (rxstate == MSG_WAIT)
    if (!canRxFrameBuf.isFull()) {
      canRxFrameBuf.push(f);
    } else {
      vc.Println("rxbuf full");
    }
  else if (rxstate == READY)
    vc.DebugPrintPacket(f, "RX");
}

////////////////////////////////////////////////////////////////////////
// ChanTest : tsCrit TASK_IMMEDIATE
void taskChanTestCallback() {
  // only send chantest if connected with timing
  // or if tx timeout is soon
  if (rxstate != READY || v.GetConnected() < ConnectedWithTiming ||
      millis() - (unsigned long)v.GetTxTimeoutMs() < lastMsgMs) {
    return;
  }

  tCanFrame f;
  f.id = v.GetClientID();
  f.length = 1;
  f.data[0] = VWTP_TPDU_CONNTEST;

  rxstate = CHAN_TEST;
  vc.TXInternal(f);

  // vc.Printf("c] %lu CT\r\n", millis());
}

////////////////////////////////////////////////////////////////////////
void VWTPCHANNEL::DebugPrintPacket(tCanFrame f, const char *str) {
  // DEBUG
  v.PacketToChar(f, packetToCharBuf);

  vc.Printf("c] %s %d %5lu %s\r\n", str, v.GetConnected(), millis(),
            packetToCharBuf);
}