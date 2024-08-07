#ifndef RNMACROS_H
#define RNMACROS_H

// Frame offset
constexpr int OPCODE_OFFSET      = 0;   // 1 byte para opcode
constexpr int DEST_OFFSET        = 1;   // 1 byte para dest node
constexpr int SRC_OFFSET         = 2;   // 1 byte para src node
constexpr int DATA_OFFSET        = 3;   // 506 bytes + 1 byte de null terminator
constexpr int FRAME_STATE_OFFSET = 510; // 1 byte para frame state
constexpr int CRC_OFFSET         = 511; // 1 byte para CRC

// Operation codes for function mapping
constexpr char OPC_MSG       = 1;
constexpr char OPC_COLL_MSG  = 2;
constexpr char OPC_BROADCAST = 3;
constexpr char OPC_RESEND    = 6;
constexpr char OPC_NOP       = 7;
constexpr char OPC_RECEIPT   = 8;

// Other stuff
constexpr int  BASE_PORT      = 5000;
constexpr int  BUFFER_SIZE    = 512;
constexpr int  NODE_QTT       = 4;
constexpr int  DATA_SIZE      = 506;
constexpr int  CONN_SUCCESS   = 1;
constexpr int  CONN_FAIL      = 0;
constexpr int  FRAME_RECEIVED = 1;
constexpr int  FRAME_RESET    = 0;
constexpr char RING_LOCK[]   = "0123";

// Error codes for handle_error ()
constexpr int ERR_SCKFD_DEF       = -2;
constexpr int ERR_SCKFD_BIND      = -3;
constexpr int ERR_INVALID_ADDRESS = -5;
constexpr int ERR_GET_IFADDR      = -6;
constexpr int ERR_GET_NAMEINFO     = -7;
constexpr int ERR_INVALID_OPCODE  = -8;
constexpr int ERR_CRC_FAIL        = -9;
constexpr int ERR_UNKNOWN         = -10;

#endif // RNMACROS_H
