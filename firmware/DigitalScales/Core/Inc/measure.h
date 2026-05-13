

#ifndef INC_MEASURE_H_
#define INC_MEASURE_H_

#include <stdint.h>
#include <stdbool.h>

#define MEASURE_NUM_SAMPLES 4


// The HX711 takes 10 samples per second
#define MEASURE_SAMPLES_PER_SECOND 10
#define MEASURE_CAL_TIME_SECONDS 5
#define MEASURE_NUM_CAL_SAMPLES (MEASURE_CAL_TIME_SECONDS*MEASURE_SAMPLES_PER_SECOND)

typedef struct
{
	uint8_t in_samps;
	int32_t samples[MEASURE_NUM_SAMPLES];

	bool    calibration_complete;
	int32_t calibration_value;
	int16_t in_calsamps;
	int32_t calibration_samples[MEASURE_NUM_CAL_SAMPLES];
} measure_t;

void measure_init(measure_t *);
void measure_put(measure_t *, int32_t);
int32_t measure_get_avg(measure_t *);

void measure_calibrate_init(measure_t *p);
void measure_calibrate_put(measure_t *p, int32_t val);


#endif /* INC_MEASURE_H_ */
