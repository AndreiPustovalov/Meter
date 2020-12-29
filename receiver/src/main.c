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

#define LED_PORT GPIOB
#define LED_PIN GPIO_Pin_1

struct Proto_Packet_TypeDef rxPacket;
uint32_t totalPower = 0;

#define TX_SIZE 22
char tx_data[TX_SIZE];

void fail(void);
uint8_t fillBuffer(uint32_t power, uint16_t battery, uint8_t packet);

void main()
{
  uint8_t size;
  CLK_SYSCLKDivConfig(CLK_SYSCLKDiv_1); // 16 MHz clock
  
  SYSCFG_REMAPPinConfig(REMAP_Pin_USART1TxRxPortC, ENABLE);
  
  GPIO_Init(LED_PORT, LED_PIN, GPIO_Mode_Out_PP_Low_Fast);

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
  while (1) {
    if (nRF24_Wait_IRQ(0)) {
      if (nRF24_RXPacket(&rxPacket, sizeof(rxPacket)) != nRF24_RX_PCKT_PIPE0) {
        fail();
      }
      totalPower += rxPacket.power;
      size = fillBuffer(totalPower, rxPacket.voltage, rxPacket.packetNum);
      UART_Send(tx_data + (TX_SIZE - size - 2), size + 2);
      GPIO_ToggleBits(LED_PORT, LED_PIN);
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
  disableInterrupts();
  GPIO_SetBits(LED_PORT, LED_PIN);
  halt();
}