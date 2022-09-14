#include "stdio.h"
#include "deviceids.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "xgpiops.h"
#include "sleep.h"
#include "xscutimer.h"
#include "math.h"
#include "xqspips.h"
#include "flash.h"

XGpioPs Gpio;
XGpioPs_Config *ConfigPtr;

XScuTimer XSc_Timer;
XScuTimer_Config *TimerConfigPtr;
XScuTimer *TimerInstancePtr = &XSc_Timer;

void init_gpio();
void read_gpio(u8 *data_array);
int timerSet_S();
u8 checkCache(u8 count);
void initTimer();

//----------------------GPIO Buffer
u8 buffer[12];
//数据缓存
int cache[200];
u32 flashOffset = 0;
//----------------------
u8 readTime = 0;
u8 numberAll = 0;
//----------------------

int main(){
	InitQspi();
	usleep(100);
	initTimer();
	usleep(100);
	init_gpio();
	usleep(100);

	u8 signal = 0;
	u8 count = 0;

	memset(cache,(u32)0x00,sizeof(buffer));
	XScuTimer_Start(TimerInstancePtr);
	while(1){
		memset(buffer,0x00,sizeof(buffer));
		read_gpio(buffer);
		int result = 0;
		for(int i = 0;i < 12;i++){
			result += (buffer[11-i] & 0x0F) << i;
		}

		if(!signal){
			if(abs(result - 2048) < 50){
				continue;
			}
			printf("工作状态..\r\n");
			XScuTimer_RestartTimer(TimerInstancePtr);
			signal = 1;
		}else {
			signal = checkCache(count);
		}

		if(count == 0 || (count > 0 && abs(cache[count-1] - result) < 500)){
			cache[count] = result;
			count++;
		}

		if(count == 200){
			int max = cache[0], min = cache[0];
			for(int i = 0;i < 200;i++) {
				if(cache[i] > max){
					max = cache[i];
				}
				if(cache[i] < min){
					min = cache[i];
				}
			}

			int average = ((max - min) / 2);

			int number = 0;
			for(int i = 1;i < 200;i++) {
				if(((cache[i-1] - average) > 0 && (cache[i] - average) < 0) ||
						((cache[i-1] - average) < 0 && (cache[i] - average) > 0) ||
						(cache[i-1] - average) == 0 || (cache[i-1] - average) == 0
				){
					number++;
				}
			}

			numberAll += number;

			printf("%d \r\n", numberAll);
//			printf("%d", (number / 2)/readTime);

//			(cache[i]/(float)4095)*10-5;

			if(readTime == READ_TIME){
				u32 time = XScuTimer_GetCounterValue(TimerInstancePtr);
			//	printf("%d \r\n", time);
				double readTime = ((double)1000000/TIMER_LOAD_VALUE)*(double)time;

//				printf("%d \r\n", numberAll);

				XScuTimer_RestartTimer(TimerInstancePtr);
				readTime = 0;
				numberAll = 0;
			}else{
				readTime++;
			}

			memset(cache,(u32)0x00,sizeof(buffer));
			count = 0;
		}

	}

	return 0;
}

u8 checkCache(u8 count){
	if(count > 30){
		for(u8 i = count-30;i < count;i++){
			if(abs(cache[i] - 2048) > 50){
				return 1;
			}
		}
		XScuTimer_Stop(TimerInstancePtr);
		printf("空闲状态..\r\n");
		return 0;
	}
	return 1;
}

void read_gpio(u8 *data_array){
	for(u32 i = 0;i < 12;i++){
		data_array[i] = XGpioPs_ReadPin(&Gpio,54 + i);
	}
}

void init_gpio(){
	ConfigPtr = XGpioPs_LookupConfig(GPIO_DEVICE_ID);
	XGpioPs_CfgInitialize(&Gpio, ConfigPtr, ConfigPtr->BaseAddr);
	for(int id = 0;id < 12;id++){
		XGpioPs_SetDirectionPin(&Gpio, 54 + id, 0);
	}

}

void initTimer(){
	TimerConfigPtr = XScuTimer_LookupConfig(TIMER_DEVICE_ID);
	XScuTimer_CfgInitialize(TimerInstancePtr, TimerConfigPtr,TimerConfigPtr->BaseAddr);
	XScuTimer_LoadTimer(TimerInstancePtr, TIMER_LOAD_VALUE);
}

int timerSet_S(){
//---------------------------------------
	int status = 0;
	int count = 0;
	int keep_timer = 0;
//---------------------------------------
	while(1) {
		keep_timer = XScuTimer_GetCounterValue(TimerInstancePtr);
		if(keep_timer == 0){
			XScuTimer_RestartTimer(TimerInstancePtr);
			xil_printf("计时完成: %d   \r\n", count++);
			if(count==2) {
				return status;
			}
		}else{
			xil_printf("Timer is still running (Timer value = %d)\r\n", keep_timer );
		}
	}
	return status;
}
