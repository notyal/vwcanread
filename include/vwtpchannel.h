#include "config.h"

#ifndef VWTPCHANNEL_H
#define VWTPCHANNEL_H

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

#include "canwrapper.h"

typedef struct {
  bool ready = false; // true when can message is ready to be sent
  tCanFrame frame;
} tCanBuf;

typedef struct {
  bool ready = false; // true when serial is ready
  char data[SERIAL_BUF_SIZE];
} tSerialBuf;

void taskMainCallback();
void taskPrintSerialCallback();
void taskRxMsgCallback();
void taskTxMsgCallback();
void taskChanTestCallback();

// Schedulers:
// tsLow  -- low priority
//   * printSerial 10ms
// tsHigh -- high priority
//   * rxMsg TASK_IMMEDIATE
//   * txMsg 1ms
// tsCrit -- critical priority
//   * channelTest 10 ms
Scheduler tsLow, tsHigh, tsCrit;

Task taskMain(TASK_IMMEDIATE, TASK_FOREVER, &taskMainCallback, &tsLow);
Task taskPrintSerial(10, TASK_FOREVER, &taskPrintSerialCallback, &tsLow);
Task taskRxMsg(TASK_IMMEDIATE, TASK_FOREVER, &taskRxMsgCallback, &tsHigh);
Task taskTxMsg(1, TASK_FOREVER, &taskRxMsgCallback, &tsHigh);
Task taskChanTest(10, TASK_FOREVER, &taskChanTestCallback, &tsCrit);

class VWTPCHANNEL {
public:
  VWTPCHANNEL();
  void Enable();
  bool Execute();
  void Print(const char *);
  void Println(const char *);
  static void TX(tCanFrame);
  static void TXInternal(tCanFrame);

private:
};

#endif /* VWTPCHANNEL_H */
