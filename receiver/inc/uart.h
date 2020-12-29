#ifndef __UART_H
#define __UART_H

#include "stm8l15x.h"

void UART_Init(void);
void UART_Send(char * tx_data, uint8_t size);
@far @interrupt void DMA_TransactionComplete(void);

#endif
