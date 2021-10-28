#ifndef __NRF24_CONF_H
#define __NRF24_CONF_H

#ifdef NRF24_BREADBOARD
#include "nRF24_conf_breadboard.h"
#else
#ifdef NRF24_MODULE
#include "nRF24_conf_module.h"
#else
#error "Specify target"
#endif
#endif
#endif