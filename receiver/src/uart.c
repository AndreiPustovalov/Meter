#include "stm8l15x_usart.h"
#include "stm8l15x_dma.h"
#include "stm8l15x_gpio.h"

extern char tx_data[];

bool dma_transaction_compelete = FALSE;
bool uart_transmission_compelete = FALSE;
uint16_t wait = 4095;
  

void UART_Init(void) {
//  GPIO_Init(GPIOC, GPIO_Pin_5, GPIO_Mode_Out_PP_High_Fast);
//  GPIO_Init(GPIOC, GPIO_Pin_6, GPIO_Mode_In_PU_No_IT);
  CLK_PeripheralClockConfig(CLK_Peripheral_USART1, ENABLE);
  USART_Init(
    USART1, 
    19200, 
    USART_WordLength_8b,
    USART_StopBits_1, 
    USART_Parity_No, 
    USART_Mode_Tx
  );
  // USART_ITConfig(USART1, USART_IT_TC, ENABLE);
  USART_Cmd(USART1, DISABLE);

  CLK_PeripheralClockConfig(CLK_Peripheral_DMA1, ENABLE);
  //DMA_GlobalDeInit(); 
  //DMA_DeInit(DMA1_Channel1); 
  DMA_Init(
    DMA1_Channel1, 
    (uint16_t) tx_data, 
    (uint16_t) &USART1->DR,
    0,
    DMA_DIR_MemoryToPeripheral,
    DMA_Mode_Normal,
    DMA_MemoryIncMode_Inc,
    DMA_Priority_High,
    DMA_MemoryDataSize_Byte
  );
  DMA_ITConfig(DMA1_Channel1, DMA_ITx_TC, ENABLE);
  USART_DMACmd(USART1, USART_DMAReq_TX, ENABLE);
  DMA_GlobalCmd(ENABLE);
}

void UART_Send(uint8_t size) {
  USART_Cmd(USART1, ENABLE);
  DMA_SetCurrDataCounter(DMA1_Channel1, size);
  DMA_Cmd(DMA1_Channel1, ENABLE); 
  dma_transaction_compelete = FALSE;
  while (!dma_transaction_compelete) {
    wfi();
  }
  while(wait--); // Needed to send correct first line adter reset. Hove no idea why.
  while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) != SET);
  while(USART_GetFlagStatus(USART1, USART_FLAG_TC) != SET);
  DMA_Cmd(DMA1_Channel1, DISABLE);
  USART_Cmd(USART1, DISABLE);
}

@far @interrupt void DMA_TransactionComplete(void) {
  DMA_ClearITPendingBit(DMA1_IT_TC1);
  dma_transaction_compelete = TRUE;
}