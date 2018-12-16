#include "vwtp20.h"
#include "vwtp20defs.h"
#include <Arduino.h>
#include <CAN.h>
#include <SPI.h>

VWTP20::VWTP20() {
  sequence = 0;
  clientID = 0x000; // rx
  ecuID = 0x000;    // tx
  connected = 0;
  txTimeout = 0;
  txMinTime = 0;
}

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

// Getters
uint8_t VWTP20::GetSequence() { return sequence; }
uint32_t VWTP20::GetClientID() { return clientID; }
uint32_t VWTP20::GetEcuID() { return ecuID; }
uint8_t VWTP20::GetConnected() { return connected; }
uint8_t VWTP20::GetTxTimeout() { return txTimeout; }
uint8_t VWTP20::GetTxMinTime() { return txMinTime; }

void VWTP20::Connect() {
  // TODO
  // 1 Setup KWP2000
  // 2 Dump ram
  //   - REF: http://nefariousmotorsports.com/forum/index.php?topic=9182.5
  //   - REF: https://github.com/seishuku/teensycanbusdisplay
  // send chan_test messages for keepalive

  CAN.clearFilter();
  uint8_t counter = 0;

  // setup channel
  do {
    delay(50);
    init();
    if (CAN.available()) {
      CAN_Frame f = CAN.read();
      // 00 d0 00 03 40 07 01
      if (f.id == 0x201 && f.length == 7 && f.data[1] == VWTP_OP_CHAN_ACK_POS) {
        // tVWTP_MSG msg;
        // memcpy(&msg, f.data, sizeof(msg));

        // clientID = msg.tx_id_high << 8 | msg.tx_id_low;
        // ecuID = msg.rx_id_high << 8 | msg.rx_id_low;

        ecuID = (f.data[3] & 0x0F) << 8 | f.data[2];
        clientID = (f.data[5] & 0x0F) << 8 | f.data[4];

        connected = 1; // Channel established

        Serial.print("ChanSetup RX:");
        PrintPacket(f);
        Serial.println(F("I] Channel established."));
        // DEBUG
        char buf[64];
        snprintf(buf, sizeof(buf),
                 "millis()=%lums, clientID=%03lx, ecuID=%03lx", millis(),
                 clientID, ecuID);
        Serial.println(buf);
        // DEBUG
        // char buf[64];
        // snprintf(buf, sizeof(buf), "OP=%x DEST=%x TXh=%x TXl=%x RXh=%x
        // RXl=%x",
        //          msg.op, msg.dest, msg.tx_id_high, msg.tx_id_low,
        //          msg.rx_id_high, msg.rx_id_low);
        // Serial.println(buf);
      }

      // timeout counter
      if (++counter > 20) {
        Serial.println(F("E] Channel setup timed out."));
        return;
      }
    }
  } while (connected < 1);

  // set timing
  setTiming();

  // setup filter for ECU TODO FIXME
  // CAN_Filter filter;
  // filter.id = ecuID;
  // CAN.setFilter(filter);

  // wait for message from ECU
  CAN_Frame f;
  counter = 0;
  do {
    delay(5);
    if (CAN.available()) {
      f = CAN.read();
      if (f.id == ecuID || f.id == clientID || f.id == 0x200 || f.id == 0x201)
        PrintPacket(f);
    }

    // timeout counter
    if (++counter > 200) {
      Serial.println(F("E] Timing setup timed out."));
      return;
    }
  } while (!(f.id == ecuID && f.length == 6 && f.data[1] == VWTP_TPDU_CONNACK));

  // a1 0f 8a ff 4a ff
  if (f.id == ecuID && f.length == 6 && f.data[1] == VWTP_TPDU_CONNACK) {
    connected = 2; // Timing established

    Serial.println(F("I] Timing established."));
    char buf[50];
    snprintf(buf, sizeof(buf), "millis()=%lums, T1=%02x, T3=%02x", millis(),
             f.data[2], f.data[4]);
    txTimeout = f.data[2];
    txMinTime = f.data[4];
    Serial.println(buf);

    // TODO Decode timing params to ms
  }
}

// int VWTP20::prepVwtpMsg(uint8_t *buf, tVWTP_MSG msg) {
//   // TODO FIXME BROKEN

//   // buf must be at least size 7
//   if (sizeof(buf) < 7)
//     return -1;

//   buf[0] = msg.dest;
//   buf[1] = msg.op;
//   buf[2] = msg.rx_id_low;
//   buf[3] = msg.rx_valid << 4 | msg.rx_id_high;
//   buf[4] = msg.tx_id_low;
//   buf[5] = msg.tx_valid << 4 | msg.tx_id_high;
//   buf[6] = msg.apptype;
//   return 1;
// }

void VWTP20::init() {
  CAN_Frame f;

  f.id = 0x200; // BCAST
  f.valid = true;
  f.rtr = 0;
  f.extended = CAN_STANDARD_FRAME;

  // 01  C0 00  1 0  00  0 3  01
  // dst op rl rv rh tl tv th app
  // tVWTP_MSG msg;
  // msg.dest = 0x01; // ECU
  // msg.op = VWTP_OP_CHAN_SETUP;
  // msg.rx_id_low = 0x00;
  // msg.rx_valid = 0x1;
  // msg.rx_id_high = 0x0;
  // msg.tx_id_low = 0x00;
  // msg.tx_valid = 0x0;
  // msg.tx_id_high = 0x3;
  // msg.apptype = VWTP_APP_TYPE_DIAG;

  f.length = 7;

  f.data[0] = 0x01;               // Dest
  f.data[1] = VWTP_OP_CHAN_SETUP; // Opcode
  f.data[2] = 0x00;               // RX_ID_Low
  f.data[3] = 0x1 << 4 | 0x0;     // RX_Valid << 4 | RX_ID_High
  f.data[4] = 0x00;               // TX_ID_Low
  f.data[5] = 0x0 << 4 | 0x3;     // TX_Valid << 4 | TX_ID_High
  f.data[6] = VWTP_APP_TYPE_DIAG; // Apptype

  // prepVwtpMsg(f.data, msg);
  // memcpy(f.data, &msg, sizeof(msg));

  Serial.print(F("ChanSetup TX:"));
  PrintPacket(f);

  CAN.write(f);

  // f = CAN.read();
  // PrintPacket(f);
}

// a0 0f 8a ff 32 ff
void VWTP20::setTiming() {
  CAN_Frame f;

  f.id = clientID;
  f.valid = true;
  f.rtr = 0;
  f.extended = CAN_STANDARD_FRAME;

  f.length = 6;

  f.data[0] = VWTP_TPDU_CONNSETUP;
  f.data[1] = 0x0F; // Block size (num packets before ACK) 0xF
  f.data[2] = 0x8A; // T1 tx timeout 100ms
  f.data[3] = 0xFF; // T2 unused
  f.data[4] = 0x32; // T3 min time for tx packet 5ms
  f.data[5] = 0xFF; // T4 unused

  Serial.print(F("Timing TX:"));
  PrintPacket(f);

  CAN.write(f);
}

// send chan_test message for keepalive
void VWTP20::channelTest() {
  CAN_Frame f;

  f.id = clientID;
  f.valid = true;
  f.rtr = 0;
  f.extended = CAN_STANDARD_FRAME;

  f.length = 1;

  f.data[0] = VWTP_TPDU_CONNTEST;

  CAN.write(f);
}

void VWTP20::PrintPacket(CAN_Frame m) {
  Serial.print(F("D] 0x"));
  Serial.print(m.id, HEX);
  Serial.print(F("  "));

  char mbuf[3] = {0};

  if (!m.rtr)
    for (byte i = 0; i < m.length; i++) {
      snprintf(mbuf, sizeof(mbuf), "%02x", m.data[i]);
      Serial.print(mbuf);
    }

  Serial.println();
}

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