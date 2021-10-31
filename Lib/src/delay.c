/**
  ******************************************************************************
  * @file    delay.c
  * @author  Microcontroller Division
  * @version V1.2.0
  * @date    09/2010
  * @brief   delay functions
  ******************************************************************************
  * @copy
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2010 STMicroelectronics</center></h2>
  */

/* Includes ------------------------------------------------------------------*/

#include "stm8l15x_clk.h"
#include "stm8l15x_wfe.h"
/**
  * @brief  delay for some time in ms unit
  * @caller auto_test
  * @param  n_ms is how many ms of time to delay
  * @retval None
  */
void delay_ms(u16 n_ms)
{
/* Init TIMER 4 */
  CLK_PeripheralClockConfig(CLK_Peripheral_TIM4, ENABLE);
  WFE_WakeUpSourceEventCmd(WFE_Source_TIM4_EV, ENABLE);
  TIM4_ITConfig(TIM4_IT_Update, ENABLE);

/* Init TIMER 4 prescaler: / (2^6) = /64 */
/* HSI div by 1 --> Auto-Reload value: 16M / 64 = 1/4M, 1/4M / 1k = 250*/
/* HSI div by 2 --> Auto-Reload value: 8M / 64 = 1/8M, 1/8M / 1k = 125*/
  TIM4_TimeBaseInit(TIM4_Prescaler_64, 125);
  TIM4_ClearITPendingBit(TIM4_IT_Update);
  TIM4_SetCounter(4);
  TIM4_Cmd(ENABLE);

  while(n_ms--)
  {
    wfe();
    _asm("JRA next");
    _asm("next:");
    TIM4_ClearITPendingBit(TIM4_IT_Update);
  }

/* Disable Counter */
  TIM4_Cmd(DISABLE);
  CLK_PeripheralClockConfig(CLK_Peripheral_TIM4, DISABLE);
}

/**
  * @brief  delay for some time in 10us unit(partial accurate)
  * @caller auto_test
  * @param n_10us is how many 10us of time to delay
  * @retval None
  */
void delay_10us(u16 n_10us)
{
/* Init TIMER 4 */
  CLK_PeripheralClockConfig(CLK_Peripheral_TIM4, ENABLE);
  WFE_WakeUpSourceEventCmd(WFE_Source_TIM4_EV, ENABLE);
  TIM4_ITConfig(TIM4_IT_Update, ENABLE);

/* prescaler: / (2^0) = /1 */
/* SYS_CLK_HSI_DIV1 Auto-Reload value: 16M / 1 = 16M, 16M / 100k = 160 */
/* SYS_CLK_HSI_DIV2 Auto-Reload value: 8M / 1 = 8M, 8M / 100k = 80 */
  TIM4_TimeBaseInit(TIM4_Prescaler_1, 80);

/* Counter value: 10, to compensate the initialization of TIMER */
  TIM4_SetCounter(10);

/* clear update flag */
  TIM4_ClearITPendingBit(TIM4_IT_Update);

/* Enable Counter */
  TIM4_Cmd(ENABLE);

  while(n_10us--)
  {
    wfe();
    _asm("JRA next10");
    _asm("next10:");
    TIM4_ClearITPendingBit(TIM4_IT_Update);
  }

/* Disable Counter */
  TIM4_Cmd(DISABLE);
  CLK_PeripheralClockConfig(CLK_Peripheral_TIM4, DISABLE);
}

/******************* (C) COPYRIGHT 2010 STMicroelectronics *****END OF FILE****/
