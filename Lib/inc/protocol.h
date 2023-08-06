#ifndef __PROTOCOL_H
#define __PROTOCOL_H

struct InputBits {
  uint8_t input0:1;
	uint8_t input1:1;
	uint8_t input2:1;
	uint8_t input3:1;
	uint8_t reserved;
};

union State {
	struct InputBits io;
	uint16_t state;
};

struct Proto_Packet_TypeDef {
  uint8_t packetNum;
  union {
	  struct InputBits io;
  	uint16_t state;
  };
  uint16_t voltage;
};

#endif