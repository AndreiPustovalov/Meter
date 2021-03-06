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

#define LED_PORT GPIOB
#define LED_PIN GPIO_Pin_3

#define TRAN_PORT GPIOC
#define TRAN_PIN GPIO_Pin_5

#define TRAN_EXTI_IT EXTI_IT_Pin5
#define TRAN_EXTI_PIN EXTI_Pin_5

#define BLINK_TIME 1
#define BLINK_DELAY 200

typedef enum {
  Event_WakeUp,
  Event_Debounce,
  Event_None
} Event_TypeDef;

void RTC_WakeUpConfig(void);
void TimerInit(uint16_t period);
void WakeUp(Event_TypeDef e);

volatile uint16_t powerCounter = 0;
struct Proto_Packet_TypeDef txPacket = {0};
volatile Event_TypeDef lastEvent;

void main()
{
  nRF24_TX_PCKT_TypeDef ret;
  RTC_WakeUpConfig();
  
  enableInterrupts();
  halt();
  disableInterrupts();

  CLK_SYSCLKDivConfig(CLK_SYSCLKDiv_1); // 16 MHz clock

  ADC_Config();

  GPIO_Init(LED_PORT, LED_PIN, GPIO_Mode_Out_PP_High_Fast);
  GPIO_Init(TRAN_PORT, TRAN_PIN, GPIO_Mode_In_FL_IT);

  EXTI_SetPinSensitivity(TRAN_EXTI_PIN, EXTI_Trigger_Falling);
  
  nRF24_Init();

  if (!nRF24_Check()) {
    GPIO_ResetBits(LED_PORT, LED_PIN);
    halt();
  }
  
  nRF24_TXMode(
    15,   // RetrCnt
    15,   // RetrDelay
    100, // RFChan
    nRF24_DataRate_1Mbps, 
    nRF24_TXPower_6dBm,
    nRF24_CRC_2byte, 
    nRF24_PWR_Down, 
    "pwmtr", // TX_Addr
    5        // TX_Addr_Width
  );

  RTC_WakeUpConfig();
  
  enableInterrupts();
  halt();
  while (1) {
    switch (lastEvent) {
      case Event_WakeUp:
        txPacket.voltage = ADC_MeasureBatVoltage();
        disableInterrupts();
        txPacket.power = powerCounter;
        enableInterrupts();
        nRF24_Wake();
        switch (nRF24_TXPacket(&txPacket, sizeof(txPacket))) {
          case nRF24_TX_SUCCESS:
            ++txPacket.packetNum;
            disableInterrupts();
            powerCounter = powerCounter - txPacket.power;
            enableInterrupts();
            #ifdef DEBUG
            GPIO_ResetBits(LED_PORT, LED_PIN);
            delay_ms(BLINK_TIME);
            GPIO_SetBits(LED_PORT, LED_PIN);
            #endif
            break;
          default:
            GPIO_ResetBits(LED_PORT, LED_PIN);
            delay_ms(BLINK_TIME);
            GPIO_SetBits(LED_PORT, LED_PIN);
            delay_ms(BLINK_DELAY);
            GPIO_ResetBits(LED_PORT, LED_PIN);
            delay_ms(BLINK_TIME);
            GPIO_SetBits(LED_PORT, LED_PIN);
            break;
        }
        nRF24_PowerDown();
        lastEvent = Event_None;
        halt();
        break;
      case Event_Debounce:
        lastEvent = Event_None;
        wfi();
        break;
      default:
        lastEvent = Event_None;
        halt();
        break;
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
  CFG->GCR |= CFG_GCR_AL;
  RTC_WakeUpCmd(ENABLE);
}

void TimerInit(uint16_t period) {
  CLK_PeripheralClockConfig(CLK_Peripheral_TIM2, ENABLE);
  TIM2_TimeBaseInit(TIM2_Prescaler_128, TIM2_CounterMode_Up, period);
  TIM2_SelectOnePulseMode(TIM2_OPMode_Single);
  TIM2_ITConfig(TIM2_IT_Update, ENABLE);
}

void WakeUp(Event_TypeDef e) {
  lastEvent = e;
  CFG->GCR &= (u8)~CFG_GCR_AL;
}

@far @interrupt void RTC_Interrupt(void)
{
  RTC_ClearITPendingBit(RTC_IT_WUT);
  WakeUp(Event_WakeUp);
}

@far @interrupt void EXTI_Tran_IRQ(void) {
  EXTI_ClearITPendingBit(TRAN_EXTI_IT);
  ++powerCounter;
}

@far @interrupt void TIM2_ISR(void) {
  TIM2_ClearITPendingBit(TIM2_IT_Update);
}