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
Scheduler tsLow;
Scheduler tsHigh;
Scheduler tsCrit;

// callbacks
void taskMainCallback();
void taskPrintSerialCallback();
void taskRxMsgCallback();
void taskTxMsgCallback();
void taskChanTestCallback();
void taskRxSerialCmdCallback();

// tasks
// clang-format off
Task taskMain(TASK_IMMEDIATE, TASK_FOREVER, &taskMainCallback, &tsLow);
Task taskPrintSerial(10 * TASK_MILLISECOND, TASK_FOREVER, &taskPrintSerialCallback, &tsLow);
Task taskRxMsg(TASK_IMMEDIATE, TASK_FOREVER, &taskRxMsgCallback, &tsHigh);
Task taskChanTest(TASK_IMMEDIATE, TASK_FOREVER, &taskChanTestCallback, &tsCrit);
Task taskRxSerialCmd(TASK_IMMEDIATE, TASK_FOREVER, &taskRxSerialCmdCallback);
// clang-format on

VWTP20 v;
VWTPCHANNEL vc;
tCANRxState rxstate;

// task vars
unsigned long lastMsgMs = 0;          // millis when last message was sent
RingBuf<tCanFrame, 16> canRxFrameBuf; // 208 bytes
tCanBuf txTaskSend;
tCanBuf txTaskSendInternal; // for responding to ack messages
tSerialBuf serialBuf;
char _printBuf[128]; // for VWTPCHANNEL::Printf
char packetToCharBuf[60];
tDataBlock dataBuf;
bool dataBufReady = false;
bool cmdExit = false;

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
// run channel and receive serial commands
void VWTPCHANNEL::RunCmds() {
  Serial.setTimeout(2); // 2ms/((1000ms/115200bps)*(9bits)) = 25 bytes per msg
  tsLow.addTask(taskRxSerialCmd);
  Enable();
  cmdExit = false;
  while (!cmdExit)
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
  vsprintf(_printBuf, fmt, va);
  va_end(va);

  Print(_printBuf);
}

////////////////////////////////////////////////////////////////////////
// async println to serial
void VWTPCHANNEL::Println() { Print("\r\n"); }

////////////////////////////////////////////////////////////////////////
void VWTPCHANNEL::Println(const char *msg) {
  Print(msg);
  Println();
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
  switch (count) {
  case 0: // Start kwp2000 debug 1000021089
    vc.Println("c] Connected With Timing");
    // tCanFrame f;
    // f.id = v.GetClientID();
    // f.length = 5;
    // f.data[0] = 0x10;
    // f.data[1] = 0x00;
    // f.data[2] = 0x02;
    // f.data[3] = 0x10;
    // f.data[4] = 0x89;
    // vc.TX(f);

    count++;
    break;
  case 1:
    break;
  default:
    break;
  }

  // TODO
  while (!canRxFrameBuf.isEmpty()) {
    // TODO Count for sequence
    tCanFrame f;
    canRxFrameBuf.pop(f);
    vc.DebugPrintPacket(f, "MDATA");
  }

  if (dataBufReady) {
    dataBufReady = false;
    // TODO: put data on sdcard
    vc.Printf("m] Rx data len %d\r\n", dataBuf.length);
    // Serial.println("binarydata]\7");
    // for (uint8_t i = 0; i < dataBuf.length; i++)
    //   Serial.print(dataBuf.buf[i]);
    // Serial.println("\7[endbinarydata");
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
  if (v.CheckData(&f)) {
    v.ReadData(&dataBuf);
    dataBufReady = true;
  }

  // send channel test response
  if (f.length == 1 && f.data[0] == VWTP_TPDU_CONNTEST) {
    // vc.Printf("c] ct %5lu tx resp\r\n", millis());
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
    else {
      v.SetConnected(ConnectionTestError);
      vc.Println("c] ConnectionTestError");
    }

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
  if (rxstate == MSG_WAIT) {
    if (!canRxFrameBuf.isFull()) {
      canRxFrameBuf.push(f);
    } else {
      vc.Println("c] rxbuf full");
    }
  }
  // else if (rxstate == READY)
  //   vc.DebugPrintPacket(f, "RX");
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
// RxSerialCmd : tsLow TASK_IMMEDIATE
void taskRxSerialCmdCallback() {
  static char buf[25];
  uint8_t buflen = 0;

  if (!Serial.available())
    return;

  switch (Serial.read()) {
  // exit
  case 'Q':
    cmdExit = true;
    return;

  // kwp2000 send
  case 'K': // K []
    buflen = Serial.readBytesUntil('\r', buf, sizeof(buf));

    if (buflen > 0) {
      // send kwp message

      // DEBUG
      vc.Printf("K buf[%d] ", buflen);
      for (uint8_t i = 0; i < buflen; i++)
        vc.Printf("%02x", buf[i]);
      vc.Println();

      vc.SendKWP2000PacketStr(buf, buflen);
    }
    break;

  default: // \r \n
    break;
  }
}

////////////////////////////////////////////////////////////////////////
void VWTPCHANNEL::DebugPrintPacket(tCanFrame f, const char *str) {
  // DEBUG
  v.PacketToChar(f, packetToCharBuf);

  vc.Printf("c] %s %d %5lu %s\r\n", str, v.GetConnected(), millis(),
            packetToCharBuf);
}

////////////////////////////////////////////////////////////////////////
// Convert string to kwp2000 packet
void VWTPCHANNEL::SendKWP2000PacketStr(const char *buf, uint8_t size) {
  if (v.GetConnected() < ConnectedWithTiming) {
    vc.Println("Kerr] not connected");
    return;
  }

  if (size % 2) {
    // odd
    vc.Println("Kwarn] given packet hex cannot be odd");
    return;
  }

  // uint16_t kwplen;
  // tCanFrame f;
  // f.id = v.GetClientID;
  // f.data[0] = 0x10;

  char tbuf[size], *pos = tbuf;
  memcpy(tbuf, buf, size);

  uint8_t val[size / 2];

  // https://stackoverflow.com/a/3409211
  for (size_t count = 0; count < sizeof val / sizeof *val; count++) {
    sscanf(pos, "%2hhx", &val[count]);
    pos += 2;
  }

  // DEBUG
  // for (uint8_t i = 0; i < sizeof(hbuf); i++) {
  //   bufdata[i] = ((hbuf >> (8 * i)) & 0xFF);
  // }

  vc.Printf("kpsa] [%d] ", sizeof(val));
  for (uint8_t i = 0; i < sizeof(val); i++) {
    vc.Printf("%02x", val[i]);
  }
  vc.Println();

  // // send packet
  // f.length = 3 + kwplen;
  // if (f.length > 8) {
  //   // need moar packets and f.data[0] will not be 0x10
  //   vc.Println("Kwarn] big packets not yet implemented");
  //   return;
  // }
  // f.data[1] = (kwplen & 0xFF00) >> 8;
  // f.data[2] = kwplen & 0x00FF;

  // vc.TX(f);
}