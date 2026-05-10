

#ifndef INC_MEASURE_H_
#define INC_MEASURE_H_

#include <stdint.h>
#include "stm32g4xx_hal.h"

void measure_set_i2c(I2C_HandleTypeDef *p_hi2c);
void measure(int32_t measurement);

#endif /* INC_MEASURE_H_ */
