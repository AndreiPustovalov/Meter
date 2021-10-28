#ifndef __NRF24_CONF_BREADBOARD_H
#define __NRF24_CONF_BREADBOARD_H

#define CE_PORT   GPIOC
#define CE_PIN    GPIO_Pin_4

#define CSN_PORT  GPIOA
#define CSN_PIN   GPIO_Pin_3

#define SCK_PORT  GPIOB
#define SCK_PIN   GPIO_Pin_5

#define MOSI_PORT GPIOB
#define MOSI_PIN  GPIO_Pin_6

#define MISO_PORT GPIOB
#define MISO_PIN  GPIO_Pin_7

#define IRQ_PORT  GPIOC
#define IRQ_PIN   GPIO_Pin_6

#define IRQ_EXTI_PIN  EXTI_Pin_6
#define IRQ_EXTI_IT  EXTI_IT_Pin6

#endif