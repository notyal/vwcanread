#include "vwtp20.h"
#include "vwtp20defs.h"

#include <Arduino.h>
#include <CAN.h>
#include <SPI.h>

VWTP20::VWTP20() {}
VWTP20::~VWTP20() {}

// tVWTP VWTP20::Ack(uint8_t dest) {
//   tVWTP tp;
//   tp.dest = dest;
//   tp.op = VWTP_CHAN_ACK_POS;
//   return tp;
// }

// tVWTP_MSG VWTP20::ChanResp() {
//   // Identifier 0x201
//   tVWTP_MSG tp;

//   tp.dest = 0x01; // ECU
//   tp.op = VWTP_OP_CHAN_SETUP;
//   tp.tx_id_low = 0;
//   tp.info_tx = 0x10;

//   return tp;
// }

void VWTP20::Connect() {}

// void readPacket() {
//   CAN_Frame m;

//   if (CAN.available()) {
//     m = CAN.read();

//     // return if m.id matches
//     if (m.id == 0x280 || m.id == 0x284 || m.id == 0x288 || m.id == 0x380 ||
//         m.id == 0x480 || m.id == 0x488 || m.id == 0x580 || m.id == 0x588)
//       return;

//     Serial.print(F("D] 0x"));
//     Serial.print(m.id, HEX);
//     Serial.print(F("  "));
//     char mbuf[3] = {0};

//     if (!m.rtr)
//       for (byte i = 0; i < m.length; i++) {
//         snprintf(mbuf, sizeof(mbuf), "%02x", m.data[i]);
//         Serial.print(mbuf);
//       }

//     Serial.println();
//   }
// }