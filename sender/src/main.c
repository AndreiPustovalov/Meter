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
#include "stm8l15x_pwr.h"

#define LED_PORT GPIOB
#define LED_PIN GPIO_Pin_3

#define TRAN_PORT GPIOC
#define TRAN_PIN GPIO_Pin_5

#define TRAN_EXTI_IT EXTI_IT_Pin5
#define TRAN_EXTI_PIN EXTI_Pin_5

#define BLINK_TIME 1
#define BLINK_DELAY 200

// (38000 / 16 / 16) hz * 30 sec = 4453
#define RTC_WAKE_UP_INTERVAL 4453


void RTC_WakeUpConfig(void);
void TimerInit(uint16_t period);
void WakeUp(void);
void Sleep(void);

static volatile uint16_t powerCounter[2] = {0, 0};
static volatile uint8_t counterIndex = 0;
static struct Proto_Packet_TypeDef txPacket = {0};

void main()
{
  uint16_t newPowerCounter;
  nRF24_TX_PCKT_TypeDef ret;

  CLK_SYSCLKDivConfig(CLK_SYSCLKDiv_2); // 8 MHz clock
  
  GPIO_Init(GPIOA, GPIO_Pin_0, GPIO_Mode_In_PU_No_IT);
  GPIO_Init(GPIOA, GPIO_Pin_2, GPIO_Mode_In_PU_No_IT);
  GPIO_Init(GPIOA, GPIO_Pin_3, GPIO_Mode_In_PU_No_IT);
  GPIO_Init(GPIOB, GPIO_Pin_0, GPIO_Mode_In_PU_No_IT);
  GPIO_Init(GPIOB, GPIO_Pin_1, GPIO_Mode_In_PU_No_IT);
  GPIO_Init(GPIOB, GPIO_Pin_3, GPIO_Mode_In_PU_No_IT);
  GPIO_Init(GPIOC, GPIO_Pin_0, GPIO_Mode_In_PU_No_IT);
  GPIO_Init(GPIOC, GPIO_Pin_1, GPIO_Mode_In_PU_No_IT);
  GPIO_Init(GPIOC, GPIO_Pin_4, GPIO_Mode_In_PU_No_IT);
  GPIO_Init(GPIOC, GPIO_Pin_5, GPIO_Mode_In_PU_No_IT);
  GPIO_Init(GPIOD, GPIO_Pin_0, GPIO_Mode_In_PU_No_IT);

  GPIO_Init(LED_PORT, LED_PIN, GPIO_Mode_Out_PP_High_Slow);
  GPIO_Init(TRAN_PORT, TRAN_PIN, GPIO_Mode_In_FL_IT);
  
  PWR_UltraLowPowerCmd(ENABLE);

  nRF24_Init();
  delay_ms(500);

  EXTI_SetPinSensitivity(TRAN_EXTI_PIN, EXTI_Trigger_Falling);

  ADC_Config();

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
  nRF24_PowerDown();

  RTC_WakeUpConfig();
  RTC_SetWakeUpCounter(RTC_WAKE_UP_INTERVAL);

  enableInterrupts();
  while (1) {
    Sleep();
    txPacket.voltage = ADC_MeasureBatVoltage();
    counterIndex ^= 0x01;
    txPacket.power = powerCounter[counterIndex ^ 0x01];
    nRF24_Wake();
    switch (nRF24_TXPacket(&txPacket, sizeof(txPacket))) {
      case nRF24_TX_SUCCESS:
        ++txPacket.packetNum;
        powerCounter[counterIndex ^ 0x01] = 0;
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
  }
}

void RTC_WakeUpConfig() {
  CLK_RTCClockConfig(CLK_RTCCLKSource_LSI, CLK_RTCCLKDiv_16);
  while (CLK_GetFlagStatus(CLK_FLAG_LSIRDY) == RESET);
  CLK_PeripheralClockConfig(CLK_Peripheral_RTC, ENABLE);
  RTC_WakeUpClockConfig(RTC_WakeUpClock_RTCCLK_Div16);

  RTC_ITConfig(RTC_IT_WUT, ENABLE);
  while(RTC_GetFlagStatus(RTC_FLAG_WUTWF) != SET);
  RTC_SetWakeUpCounter(RTC_WAKE_UP_INTERVAL);
  CFG->GCR |= CFG_GCR_AL;
  RTC_WakeUpCmd(ENABLE);
}

void TimerInit(uint16_t period) {
  CLK_PeripheralClockConfig(CLK_Peripheral_TIM2, ENABLE);
  TIM2_TimeBaseInit(TIM2_Prescaler_128, TIM2_CounterMode_Up, period);
  TIM2_SelectOnePulseMode(TIM2_OPMode_Single);
  TIM2_ITConfig(TIM2_IT_Update, ENABLE);
}

void WakeUp() {
  CFG->GCR &= (u8)~CFG_GCR_AL;
}

void Sleep() {
  CFG->GCR |= CFG_GCR_AL;
  halt();
}

@far @interrupt void RTC_Interrupt(void)
{
  RTC_ClearITPendingBit(RTC_IT_WUT);
  WakeUp();
}

@far @interrupt void EXTI_Tran_IRQ(void) {
  EXTI_ClearITPendingBit(TRAN_EXTI_IT);
  ++powerCounter[counterIndex];
}

@far @interrupt void TIM2_ISR(void) {
  TIM2_ClearITPendingBit(TIM2_IT_Update);
}
