#include "vwtp20.h"
#include "config.h"
#include "vwtp20defs.h"
#include <Arduino.h>
#include <canwrapper.h>

////////////////////////////////////////////////////////////////////////
VWTP20::VWTP20() {
  sequence = 0;
  clientID = 0x000; // rx
  ecuID = 0x000;    // tx
  connected = NotConnected;
  txTimeoutMs = 0.0; // T1
  txMinTimeMs = 0.0; // T3
  // isLastPacket;
}

////////////////////////////////////////////////////////////////////////
// Getters
uint8_t VWTP20::GetSequence() { return sequence; }
uint32_t VWTP20::GetClientID() { return clientID; }
uint32_t VWTP20::GetEcuID() { return ecuID; }
CONNECTION VWTP20::GetConnected() { return connected; }
void VWTP20::SetConnected(CONNECTION c) { connected = c; }
float VWTP20::GetTxTimeoutMs() { return txTimeoutMs; }
float VWTP20::GetTxMinTimeMs() { return txMinTimeMs; }

////////////////////////////////////////////////////////////////////////
void VWTP20::Connect() {
  // TODO
  // 1 Setup KWP2000
  // 2 Dump ram
  //   - REF: http://nefariousmotorsports.com/forum/index.php?topic=9182.5
  //   - REF: https://github.com/seishuku/teensycanbusdisplay
  // send chan_test messages for keepalive

  uint8_t counter = 0;

  Serial.println(F("E] Channel setup ..."));
  delay(100);

  // setup channel
  do {
    delay(50);
    channelInit();
    if (isCANAvail()) {
      tCanFrame f;
      CANReadMsg(&f);
      // 00 d0 00 03 40 07 01
      if (f.id == 0x201 && f.length == 7 && f.data[1] == VWTP_OP_CHAN_ACK_POS) {
        ecuID = (f.data[3] & 0x0F) << 8 | f.data[2];
        clientID = (f.data[5] & 0x0F) << 8 | f.data[4];

        connected = Connected;
        break; // goto timing
      }

      // timeout counter
      if (++counter > 20) {
        Serial.println(F("E] Channel setup timed out."));
        connected = NotConnected;
        return;
      }
    }
  } while (connected < Connected);

  // set timing
  setTiming();
  if (connected < ConnectedWithTiming) {
    connected = ConnectionTimingError;
    Serial.println(F("E] Channel setup timing error."));
    return; // failure
  }
}

////////////////////////////////////////////////////////////////////////
void VWTP20::Disconnect() {
  if (connected < ConnectedWithTiming)
    return;

  tCanFrame f;
  f.id = clientID;
  f.length = 1;
  f.data[0] = VWTP_TPDU_DISCONN;

  CANSendMsg(f);
}

////////////////////////////////////////////////////////////////////////
// init channel for Connect()
void VWTP20::channelInit() {
  tCanFrame f;
  f.id = 0x200; // BCAST
  f.length = 7;

  // 01  C0 00  1 0  00  0 3  01
  // dst op rl rv rh tl tv th app
  f.data[0] = 0x01;               // Dest (ECU)
  f.data[1] = VWTP_OP_CHAN_SETUP; // Opcode
  f.data[2] = 0x00;               // RX_ID_Low
  f.data[3] = 0x1 << 4 | 0x0;     // RX_Valid << 4 | RX_ID_High
  f.data[4] = 0x00;               // TX_ID_Low
  f.data[5] = 0x0 << 4 | 0x3;     // TX_Valid << 4 | TX_ID_High
  f.data[6] = VWTP_APP_TYPE_DIAG; // Apptype

  CANSendMsg(f);
}

////////////////////////////////////////////////////////////////////////
// ESTABLISH TIMING for Connect()
void VWTP20::setTiming() {
  tCanFrame f;
  f.id = clientID;
  f.length = 6;

  // a0 0f 8a ff 32 ff
  f.data[0] = VWTP_TPDU_CONNSETUP;
  f.data[1] = 0x0F; // Block size (num packets before ACK) 0xF
  f.data[2] = 0x8A; // T1 tx timeout 100ms
  f.data[3] = 0xFF; // T2 unused
  f.data[4] = 0x32; // T3 min time for tx packet 5ms
  f.data[5] = 0xFF; // T4 unused

  CANSendMsg(f);

  // wait for response from ecu
  // NOTE: Do not use serial print statements here, unless failure condition.
  tCanFrame resp;
  uint16_t counter = 0;
  do {
    // for counter timing
    delayMicroseconds(10);

    if (isCANAvail()) {
      CANReadMsg(&resp);
    }
    if (++counter >= 10000) {
      Serial.print(millis());
      Serial.println(F(" E] Timed out while awaiting a timing response."));
      connected = ConnectionTimingError;
      return;
    }
  } while (resp.id != ecuID);

  // decode tx timeout and mintime
  if (resp.length == 6 && resp.data[0] == VWTP_TPDU_CONNACK) {
    txTimeoutMs = decodeTimingMs(resp.data[2]); // T1
    txMinTimeMs = decodeTimingMs(resp.data[4]); // T3
    connected = ConnectedWithTiming;
  } else {
    connected = ConnectionTimingError;
    return;
  }
}

////////////////////////////////////////////////////////////////////////
// decode timing from frame
float VWTP20::decodeTimingMs(uint8_t byt) {
  uint8_t bunit = (byt >> 6) & 0b11;
  uint8_t scale = byt & 0b00111111;
  float unit = 0.0;
  switch (bunit) {
  case 0x0:
    unit = 0.1;
    break;
  case 0x1:
    unit = 1;
    break;
  case 0x2:
    unit = 10;
    break;
  case 0x3:
    unit = 100;
    break;
  }

  return unit * scale;
}

////////////////////////////////////////////////////////////////////////
// convert ms to microseconds
unsigned int VWTP20::MsToMicros(float ms) { return (unsigned int)(ms * 1000); }

/*
// TODO DELETE send chan_test message for keepalive
bool VWTP20::ChannelTest() {
  if (connected < ConnectedWithTiming) {
    connected = ConnectionTestError;
    return false; // not connected
  }

  tCanFrame f;
  f.id = clientID;
  f.length = 1;

  f.data[0] = VWTP_TPDU_CONNTEST;

  // check ecu resp
  tCanFrame resp = AwaitECUResponse(f);

  if (resp.length == 6 && resp.data[0] == VWTP_TPDU_CONNACK) {
    connected = ConnectedWithTiming;
    return true;
  }

  connected = ConnectionTestError;
  return false;
}

// TODO DELETE await ecu response
tCanFrame VWTP20::AwaitECUResponse() {
  unsigned int ctimeout = MsToMicros(txTimeoutMs) / 10;

  tCanFrame resp;
  uint16_t counter = 0;
  do {
    // for counter timing
    delayMicroseconds(10);

    if (isCANAvail()) {
      CANReadMsg(&resp);
    }
    if (++counter >= ctimeout) {
      Serial.print(millis());
      Serial.println(F(" E] Timed out while awaiting a response."));
      connected = ConnectionResponseError;
      break;
    }
  } while (resp.id != ecuID);

  return resp;
}

// TODO DELETE send can frame and return ecu response
tCanFrame VWTP20::AwaitECUResponse(tCanFrame f) {
  CANSendMsg(f);
  return AwaitECUResponse();
}

// TODO DELETE
tCanFrame VWTP20::AwaitECUResponseCmd(tCanFrame f, uint8_t cmd) {
  CANSendMsg(f);
  return AwaitECUResponseCmd(cmd);
}

// TODO DELETE
tCanFrame VWTP20::AwaitECUResponseCmd(uint8_t cmd) {
  unsigned int ctimeout = MsToMicros(txTimeoutMs) / 10;

  tCanFrame resp;
  uint16_t counter = 0;
  do {
    // for counter timing
    delayMicroseconds(10);

    if (isCANAvail()) {
      CANReadMsg(&resp);
    }
    if (++counter >= ctimeout) {
      Serial.print(millis());
      Serial.println(F(" E] Timed out while awaiting a response."));
      connected = ConnectionResponseError;
      break;
    }
  } while (resp.id != ecuID && resp.data[0] != cmd);

  return resp;
}
*/

////////////////////////////////////////////////////////////////////////
// DEPRECIATED: Print CAN packet
void VWTP20::PrintPacket(tCanFrame f) { CANPacketPrint(F("D]"), f); }

////////////////////////////////////////////////////////////////////////
void VWTP20::PrintPacketMs(tCanFrame f) {
  char buf[16];
  sprintf(buf, "%06lu D]", millis());
  CANPacketPrint(buf, f);
}

////////////////////////////////////////////////////////////////////////
// Print tCanFrame to char[58]
void VWTP20::PacketToChar(tCanFrame f, char *out) {
  sprintf(out, "0x%03lx  ", f.id); // ~8 chars

  char mbuf[3];

  for (byte i = 0; i < f.length; i++) {
    snprintf(mbuf, sizeof(mbuf), "%02x", f.data[i]); // 3*8*2 = 48 chars
    strcat(out, mbuf);
  }

  // + '\0' = 57 chars
}

////////////////////////////////////////////////////////////////////////
// Check data ack
bool VWTP20::CheckDataACK(tCanFrame *f) {
  uint8_t op = f->data[0] >> 8;
  sequence = (f->data[0] & 0xF);

  if (op == 0x0 || op == 0x1)
    return true; // tx ack op is 0xB if ready or 0x9 if not ready
  else if (op == 0x2 || op == 0x3)
    return false;

  // default case
  return false;
}

////////////////////////////////////////////////////////////////////////
// prepare response data ack
void VWTP20::PrepareDataACKResponse(bool readyForNextPacket, tCanFrame *f) {
  uint8_t op;
  if (readyForNextPacket)
    op = 0xB;
  else
    op = 0x9;

  NextSequence();

  f->id = clientID;
  f->length = 1;
  f->data[0] = (op << 8 | sequence);
}

////////////////////////////////////////////////////////////////////////
uint8_t VWTP20::NextSequence() {
  if (++sequence > 0xF)
    sequence = 0;

  return sequence;
}

////////////////////////////////////////////////////////////////////////
uint8_t VWTP20::ResetSequence() {
  sequence = 0;
  return sequence;
}
