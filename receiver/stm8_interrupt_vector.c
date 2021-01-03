/*	BASIC INTERRUPT VECTOR TABLE FOR STM8 devices
 *	Copyright (c) 2007 STMicroelectronics
 */

#include "uart.h"
#include "nrf24_hal.h"

typedef void @far (*interrupt_handler_t)(void);

struct interrupt_vector {
	unsigned char interrupt_instruction;
	interrupt_handler_t interrupt_handler;
};

@far @interrupt void NonHandledInterrupt (void)
{
	/* in order to detect unexpected events during development, 
	   it is recommended to set a breakpoint on the following instruction
	*/
	return;
}

extern void _stext();     /* startup routine */

struct interrupt_vector const _vectab[] = {
	{0x82, (interrupt_handler_t)_stext}, /* reset */
	{0x82, NonHandledInterrupt}, /* trap  */
	{0x82, NonHandledInterrupt}, /* TLI  */
	{0x82, NonHandledInterrupt}, /* FLASH  */
	{0x82, DMA_TransactionComplete}, /* DMA1 0/1  */
	{0x82, NonHandledInterrupt}, /* DMA1 2/3  */
	{0x82, NonHandledInterrupt}, /* RTC  */
	{0x82, NonHandledInterrupt}, /* PVD  */
	{0x82, NonHandledInterrupt}, /* EXTIB  */
	{0x82, NonHandledInterrupt}, /* EXTID  */
	{0x82, NonHandledInterrupt}, /* EXTI0  */
	{0x82, NonHandledInterrupt}, /* EXTI1  */
	{0x82, NonHandledInterrupt}, /* EXIT2 */
	{0x82, NonHandledInterrupt}, /* EXTI3 */
	{0x82, NonHandledInterrupt}, /* EXTI4 */
	{0x82, NonHandledInterrupt}, /* EXTI5 */
	{0x82, EXTI_nRF24_IRQ},      /* EXTI6 */
	{0x82, NonHandledInterrupt}, /* EXTI7 */
	{0x82, NonHandledInterrupt}, /* Reserved */
	{0x82, NonHandledInterrupt}, /* CLK */
	{0x82, NonHandledInterrupt}, /* ADC1 */
	{0x82, NonHandledInterrupt}, /* TIM2 Update */
	{0x82, NonHandledInterrupt}, /* TIM2 Capture */
	{0x82, NonHandledInterrupt}, /* TIM3 Update */
	{0x82, NonHandledInterrupt}, /* TIM3 Capture */
	{0x82, NonHandledInterrupt}, /* RI */
	{0x82, NonHandledInterrupt}, /* Reserved */
	{0x82, NonHandledInterrupt}, /* TIME4 */
	{0x82, NonHandledInterrupt}, /* SPI1 */
	{0x82, NonHandledInterrupt}, /* USART1 tx */
	{0x82, NonHandledInterrupt}, /* USART1 rx */
	{0x82, NonHandledInterrupt}, /* I2C1 */
};
