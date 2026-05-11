

#ifndef INC_MEASURE_H_
#define INC_MEASURE_H_

#include <stdint.h>

#define MEASURE_NUM_SAMPLES 1000

typedef struct
{
	uint8_t in_samps;
	int32_t samples[MEASURE_NUM_SAMPLES];
} measure_t;

void measure_init(measure_t *);
void measure_put(measure_t *, int32_t);
int32_t measure_get_avg(measure_t *);

#endif /* INC_MEASURE_H_ */
