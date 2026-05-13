/*
 * measure.c
 *
 *  Created on: 10 May 2026
 *      Author: kirkh
 */

#include <stdio.h>

#include "measure.h"
#include "app.h"

void measure_init(measure_t *p)
{
	p->calibration_complete = false;
	p->calibration_value = 0;
	p->in_samps = 0;
	for(int i = 0; i < MEASURE_NUM_SAMPLES; i++) {
		p->samples[i] = 0;
	}
	measure_calibrate_init(p);
}
void measure_put(measure_t *p, int32_t val)
{
	p->samples[p->in_samps] = val;
	p->in_samps++;
	if(p->in_samps >= MEASURE_NUM_SAMPLES) {
		p->in_samps = 0;
	}
}

int32_t measure_get_avg(measure_t *p)
{
	int32_t rval = 0;
	for(int i = 0; i < MEASURE_NUM_SAMPLES; i++) {
		rval += p->samples[i];
	}
	return (int32_t)(rval / MEASURE_NUM_SAMPLES);
}

void measure_calibrate_init(measure_t *p)
{
	p->calibration_complete = false;
	p->calibration_value = 0;
	p->in_calsamps = 0;
	for(int i = 0; i < MEASURE_NUM_CAL_SAMPLES; i++) {
		p->calibration_samples[i] = 0;
	}
}

void measure_calibrate_put(measure_t *p, int32_t val)
{
	p->calibration_samples[p->in_calsamps] = val;
	p->in_calsamps++;
	if(p->in_calsamps >= MEASURE_NUM_CAL_SAMPLES) {
		int32_t cal = 0;
		p->in_calsamps = 0;
		p->calibration_complete = true;
		for(int i = 0; i < MEASURE_NUM_CAL_SAMPLES; i++) {
			cal += p->calibration_samples[i];
		}
		p->calibration_value = (cal / MEASURE_NUM_CAL_SAMPLES);
	}
}

