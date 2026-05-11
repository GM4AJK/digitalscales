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
	p->in_samps = 0;
	for(int i = 0; i < MEASURE_NUM_SAMPLES; i++) {
		p->samples[i] = 0;
	}
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

