#include <Arduino.h>
#include <SPI.h>

namespace Comms {
  typedef uint8_t SOCKET;

  class SnMR {
  public:
    static const uint8_t CLOSE  = 0x00;
    static const uint8_t TCP    = 0x21;
    static const uint8_t UDP    = 0x02;
    static const uint8_t IPRAW  = 0x03;
    static const uint8_t MACRAW = 0x04;
    static const uint8_t PPPOE  = 0x05;
    static const uint8_t ND     = 0x20;
    static const uint8_t MULTI  = 0x80;
  };

  enum SockCMD {
    Sock_OPEN      = 0x01,
    Sock_LISTEN    = 0x02,
    Sock_CONNECT   = 0x04,
    Sock_DISCON    = 0x08,
    Sock_CLOSE     = 0x10,
    Sock_SEND      = 0x20,
    Sock_SEND_MAC  = 0x21,
    Sock_SEND_KEEP = 0x22,
    Sock_RECV      = 0x40
  };

  class SnIR {
  public:
    static const uint8_t SEND_OK = 0x10;
    static const uint8_t TIMEOUT = 0x08;
    static const uint8_t RECV    = 0x04;
    static const uint8_t DISCON  = 0x02;
    static const uint8_t CON     = 0x01;
  };

  class SnSR {
  public:
    static const uint8_t CLOSED      = 0x00;
    static const uint8_t INIT        = 0x13;
    static const uint8_t LISTEN      = 0x14;
    static const uint8_t SYNSENT     = 0x15;
    static const uint8_t SYNRECV     = 0x16;
    static const uint8_t ESTABLISHED = 0x17;
    static const uint8_t FIN_WAIT    = 0x18;
    static const uint8_t CLOSING     = 0x1A;
    static const uint8_t TIME_WAIT   = 0x1B;
    static const uint8_t CLOSE_WAIT  = 0x1C;
    static const uint8_t LAST_ACK    = 0x1D;
    static const uint8_t UDP         = 0x22;
    static const uint8_t IPRAW       = 0x32;
    static const uint8_t MACRAW      = 0x42;
    static const uint8_t PPPOE       = 0x5F;
  };

  class IPPROTO {
  public:
    static const uint8_t IP   = 0;
    static const uint8_t ICMP = 1;
    static const uint8_t IGMP = 2;
    static const uint8_t GGP  = 3;
    static const uint8_t TCP  = 6;
    static const uint8_t PUP  = 12;
    static const uint8_t UDP  = 17;
    static const uint8_t IDP  = 22;
    static const uint8_t ND   = 77;
    static const uint8_t RAW  = 255;
  };

  enum W5500Linkstatus {
    UNKNOWN,
    LINK_ON,
    LINK_OFF
  };

  typedef struct {
    uint16_t RX_RSR; // Number of bytes received
    uint16_t RX_RD;  // Address to read
    uint16_t TX_FSR; // Free space ready for transmit
    uint8_t  RX_inc; // how much have we advanced RX_RD
  } socketstate_t;

  const uint16_t CH_BASE = 0x1000;
  const uint16_t CH_SIZE = 0x0100;
  const uint16_t SSIZE = 2048;
  const uint16_t SMASK = 0x07FF;

  uint8_t init(uint8_t ip_end);
  void setReceiverIPandPort(uint8_t end, uint16_t port);
  void sendRawPacket(uint8_t *data, uint16_t len);

  uint8_t softReset();
  W5500Linkstatus getLinkStatus();

  void setMACAddress(const uint8_t *mac);
  void setIPAddress(const uint8_t *ip);
  void setGatewayIp(const uint8_t *ip);
	void setSubnetMask(const uint8_t *mask);
  void setRetransmissionCount(uint8_t count);
  void setRetransmissionTimeout(uint16_t milliseconds);

  W5500Linkstatus getLinkStatus();

  uint16_t read(uint16_t addr, uint8_t *buf, uint16_t len);
  uint16_t write(uint16_t addr, const uint8_t *buf, uint16_t len);
  uint8_t read(uint16_t addr);
  uint8_t write(uint16_t addr, uint8_t data);
  uint8_t readSn(SOCKET s, uint16_t addr);
  uint8_t writeSn(SOCKET s, uint16_t addr, uint8_t data);
  uint16_t readSn(SOCKET s, uint16_t addr, uint8_t *buf, uint16_t len);
  uint16_t writeSn(SOCKET s, uint16_t addr, uint8_t *buf, uint16_t len);
  void execCmdSn(SOCKET s, SockCMD _cmd);
  static uint16_t SBASE(uint8_t socknum) {
    return socknum * SSIZE + 0x8000;
  }

  /* --- General Purpose Register Definitions --- */

  #define __GP_REGISTER8(name, address)             \
    static inline void write##name(uint8_t _data) { \
      write(address, _data);                        \
    }                                               \
    static inline uint8_t read##name() {            \
      return read(address);                         \
    }
  #define __GP_REGISTER16(name, address)            \
    static void write##name(uint16_t _data) {       \
      uint8_t buf[2];                               \
      buf[0] = _data >> 8;                          \
      buf[1] = _data & 0xFF;                        \
      write(address, buf, 2);                       \
    }                                               \
    static uint16_t read##name() {                  \
      uint8_t buf[2];                               \
      read(address, buf, 2);                        \
      return (buf[0] << 8) | buf[1];                \
    }
  #define __GP_REGISTER_N(name, address, size)      \
    static uint16_t write##name(const uint8_t *_buff) {   \
      return write(address, _buff, size);           \
    }                                               \
    static uint16_t read##name(uint8_t *_buff) {    \
      return read(address, _buff, size);            \
    }

  __GP_REGISTER8 (MR,     0x0000);    // Mode
  __GP_REGISTER_N(GAR,    0x0001, 4); // Gateway IP address
  __GP_REGISTER_N(SUBR,   0x0005, 4); // Subnet mask address
  __GP_REGISTER_N(SHAR,   0x0009, 6); // Source MAC address
  __GP_REGISTER_N(SIPR,   0x000F, 4); // Source IP address
  __GP_REGISTER16(INTLEVEL,0x0013);   // Interrupt low level timer
  __GP_REGISTER8 (IR,     0x0015);    // Interrupt
  __GP_REGISTER8 (IMR,    0x0016);    // Interrupt Mask
  __GP_REGISTER8 (SIR,    0x0017);    // Socket interrupt
  __GP_REGISTER8 (SIMR,   0x0018);    // Socket interrupt mask
  __GP_REGISTER16(RTR,    0x0019);    // Retry timeout
  __GP_REGISTER8 (RCR,    0x001B);    // Retry count
  __GP_REGISTER8 (PTIMER, 0x001C);    // PPP LCP Request Timer
  __GP_REGISTER8 (PMAGIC, 0x001D);    // PPP LCP Magic Number
  __GP_REGISTER_N(PHAR,   0x001E, 6); // PPP Destination MAC address
  __GP_REGISTER16(PSID,    0x0024);   // PPP Session Identification
  __GP_REGISTER16(PMRU,    0x0026);   // PPP Maximum Segment Size
  __GP_REGISTER_N(UIPR,    0x0028, 4); // Unreachable IP Address
  __GP_REGISTER16(UPORTR,  0x002C);   // PPP Maximum Segment Size
  __GP_REGISTER8 (PHYCFGR, 0x002E);    // PHY Configuration register, default: 10111xxx
  __GP_REGISTER8 (VERSIONR,0x0039);   // Chip Version Register

  #undef __GP_REGISTER8
  #undef __GP_REGISTER16
  #undef __GP_REGISTER_N

  /* --- END General Purpose Register Definitions --- */

  /* --- Socket Register Definitions --- */

  #define __SOCKET_REGISTER8(name, address)                    \
    static inline void write##name(SOCKET _s, uint8_t _data) { \
      writeSn(_s, address, _data);                             \
    }                                                          \
    static inline uint8_t read##name(SOCKET _s) {              \
      return readSn(_s, address);                              \
    }
  #define __SOCKET_REGISTER16(name, address)                   \
    static void write##name(SOCKET _s, uint16_t _data) {       \
      uint8_t buf[2];                                          \
      buf[0] = _data >> 8;                                     \
      buf[1] = _data & 0xFF;                                   \
      writeSn(_s, address, buf, 2);                            \
    }                                                          \
    static uint16_t read##name(SOCKET _s) {                    \
      uint8_t buf[2];                                          \
      readSn(_s, address, buf, 2);                             \
      return (buf[0] << 8) | buf[1];                           \
    }
  #define __SOCKET_REGISTER_N(name, address, size)             \
    static uint16_t write##name(SOCKET _s, uint8_t *_buff) {   \
      return writeSn(_s, address, _buff, size);                \
    }                                                          \
    static uint16_t read##name(SOCKET _s, uint8_t *_buff) {    \
      return readSn(_s, address, _buff, size);                 \
    }

  __SOCKET_REGISTER8(SnMR,        0x0000)        // Mode
  __SOCKET_REGISTER8(SnCR,        0x0001)        // Command
  __SOCKET_REGISTER8(SnIR,        0x0002)        // Interrupt
  __SOCKET_REGISTER8(SnSR,        0x0003)        // Status
  __SOCKET_REGISTER16(SnPORT,     0x0004)        // Source Port
  __SOCKET_REGISTER_N(SnDHAR,     0x0006, 6)     // Destination Hardw Addr
  __SOCKET_REGISTER_N(SnDIPR,     0x000C, 4)     // Destination IP Addr
  __SOCKET_REGISTER16(SnDPORT,    0x0010)        // Destination Port
  __SOCKET_REGISTER16(SnMSSR,     0x0012)        // Max Segment Size
  __SOCKET_REGISTER8(SnTOS,       0x0015)        // IP TOS
  __SOCKET_REGISTER8(SnTTL,       0x0016)        // IP TTL
  __SOCKET_REGISTER8(SnRX_SIZE,   0x001E)        // RX Memory Size (W5200 only)
  __SOCKET_REGISTER8(SnTX_SIZE,   0x001F)        // RX Memory Size (W5200 only)
  __SOCKET_REGISTER16(SnTX_FSR,   0x0020)        // TX Free Size
  __SOCKET_REGISTER16(SnTX_RD,    0x0022)        // TX Read Pointer
  __SOCKET_REGISTER16(SnTX_WR,    0x0024)        // TX Write Pointer
  __SOCKET_REGISTER16(SnRX_RSR,   0x0026)        // RX Free Size
  __SOCKET_REGISTER16(SnRX_RD,    0x0028)        // RX Read Pointer
  __SOCKET_REGISTER16(SnRX_WR,    0x002A)        // RX Write Pointer (supported?)
  __SOCKET_REGISTER8(SnIMR,       0x002C)        // Socket Interrupt Mask
  __SOCKET_REGISTER16(SnFRAG,     0x002D)         // Socket fragment offset in IP header
  __SOCKET_REGISTER8(SnKPALVTR,   0x002F)        // Socket keep alive timer
  

  #undef __SOCKET_REGISTER8
  #undef __SOCKET_REGISTER16
  #undef __SOCKET_REGISTER_N

  /* --- END Socket Register Definitions --- */
};
