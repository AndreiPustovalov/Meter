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

#define BLINK_TIME 1
#define BLINK_DELAY 200

#define INPUT0_PORT GPIOA
#define INPUT0_PIN GPIO_Pin_2
#define INPUT0_EXTI_IT EXTI_IT_Pin2
#define INPUT0_EXTI_PIN EXTI_Pin_2

#define INPUT1_PORT GPIOA
#define INPUT1_PIN GPIO_Pin_3
#define INPUT1_EXTI_IT EXTI_IT_Pin3
#define INPUT1_EXTI_PIN EXTI_Pin_3

#define INPUT2_PORT GPIOC
#define INPUT2_PIN GPIO_Pin_4
#define INPUT2_EXTI_IT EXTI_IT_Pin4
#define INPUT2_EXTI_PIN EXTI_Pin_4

#define INPUT3_PORT GPIOC
#define INPUT3_PIN GPIO_Pin_5
#define INPUT3_EXTI_IT EXTI_IT_Pin5
#define INPUT3_EXTI_PIN EXTI_Pin_5

void TimerInit(uint16_t period);
void WakeUp(void);
void Sleep(void);
void ReadState(void);
void IO_Enable(void);
void IO_Disable(void);

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

  EXTI_SetPinSensitivity(INPUT0_EXTI_PIN, EXTI_Trigger_Rising_Falling);
  EXTI_SetPinSensitivity(INPUT1_EXTI_PIN, EXTI_Trigger_Rising_Falling);
  EXTI_SetPinSensitivity(INPUT2_EXTI_PIN, EXTI_Trigger_Rising_Falling);
  EXTI_SetPinSensitivity(INPUT3_EXTI_PIN, EXTI_Trigger_Rising_Falling);

  ADC_Config();

  if (!nRF24_Check()) {
    GPIO_ResetBits(LED_PORT, LED_PIN);
    halt();
  }

  nRF24_TXMode(
    15,   // RetrCnt
    15,   // RetrDelay
    100, // RFChan
    nRF24_DataRate_250kbps, 
    nRF24_TXPower_0dBm,
    nRF24_CRC_2byte, 
    nRF24_PWR_Down, 
    "pwmtr", // TX_Addr
    5        // TX_Addr_Width
  );

  nRF24_Wake();
	txPacket.voltage = ADC_MeasureBatVoltage();
  ReadState();
  ret = nRF24_TXPacket(&txPacket, sizeof(txPacket));
  nRF24_PowerDown();
	
	previous = txPacket.state;

  enableInterrupts();
  while (1) {
    IO_Enable();
    halt(); // Wait for external interrupt
		wfi(); // Wait for timer event
		ReadState();
		if (previous == txPacket.state)
		  continue;
    nRF24_Wake();
		ret = nRF24_TXPacket(&txPacket, sizeof(txPacket));
    switch (ret) {
      case nRF24_TX_SUCCESS:
        ++txPacket.packetNum;
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
		previous = txPacket.state;
		if ((txPacket.packetNum & 0xf) == 0)
			txPacket.voltage = ADC_MeasureBatVoltage();
  }
}

void IO_Enable() {
	GPIO_Init(INPUT0_PORT, INPUT0_PIN, GPIO_Mode_In_PU_IT);
	GPIO_Init(INPUT1_PORT, INPUT1_PIN, GPIO_Mode_In_PU_IT);
	GPIO_Init(INPUT2_PORT, INPUT2_PIN, GPIO_Mode_In_PU_IT);
	GPIO_Init(INPUT3_PORT, INPUT3_PIN, GPIO_Mode_In_PU_IT);
}

void IO_Disable() {
	GPIO_Init(INPUT0_PORT, INPUT0_PIN, GPIO_Mode_In_PU_No_IT);
	GPIO_Init(INPUT1_PORT, INPUT1_PIN, GPIO_Mode_In_PU_No_IT);
	GPIO_Init(INPUT2_PORT, INPUT2_PIN, GPIO_Mode_In_PU_No_IT);
	GPIO_Init(INPUT3_PORT, INPUT3_PIN, GPIO_Mode_In_PU_No_IT);
}

void ReadState() {
	txPacket.state = 0;
	if (GPIO_ReadInputDataBit(INPUT0_PORT, INPUT0_PIN)) txPacket.io.input0 = 1;
	if (GPIO_ReadInputDataBit(INPUT1_PORT, INPUT1_PIN)) txPacket.io.input1 = 1;
	if (GPIO_ReadInputDataBit(INPUT2_PORT, INPUT2_PIN)) txPacket.io.input2 = 1;
	if (GPIO_ReadInputDataBit(INPUT3_PORT, INPUT3_PIN)) txPacket.io.input3 = 1;
}

void TimerInit(uint16_t period) {
  CLK_PeripheralClockConfig(CLK_Peripheral_TIM2, ENABLE);
  TIM2_TimeBaseInit(TIM2_Prescaler_128, TIM2_CounterMode_Up, period);
  TIM2_SelectOnePulseMode(TIM2_OPMode_Single);
  TIM2_ITConfig(TIM2_IT_Update, ENABLE);
	TIM2_ClearITPendingBit(TIM2_IT_Update);
	TIM2_ClearFlag(TIM2_FLAG_Update);
	TIM2_Cmd(DISABLE);
}

@far @interrupt void EXTI_Tran_IRQ(void) {
	IO_Disable();
  EXTI_ClearITPendingBit(INPUT0_EXTI_IT);
	EXTI_ClearITPendingBit(INPUT1_EXTI_IT);
	EXTI_ClearITPendingBit(INPUT2_EXTI_IT);
	EXTI_ClearITPendingBit(INPUT3_EXTI_IT);
  TIM2_Cmd(ENABLE);
}

@far @interrupt void TIM2_ISR(void) {
  TIM2_ClearITPendingBit(TIM2_IT_Update);
}
