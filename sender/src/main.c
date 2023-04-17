/* MAIN.C file
 * 
 * Copyright (c) 2002-2005 STMicroelectronics
 */

#include "stm8l15x.h"
#include "stm8l15x_gpio.h"
#include "stm8l15x_tim2.h"
#include "stm8l15x_pwr.h"
#include "stm8l15x_exti.h"
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

void TimerInit(uint16_t period);
void WakeUp(void);
void Sleep(void);

static struct Proto_Packet_TypeDef txPacket = {0};

void main()
{
  nRF24_TX_PCKT_TypeDef ret;
	uint16_t previous;

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
	
	TimerInit(6250); // 100ms
  
  PWR_UltraLowPowerCmd(ENABLE);

  nRF24_Init();
  delay_ms(500);

  EXTI_SetPinSensitivity(TRAN_EXTI_PIN, EXTI_Trigger_Rising_Falling);

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

  nRF24_Wake();
  txPacket.power = 255;
	txPacket.voltage = ADC_MeasureBatVoltage();
  if (GPIO_ReadInputDataBit(TRAN_PORT, TRAN_PIN))
		txPacket.power = 1;
	else
	  txPacket.power = 0;
  ret = nRF24_TXPacket(&txPacket, sizeof(txPacket));
  nRF24_PowerDown();
	
	previous = txPacket.power;

  enableInterrupts();
  while (1) {
    GPIO_Init(TRAN_PORT, TRAN_PIN, GPIO_Mode_In_PU_IT);
    Sleep();
		if (GPIO_ReadInputDataBit(TRAN_PORT, TRAN_PIN))
			txPacket.power = 1;
		else
		  txPacket.power = 0;
		if (previous == txPacket.power)
		  continue;
    nRF24_Wake();
		ret = nRF24_TXPacket(&txPacket, sizeof(txPacket));
    switch (ret) {
      case nRF24_TX_SUCCESS:
        ++txPacket.packetNum;
				previous = txPacket.power;
        break;
			case nRF24_NO_IRQ:
        GPIO_ResetBits(LED_PORT, LED_PIN);
        delay_ms(BLINK_TIME);
        GPIO_SetBits(LED_PORT, LED_PIN);
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
		if ((txPacket.packetNum & 0xf) == 0)
			txPacket.voltage = ADC_MeasureBatVoltage();
  }
}

void TimerInit(uint16_t period) {
  CLK_PeripheralClockConfig(CLK_Peripheral_TIM2, ENABLE);
  TIM2_TimeBaseInit(TIM2_Prescaler_128, TIM2_CounterMode_Up, period);
  TIM2_SelectOnePulseMode(TIM2_OPMode_Single);
  TIM2_ITConfig(TIM2_IT_Update, ENABLE);
	TIM2_ClearITPendingBit(TIM2_IT_Update);
	TIM2_Cmd(DISABLE);
}

void WakeUp() {
  CFG->GCR &= (u8)~CFG_GCR_AL;
}

void Sleep() {
  CFG->GCR |= CFG_GCR_AL;
  halt();
}

@far @interrupt void EXTI_Tran_IRQ(void) {
  EXTI_ClearITPendingBit(TRAN_EXTI_IT);
  TIM2_Cmd(ENABLE);
}

@far @interrupt void TIM2_ISR(void) {
  TIM2_ClearITPendingBit(TIM2_IT_Update);
	GPIO_Init(TRAN_PORT, TRAN_PIN, GPIO_Mode_In_PU_No_IT);
	WakeUp();
}
