#ifndef MQ_7_H
#define MQ_7_H

#include <math.h>
#include "adc.h"

#define MQ7_READ_TIMES 10

void mq7_task(void);

extern uint32_t mq7_adc_value;

extern float ppm;

#endif
