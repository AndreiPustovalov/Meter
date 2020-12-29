#include "adc.h"

#define FACTORY_REF_VOLTAGE 1671

uint16_t ADCdata;

uint16_t ADC_MeasureBatVoltage() {
  ADCdata = 0;
  ADC_SoftwareStartConv(ADC1);
  while (!ADCdata) {
    wfi();
  }
  return (uint16_t)(((uint32_t)FACTORY_REF_VOLTAGE) * 1000 * 3 / (uint32_t)ADCdata) / 10 * 10;
}

void ADC_Config(void)
{
  CLK_PeripheralClockConfig(CLK_Peripheral_ADC1, ENABLE);
  /* Initialise and configure ADC1 */
  ADC_Init(ADC1, ADC_ConversionMode_Single, ADC_Resolution_12Bit, ADC_Prescaler_2);
  ADC_SamplingTimeConfig(ADC1, ADC_Group_SlowChannels, ADC_SamplingTime_384Cycles);
  ADC_VrefintCmd(ENABLE);
  /* Enable ADC1 */
  ADC_Cmd(ADC1, ENABLE);

  /* Enable ADC1 Channel 3 */
  ADC_ChannelCmd(ADC1, ADC_Channel_Vrefint, ENABLE);

  /* Enable End of conversion ADC1 Interrupt */
  ADC_ITConfig(ADC1, ADC_IT_EOC, ENABLE);
}


@far @interrupt void ADC1_COMP_IRQHandler(void) {
  ADCdata = ADC_GetConversionValue(ADC1);
}