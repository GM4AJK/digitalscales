#ifndef INC_APP_H_
#define INC_APP_H_

#include <stdbool.h>

typedef enum {
	DNCNT_SHARED = 0,
	DNCNT_AUTO_TARE,
	DNCNT_LAST
} DNCNT_e;

bool dncnt_timedout(DNCNT_e i);
uint32_t dncnt_get(DNCNT_e i);
uint32_t dncnt_set(DNCNT_e i, uint32_t val);


void app_init(void);
void app_loop(void);
void app_tick(void);



#endif /* INC_APP_H_ */
