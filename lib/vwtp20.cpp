#include "vwtp20.h"
#include "vwtp20defs.h"

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