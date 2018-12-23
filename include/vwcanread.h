#include "config.h"

#ifndef VWCANREAD_H
#define VWCANREAD_H

#include <Arduino.h>

void readCmd(char);
void helpCmd();
void dumpMessages();
void dumpRam();
void liveMode();

#endif
