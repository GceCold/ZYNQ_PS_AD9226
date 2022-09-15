/*
 * deviceids.h
 *
 *  Created on: 2022Äê9ÔÂ8ÈÕ
 *      Author: IceCold
 */

#include "xparameters.h"
#include "xgpiops.h"

#ifndef SRC_DEVICEIDS_H_
#define SRC_DEVICEIDS_H_


#define GPIO_DEVICE_ID  	XPAR_XGPIOPS_0_DEVICE_ID

#define TIMER_DEVICE_ID    	XPAR_XSCUTIMER_0_DEVICE_ID
#define TIMER_LOAD_VALUE  	(XPAR_PS7_CORTEXA9_0_CPU_CLK_FREQ_HZ/2 -1)
#define READ_TIME			5

#define CACHE_NUMBER		200
#define CALCULATE_NUMBER	CACHE_NUMBER * 5

#endif /* SRC_DEVICEIDS_H_ */
