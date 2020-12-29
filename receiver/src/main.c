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

#define TX_SIZE 13
char tx_data[TX_SIZE] = "000000,0000\r\n";

void fail(void);
void fillBuffer(uint32_t power, uint16_t battery);

void main()
{
  CLK_SYSCLKDivConfig(CLK_SYSCLKDiv_1); // 16 MHz clock
  
  SYSCFG_REMAPPinConfig(REMAP_Pin_USART1TxRxPortC, ENABLE);
  
  GPIO_Init(LED_PORT, LED_PIN, GPIO_Mode_Out_PP_Low_Fast);

  UART_Init();

  nRF24_Init();
  
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
      fillBuffer(totalPower, rxPacket.voltage);
      UART_Send(TX_SIZE);
      GPIO_ToggleBits(LED_PORT, LED_PIN);
    }
  }
}

void fillBuffer(uint32_t power, uint16_t battery) {
  uint8_t pos = 10;
  while (battery) {
    tx_data[pos] = (char)('0' + battery % 10);
    battery /= 10;
    --pos;
  }
  pos = 5;
  while (power) {
    tx_data[pos] = (char)('0' + power % 10);
    power /= 10;
    --pos;
  }
}

void fail(void) {
  disableInterrupts();
  GPIO_SetBits(LED_PORT, LED_PIN);
  halt();
}