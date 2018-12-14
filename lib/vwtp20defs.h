#include <stdint.h>
// Reference: SAE J2819 FEB2008
// TP2.0 Vehicle Diagnostic Protocol

// VWTP default data structure
typedef struct {
  uint8_t dest : 8;
  uint8_t op : 8;
  uint64_t param : 40;
} tVWTP;

typedef struct {
  uint8_t dest : 8;
  uint8_t op : 4;
  uint8_t seq : 4;
  uint64_t payload : 40;
} tVWTP_data;

// typedef struct {
//   uint8_t dest : 8;      // TP Target Address
//   uint8_t op : 8;        // Opcode
//   uint8_t tx_id_low : 8; // TX-ID-Low
//   uint8_t info_tx : 5;
//   uint8_t tx_id_high : 3;
//   uint8_t rx_id_low : 8;
//   uint8_t info_rx : 5;
//   uint8_t rx_id_high : 3;
//   uint8_t apptype : 8;

// } tVWTP_MSG;

//
// CAN Identifiers
//

// Identifier: Fixed address of the sending (active) ECU.
// Range between 0x200 through 0x2EF
#define VWTP_CAN_ID 0x200

// Destination: TP-target address of the recipient
// (lower 8 bits of the ECU’s assigned Identifier)
// Destination < 0xFx
#define VWTP_CAN_DEST 0x1

//
// Broadcast message
//

// Request 0x23
#define VWTP_OP_BCAST_REQ 0x23

// Response 0x24
#define VWTP_OP_BCAST_RESP 0x24

//
// Dynamic channel structure message
//

// Channel Set-up 0xC0
#define VWTP_OP_CHAN_SETUP 0xC0

// Channel Ack-Positive Reply 0xD0
#define VWTP_OP_CHAN_ACK_POS 0xD0

// Channel Ack-Negative Reply 0xD6: Application type not supported
#define VWTP_OP_CHAN_ACK_NEG_APP_NOT_SUPPORTED 0xD6

// Channel Ack-Negative Reply 0xD7: Application type temporarily not supported
#define VWTP_OP_CHAN_ACK_NEG_TEMP_APP_NOT_SUPPORTED 0xD7

// Channel Ack-Negative Reply 0xD8: Temporarily no resources are free
#define VWTP_OP_CHAN_ACK_NEG_TEMP_NO_RESOURCES 0xD8

//
// Transport Protocol Data Unit (TPDU)
//

// Connection set-up
#define VWTP_TPDU_CS 0xA0

// Connection ack
#define VWTP_TPDU_CA 0xA1

// Connection test
#define VWTP_TPDU_CT 0xA3

// Break
#define VWTP_TPDU_BR 0xA4

// Disconnect
#define VWTP_TPDU_DC 0xA8

//
// Application-Type
//

// SD Diagnostics
#define VWTP_APP_TYPE_DIAG 0x1

// Infotainment communication
#define VWTP_APP_TYPE_INFOTAINMENT 0x10

// Application Protocol
#define VWTP_APP_TYPE_APPLICATION 0x20

// WFS/WIV
#define VWTP_APP_TYPE_WFS 0x21