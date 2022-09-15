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
void initTimer();
void calculate(u32 offset);
int timerSet_S();
u8 checkCache(u8 count);

//----------------------GPIO Buffer
u8 buffer[12];
//数据缓存
int cache[CACHE_NUMBER];
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
	u32 offset = 0;

	memset(cache,(u32)0x00,sizeof(cache));
	XScuTimer_Start(TimerInstancePtr);
	while(1){
		memset(buffer,0x00,sizeof(buffer));
		read_gpio(buffer);
		int result = 0;
		for(int i = 0;i < 12;i++){
			result += (buffer[11-i] & 0x0F) << i;
		}

		if((count > 0 && abs(cache[count-1] - result) > 400) || abs(result - 2048) > 1100 || result < 2048){
			continue;
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

		cache[count] = result;
		count++;
		usleep(10);

//		if(count == CACHE_NUMBER){
//			for(int i = 0;i < CACHE_NUMBER;i++){
//				printf("%d, ",cache[i]);
//			}
////			union SplitIntData result[50];
////			U32ToSplitIntData(cache,result,50);
////
////			u32 startOffset = offset * CACHE_NUMBER / DATA_COUNT;
////			for(int i = 0;i < CACHE_NUMBER / DATA_COUNT;i++){
////				WirteFlashData(result, (startOffset + i));
////			}
////			printf("Write %d %d\r\n",startOffset,offset);
////			free(result);
//
////			if(offset > 0 && offset % (CALCULATE_NUMBER / CACHE_NUMBER) == 0){
////				calculate(offset);
//////				offset = 0;
////			}
//
//			memset(cache,(u32)0x00,sizeof(cache));
//			count = 0;
////			offset++;
//		}

		if(count == 200){
			for(int i = 0;i < 200;i++){
				printf("%d, ",cache[i]);
			}
			printf("\r\n");

			memset(cache,(u32)0x00,sizeof(cache));
			count = 0;
			offset++;
		}
//		usleep(1000);

	}
	return 0;
}

void calculate(u32 offset){
	int start = offset - (CALCULATE_NUMBER/CACHE_NUMBER);
	if(start < 0){
		return;
	}

	union SplitIntData result[50];
//	printf("Read %d %d\r\n",start,(CACHE_NUMBER / CALCULATE_NUMBER) * (CACHE_NUMBER / DATA_COUNT));
	for(int i = 0;i < (CACHE_NUMBER / CALCULATE_NUMBER) * (CACHE_NUMBER / DATA_COUNT);i++){
		memset(result,(u32)0x00,sizeof(result));
		ReadFlashData(result, (start + i));
//		for(int i = 0;i < 50;i++){
//			printf("%d, ",result[i].data.var);
//		}
	}
//	FlashErase(&QspiInstance, START_ADDRESS, MAX_DATA);
}

u8 checkCache(u8 count){
	if(count > 60){
		for(u8 i = count-60;i < count;i++){
			if(abs(cache[i] - 2048) > 25){
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
