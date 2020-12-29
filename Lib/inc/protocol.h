#ifndef __PROTOCOL_H
#define __PROTOCOL_H

struct Proto_Packet_TypeDef {
  uint8_t packetNum;
  uint16_t power;
  uint16_t voltage;
};

#endif