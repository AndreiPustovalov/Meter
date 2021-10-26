/* MAIN.C file
 * 
 * Copyright (c) 2002-2005 STMicroelectronics
 */

#include "uart.h"
#include "nrf24.h"
#include "protocol.h"
#include "delay.h"
#include "stm8l15x_clk.h"
#include "stm8l15x_rtc.h"
#include "stm8l15x_syscfg.h"

struct Proto_Packet_TypeDef rxPacket;
uint32_t totalPower = 0;

#define TX_SIZE 22
char tx_data[TX_SIZE];

void fail(void);
uint8_t fillBuffer(uint32_t power, uint16_t battery, uint8_t packet);

void main()
{
  uint8_t size, lastNum = 255;
  uint16_t lastPower = 0;
  CLK_SYSCLKDivConfig(CLK_SYSCLKDiv_1); // 16 MHz clock
  
  SYSCFG_REMAPPinConfig(REMAP_Pin_USART1TxRxPortA, ENABLE);
  
  UART_Init();

  nRF24_Init();
  
  tx_data[TX_SIZE - 2] = '\r';
  tx_data[TX_SIZE - 1] = '\n';
  
  if (!nRF24_Check()) {
    fail();
  }
  nRF24_RXMode(
    nRF24_RX_PIPE0, 
    nRF24_ENAA_P0, 
    100, 
    nRF24_DataRate_1Mbps, 
    nRF24_CRC_2byte, 
    "pwmtr", 
    5, 
    sizeof(rxPacket),
    nRF24_TXPower_0dBm
  );
  
  enableInterrupts();
  UART_Send("Hello!\r\n", 8);
  while (1) {
    if (nRF24_Wait_IRQ(0)) {
      if (nRF24_RXPacket(&rxPacket, sizeof(rxPacket)) != nRF24_RX_PCKT_PIPE0) {
        fail();
      }
      if (lastNum == (uint8_t)(rxPacket.packetNum - 1)) {
        totalPower += rxPacket.power;
      } else if (lastNum == rxPacket.packetNum) {
        totalPower = totalPower - lastPower + rxPacket.power;
      }
      lastPower = rxPacket.power;
      lastNum = rxPacket.packetNum;
      size = fillBuffer(totalPower, rxPacket.voltage, rxPacket.packetNum);
      UART_Send(tx_data + (TX_SIZE - size - 2), (u8)(size + 2));
    }
  }
}

uint8_t fillBuffer(uint32_t power, uint16_t battery, uint8_t packet) {
  uint8_t pos = TX_SIZE - 3;
  do {
    tx_data[pos--] = (char)('0' + battery % 10);
    battery /= 10;
  } while (battery);
  tx_data[pos--] = ',';
  do {
    tx_data[pos--] = (char)('0' + power % 10);
    power /= 10;
  } while (power);
  tx_data[pos--] = ',';
  do {
    tx_data[pos--] = (char)('0' + packet % 10);
    packet /= 10;
  } while (packet);
  return (uint8_t)(TX_SIZE - pos - 3);
}

void fail(void) {
  enableInterrupts();
  UART_Send("fail\r\n", 6);
  disableInterrupts();
  halt();
}