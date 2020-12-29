#ifndef __ADC_H
#define __ADC_H

#include "stm8l15x.h"

void ADC_Config(void);
uint16_t ADC_MeasureBatVoltage(void);
@far @interrupt void ADC1_COMP_IRQHandler(void);

#endif