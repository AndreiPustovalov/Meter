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

const char hex_table[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

struct Proto_Packet_TypeDef rxPacket;
uint32_t totalPower = 0;

#define TX_SIZE 22
char tx_data[TX_SIZE];


void fail(char* msg, uint8_t size);
uint8_t fillBuffer(uint32_t power, uint16_t battery, uint8_t packet);
void UART_SendStr(char* tx_data, uint8_t size);

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
  delay_ms(500);
  
  if (!nRF24_Check()) {
    fail("Failed to init\r\n", 16);
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
  UART_SendStr("Hello!\r\n", 8);
  while (1) {
    if (nRF24_Wait_IRQ(0)) {
      size = nRF24_RXPacket(&rxPacket, sizeof(rxPacket));
      if (size != nRF24_RX_PCKT_PIPE0) {
        UART_SendStr("RX Error: ", 10);
        tx_data[0] = hex_table[size % 16];
        size /= 16;
        tx_data[1] = hex_table[size % 16];
        tx_data[2] = '\r';
        tx_data[3] = '\n';
        UART_Send(tx_data, 4);
        continue;
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

void UART_SendStr(char* s, uint8_t size) {
  char *dest = tx_data;
  while(*dest++ = *s++);
  UART_Send(tx_data, size);
}

void fail(char* msg, uint8_t size) {
  enableInterrupts();
  UART_SendStr(msg, size);
  disableInterrupts();
  halt();
}