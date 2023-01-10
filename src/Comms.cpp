#include "Comms.h"

#define COMMS_SPI_FREQUENCY 1000000
#define COMMS_SPI_SETTINGS SPISettings(1000000, MSBFIRST, SPI_MODE0)
#define COMMS_SCK_PIN 12
#define COMMS_MISO_PIN 13
#define COMMS_MOSI_PIN 11
#define COMMS_CS_PIN 10
// #define COMMS_ETHERNET_LARGE_BUFFERS
// #define COMMS_SPI_HAS_TRANSFER_BUF
// #define COMMS_MAX_SOCK_NUM 8

namespace Comms {
  SPIClass hspi(HSPI);
  socketstate_t state[8]; // max 8 sockets

  uint8_t softReset(void)
  {
    uint16_t count=0;

    // write to reset bit
    writeMR(0x80);
    // then wait for soft reset to complete
    do {
      uint8_t mr = readMR();
      if (mr == 0) return 1;
      delay(1);
    } while (++count < 20);
    return 0;
  }
  uint8_t getVersion() {
    uint8_t ver = readVERSIONR();
    return ver;
  }
  W5500Linkstatus getLinkStatus()
  {
    uint8_t phystatus;
  
    phystatus = readPHYCFGR();
    if (phystatus & 0x01) return LINK_ON;
    return LINK_OFF;
  }



  uint8_t socketBegin(uint8_t protocol, uint16_t port, uint8_t socket)
  {
    hspi.beginTransaction(COMMS_SPI_SETTINGS);
    writeSnMR(socket, protocol);
    writeSnIR(socket, 0xFF);
    writeSnPORT(socket, port);
    execCmdSn(socket, Sock_OPEN);
    state[socket].RX_RSR = 0;
    state[socket].RX_RD  = readSnRX_RD(socket); // always zero?
    state[socket].RX_inc = 0;
    state[socket].TX_FSR = 0;
    hspi.endTransaction();
    return socket;
  }

  uint8_t init(uint8_t ip_end) {
    // Initialize SPI
    delay(560); // was in initial ethernet code (not necessary?)
    hspi.begin(COMMS_SCK_PIN, COMMS_MISO_PIN, COMMS_MOSI_PIN, COMMS_CS_PIN);
    hspi.setHwCs(false);
    hspi.setFrequency(1000000);
    hspi.setBitOrder(MSBFIRST);
    hspi.setDataMode(SPI_MODE0);
    pinMode(COMMS_CS_PIN, OUTPUT);
    digitalWrite(COMMS_CS_PIN, HIGH);

    // reset chip
    if(!softReset()) return 0;
    if(getVersion() != 4) return 0;

    const uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x05};
    const uint8_t ipa[] = {192, 168, 0, ip_end};
    const uint8_t gateip[] = {192, 168, 0, 1};
    const uint8_t subnet[] = {255, 255, 255, 0};

    writeSHAR(mac); // mac address
    writeSIPR(ipa);
    writeGAR(gateip);
    writeSUBR(subnet);

    socketBegin(IPPROTO::UDP, 42069, 0);

    return 1;
  }

  void setReceiverIPandPort(uint8_t end, uint16_t port) {
    uint8_t addr[] = {192, 168, 0, end};
    hspi.beginTransaction(COMMS_SPI_SETTINGS);
    writeSnDIPR(0, addr);
	  writeSnDPORT(0, port);
    hspi.endTransaction();
  }

  void write_data(uint8_t s, uint16_t data_offset, const uint8_t *data, uint16_t len)
  {
    uint16_t ptr = readSnTX_WR(s);
    ptr += data_offset;
    uint16_t offset = ptr & SMASK;
    uint16_t dstAddr = offset + SBASE(s);

    write(dstAddr, data, len);
    ptr += len;
    writeSnTX_WR(s, ptr);
  }

  void sendRawPacket(uint8_t *data, uint16_t len) {
    hspi.beginTransaction(COMMS_SPI_SETTINGS);
    write_data(0, 0, data, len);
	  execCmdSn(0, Sock_SEND);
    // writeSnIR(0, SnIR::SEND_OK);
    hspi.endTransaction();
  }

  uint16_t read(uint16_t addr, uint8_t *buf, uint16_t len)
  {
    uint8_t cmd[4];
    digitalWrite(COMMS_CS_PIN, LOW);
    if (addr < 0x100) {
      // common registers 00nn
      cmd[0] = 0;
      cmd[1] = addr & 0xFF;
      cmd[2] = 0x00;
    } else if (addr < 0x8000) {
      // socket registers  10nn, 11nn, 12nn, 13nn, etc
      cmd[0] = 0;
      cmd[1] = addr & 0xFF;
      cmd[2] = ((addr >> 3) & 0xE0) | 0x08;
    } else if (addr < 0xC000) {
      // transmit buffers  8000-87FF, 8800-8FFF, 9000-97FF, etc
      //  10## #nnn nnnn nnnn
      cmd[0] = addr >> 8;
      cmd[1] = addr & 0xFF;
      // #if defined(ETHERNET_LARGE_BUFFERS) && MAX_SOCK_NUM <= 1
      // cmd[2] = 0x10;                       // 16K buffers
      // #elif defined(ETHERNET_LARGE_BUFFERS) && MAX_SOCK_NUM <= 2
      // cmd[2] = ((addr >> 8) & 0x20) | 0x10; // 8K buffers
      // #elif defined(ETHERNET_LARGE_BUFFERS) && MAX_SOCK_NUM <= 4
      // cmd[2] = ((addr >> 7) & 0x60) | 0x10; // 4K buffers
      // #else
      cmd[2] = ((addr >> 6) & 0xE0) | 0x10; // 2K buffers
      // #endif
    } else {
      // receive buffers
      cmd[0] = addr >> 8;
      cmd[1] = addr & 0xFF;
      // #if defined(ETHERNET_LARGE_BUFFERS) && MAX_SOCK_NUM <= 1
      // cmd[2] = 0x18;                       // 16K buffers
      // #elif defined(ETHERNET_LARGE_BUFFERS) && MAX_SOCK_NUM <= 2
      // cmd[2] = ((addr >> 8) & 0x20) | 0x18; // 8K buffers
      // #elif defined(ETHERNET_LARGE_BUFFERS) && MAX_SOCK_NUM <= 4
      // cmd[2] = ((addr >> 7) & 0x60) | 0x18; // 4K buffers
      // #else
      cmd[2] = ((addr >> 6) & 0xE0) | 0x18; // 2K buffers
      // #endif
    }
    hspi.transfer(cmd, 3);
    memset(buf, 0, len);
    hspi.transfer(buf, len);
    digitalWrite(COMMS_CS_PIN, HIGH);
    return len;
  }
  uint16_t write(uint16_t addr, const uint8_t *buf, uint16_t len)
  {
    uint8_t cmd[8];

    digitalWrite(COMMS_CS_PIN, LOW);
    if (addr < 0x100) {
      // common registers 00nn
      cmd[0] = 0;
      cmd[1] = addr & 0xFF;
      cmd[2] = 0x04;
    } else if (addr < 0x8000) {
      // socket registers  10nn, 11nn, 12nn, 13nn, etc
      cmd[0] = 0;
      cmd[1] = addr & 0xFF;
      cmd[2] = ((addr >> 3) & 0xE0) | 0x0C;
    } else if (addr < 0xC000) {
      // transmit buffers  8000-87FF, 8800-8FFF, 9000-97FF, etc
      //  10## #nnn nnnn nnnn
      cmd[0] = addr >> 8;
      cmd[1] = addr & 0xFF;
      // #if defined(ETHERNET_LARGE_BUFFERS) && MAX_SOCK_NUM <= 1
      // cmd[2] = 0x14;                       // 16K buffers
      // #elif defined(ETHERNET_LARGE_BUFFERS) && MAX_SOCK_NUM <= 2
      // cmd[2] = ((addr >> 8) & 0x20) | 0x14; // 8K buffers
      // #elif defined(ETHERNET_LARGE_BUFFERS) && MAX_SOCK_NUM <= 4
      // cmd[2] = ((addr >> 7) & 0x60) | 0x14; // 4K buffers
      // #else
      cmd[2] = ((addr >> 6) & 0xE0) | 0x14; // 2K buffers
      // #endif
    } else {
      // receive buffers
      cmd[0] = addr >> 8;
      cmd[1] = addr & 0xFF;
      // #if defined(ETHERNET_LARGE_BUFFERS) && COMMS_MAX_SOCK_NUM <= 1
      // cmd[2] = 0x1C;                       // 16K buffers
      // #elif defined(ETHERNET_LARGE_BUFFERS) && COMMS_MAX_SOCK_NUM <= 2
      // cmd[2] = ((addr >> 8) & 0x20) | 0x1C; // 8K buffers
      // #elif defined(ETHERNET_LARGE_BUFFERS) && COMMS_MAX_SOCK_NUM <= 4
      // cmd[2] = ((addr >> 7) & 0x60) | 0x1C; // 4K buffers
      // #else
      cmd[2] = ((addr >> 6) & 0xE0) | 0x1C; // 2K buffers
      // #endif
    }

    if (len <= 5) {
      for (uint8_t i=0; i < len; i++) {
        cmd[i + 3] = buf[i];
      }
      hspi.transfer(cmd, len + 3);
    } else {
      hspi.transfer(cmd, 3);
      hspi.transferBytes(buf, NULL, len);
    }
    digitalWrite(COMMS_CS_PIN, HIGH);

    return len;
  }
  uint8_t read(uint16_t addr) {
    uint8_t data;
    read(addr, &data, 1);
    return data;
  }
  uint8_t write(uint16_t addr, uint8_t data) {
    return write(addr, &data, 1);
  }
  uint8_t readSn(SOCKET s, uint16_t addr) {
    return read(CH_BASE + s * CH_SIZE + addr);
  }
  uint8_t writeSn(SOCKET s, uint16_t addr, uint8_t data) {
    return write(CH_BASE + s * CH_SIZE + addr, data);
  }
  uint16_t readSn(SOCKET s, uint16_t addr, uint8_t *buf, uint16_t len) {
    return read(CH_BASE + s * CH_SIZE + addr, buf, len);
  }
  uint16_t writeSn(SOCKET s, uint16_t addr, uint8_t *buf, uint16_t len) {
    return write(CH_BASE + s * CH_SIZE + addr, buf, len);
  }
  void execCmdSn(SOCKET s, SockCMD _cmd)
  {
    // Send command to socket
    writeSnCR(s, _cmd);
    // Wait for command to complete
    while (readSnCR(s));
  }
};