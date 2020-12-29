/* MAIN.C file
 * 
 * Copyright (c) 2002-2005 STMicroelectronics
 */

#include "stm8l15x.h"
#include "stm8l15x_gpio.h"
#include "main.h"
#include "adc.h"
#include "nrf24.h"
#include "delay.h"
#include "protocol.h"

#define LED_PORT GPIOC
#define LED_PIN GPIO_Pin_5

#ifdef DEBUG
#define DELAY 100
#else
#define DELAY 10
#endif

void RTC_WakeUpConfig(void);

volatile uint16_t powerCounter = 0;

struct TX_Packet_TypeDef txPacket = {0};

void main()
{
  nRF24_TX_PCKT_TypeDef ret;

  CLK_SYSCLKDivConfig(CLK_SYSCLKDiv_1); // 16 MHz clock

  ADC_Config();

  GPIO_Init(LED_PORT, LED_PIN, GPIO_Mode_Out_PP_Low_Fast);

  nRF24_Init();

  if (!nRF24_Check()) {
    GPIO_SetBits(LED_PORT, LED_PIN);
    halt();
  }
  
  nRF24_TXMode(
    3,   // RetrCnt
    7,   // RetrDelay
    100, // RFChan
    nRF24_DataRate_1Mbps, 
    nRF24_TXPower_12dBm, 
    nRF24_CRC_2byte, 
    nRF24_PWR_Down, 
    "pwmtr", // TX_Addr
    5        // TX_Addr_Width
  );

  RTC_WakeUpConfig();
  enableInterrupts();
  while (1) {
    halt();
    txPacket.voltage = ADC_MeasureBatVoltage();
    disableInterrupts();
    txPacket.power = powerCounter;
    enableInterrupts();
    nRF24_Wake();
    if (nRF24_TXPacket(&txPacket, sizeof(txPacket)) == nRF24_TX_SUCCESS) {
      ++txPacket.packetNum;
      #ifdef DEBUG
      GPIO_SetBits(LED_PORT, LED_PIN);
      delay_ms(DELAY);
      GPIO_ResetBits(LED_PORT, LED_PIN);
      #endif
    } else {
      GPIO_SetBits(LED_PORT, LED_PIN);
      delay_ms(DELAY);
      GPIO_ResetBits(LED_PORT, LED_PIN);
      delay_ms(DELAY / 2);
      GPIO_SetBits(LED_PORT, LED_PIN);
      delay_ms(DELAY);
      GPIO_ResetBits(LED_PORT, LED_PIN);
    }    
  }
}

void RTC_WakeUpConfig() {
  CLK_RTCClockConfig(CLK_RTCCLKSource_LSI, CLK_RTCCLKDiv_16);
  while (CLK_GetFlagStatus(CLK_FLAG_LSIRDY) == RESET);
  CLK_PeripheralClockConfig(CLK_Peripheral_RTC, ENABLE);
  RTC_WakeUpClockConfig(RTC_WakeUpClock_RTCCLK_Div16);

  RTC_ITConfig(RTC_IT_WUT, ENABLE);
  while(RTC_GetFlagStatus(RTC_FLAG_WUTWF) != SET);
  RTC_SetWakeUpCounter(742);  // (38000 / 16 / 16) hz * 30 sec = 4453
  RTC_WakeUpCmd(ENABLE);
}

@far @interrupt void RTC_Interrupt(void)
{
  RTC_ClearITPendingBit(RTC_IT_WUT);
}