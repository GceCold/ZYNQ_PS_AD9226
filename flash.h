/*
 * flash.h
 *
 *  Created on: 2022Äê9ÔÂ13ÈÕ
 *      Author: IceCold
 */

#ifndef FLASH_H
#define FLASH_H

#define QSPI_DEVICE_ID		XPAR_XQSPIPS_0_DEVICE_ID

#define WRITE_STATUS_CMD	0x01
#define WRITE_CMD		0x02
#define READ_CMD		0x03
#define WRITE_DISABLE_CMD	0x04
#define READ_STATUS_CMD		0x05
#define WRITE_ENABLE_CMD	0x06
#define FAST_READ_CMD		0x0B
#define DUAL_READ_CMD		0x3B
#define QUAD_READ_CMD		0x6B
#define BULK_ERASE_CMD		0xC7
#define	SEC_ERASE_CMD		0xD8
#define READ_ID			0x9F

#define COMMAND_OFFSET		0
#define ADDRESS_1_OFFSET	1
#define ADDRESS_2_OFFSET	2
#define ADDRESS_3_OFFSET	3
#define DATA_OFFSET			4
#define DUMMY_SIZE		1

#define OVERHEAD_SIZE		4

#define SECTOR_SIZE		0x10000
#define NUM_SECTORS		0x100
#define NUM_PAGES		0x10000
#define PAGE_SIZE		256

#define RD_ID_SIZE			4
#define BULK_ERASE_SIZE		1
#define SEC_ERASE_SIZE		4

#define START_ADDRESS		0x00055000
#define UNIQUE_VALUE		0x00

#define DATA_COUNT 			50

#define MAX_DATA			DATA_COUNT*4


union SplitIntData {
	struct {
		u32 var;
	} data;
	struct {
		u8 data1;
		u8 data2;
		u8 data3;
		u8 data4;
	} split;
};

u8 WriteBuffer[MAX_DATA + DATA_OFFSET];
u8 ReadBuffer[MAX_DATA + DATA_OFFSET + DUMMY_SIZE];
XQspiPs QspiInstance;

void InitQspi();
void FlashErase(XQspiPs *QspiPtr, u32 Address, u32 ByteCount);
void FlashWrite(XQspiPs *QspiPtr, u32 Address, u32 ByteCount, u8 Command);
void FlashRead(XQspiPs *QspiPtr, u32 Address, u32 ByteCount, u8 Command);
void FlashReadID(void);
void FlashQuadEnable(XQspiPs *QspiPtr);
void kfft(float *pr, float *pi, int n, int k, float *fr, float *fi, float *amp);

void WirteFlashData(union SplitIntData *data, int offset);
void ReadFlashData(int offset, union SplitIntData *data);
void WriteAllData(union SplitIntData *data, int offset);
void ReadAllData(union SplitIntData *data, int offset);

void U32ToSplitIntData(u32 *data, union SplitIntData *result, u8 count){
	memcpy(result, data, count * 4);
}

void WriteAllData(union SplitIntData *data, int offset) {
	union SplitIntData allData[50];
	memcpy(&allData, data, 50 * 4);
	WirteFlashData(allData, offset * 2);

	memcpy(&allData, data + 50, 50 * 4);
	WirteFlashData(allData, offset * 2 + 1);
}

void ReadAllData(union SplitIntData *data, int offset) {
	ReadFlashData(offset * 2, data);
	ReadFlashData(offset * 2 + 1, data + 50);
}

void WirteFlashData(union SplitIntData *data, int offset) {
	int i;
	for (i = 0; i < DATA_COUNT; i++) {
		WriteBuffer[DATA_OFFSET + i * 5] = data[i].split.data1;
		WriteBuffer[DATA_OFFSET + i * 5 + 1] = data[i].split.data2;
		WriteBuffer[DATA_OFFSET + i * 5 + 2] = data[i].split.data3;
		WriteBuffer[DATA_OFFSET + i * 5 + 3] = data[i].split.data4;
	}
	FlashWrite(&QspiInstance, offset * MAX_DATA + START_ADDRESS,
	MAX_DATA,
	WRITE_CMD);
}

void ReadFlashData(int offset, union SplitIntData *data) {
	FlashRead(&QspiInstance, offset * MAX_DATA + START_ADDRESS, MAX_DATA,
	QUAD_READ_CMD);
	memcpy(data, &ReadBuffer[DATA_OFFSET + DUMMY_SIZE], MAX_DATA);
}

void InitQspi() {
	XQspiPs_Config *QspiConfig;
	QspiConfig = XQspiPs_LookupConfig(QSPI_DEVICE_ID);
	XQspiPs_CfgInitialize(&QspiInstance, QspiConfig, QspiConfig->BaseAddress);

	memset(ReadBuffer, 0x00, sizeof(ReadBuffer));
	memset(WriteBuffer, 0x00, sizeof(WriteBuffer));

	XQspiPs_SetOptions(&QspiInstance, XQSPIPS_MANUAL_START_OPTION |
	XQSPIPS_FORCE_SSELECT_OPTION |
	XQSPIPS_HOLD_B_DRIVE_OPTION);

	XQspiPs_SetClkPrescaler(&QspiInstance, XQSPIPS_CLK_PRESCALE_8);

	XQspiPs_SetSlaveSelect(&QspiInstance);

	FlashReadID();

	FlashQuadEnable(&QspiInstance);

	FlashErase(&QspiInstance, START_ADDRESS, MAX_DATA * 2);
}

void FlashReadID(void) {
	WriteBuffer[COMMAND_OFFSET] = READ_ID;
	WriteBuffer[ADDRESS_1_OFFSET] = 0x23;
	WriteBuffer[ADDRESS_2_OFFSET] = 0x08;
	WriteBuffer[ADDRESS_3_OFFSET] = 0x09;

	XQspiPs_PolledTransfer(&QspiInstance, WriteBuffer, ReadBuffer,
	RD_ID_SIZE);
}

void FlashQuadEnable(XQspiPs *QspiPtr) {
	u8 WriteEnableCmd = { WRITE_ENABLE_CMD };
	u8 ReadStatusCmd[] = { READ_STATUS_CMD, 0 };
	u8 QuadEnableCmd[] = { WRITE_STATUS_CMD, 0 };
	u8 FlashStatus[2];

	if (ReadBuffer[1] == 0x9D) {

		XQspiPs_PolledTransfer(QspiPtr, ReadStatusCmd, FlashStatus,
				sizeof(ReadStatusCmd));

		QuadEnableCmd[1] = FlashStatus[1] | 1 << 6;

		XQspiPs_PolledTransfer(QspiPtr, &WriteEnableCmd, NULL,
				sizeof(WriteEnableCmd));

		XQspiPs_PolledTransfer(QspiPtr, QuadEnableCmd, NULL,
				sizeof(QuadEnableCmd));
	}
}

void FlashErase(XQspiPs *QspiPtr, u32 Address, u32 ByteCount) {
	u8 WriteEnableCmd = { WRITE_ENABLE_CMD };
	u8 ReadStatusCmd[] = { READ_STATUS_CMD, 0 };
	u8 FlashStatus[2];
	int Sector;

	if (ByteCount == (NUM_SECTORS * SECTOR_SIZE)) {
		XQspiPs_PolledTransfer(QspiPtr, &WriteEnableCmd, NULL,
				sizeof(WriteEnableCmd));
		WriteBuffer[COMMAND_OFFSET] = BULK_ERASE_CMD;
		XQspiPs_PolledTransfer(QspiPtr, WriteBuffer, NULL,
		BULK_ERASE_SIZE);

		while (1) {
			XQspiPs_PolledTransfer(QspiPtr, ReadStatusCmd, FlashStatus,
					sizeof(ReadStatusCmd));

			if ((FlashStatus[1] & 0x01) == 0) {
				break;
			}
		}

		return;
	}
	for (Sector = 0; Sector < ((ByteCount / SECTOR_SIZE) + 1); Sector++) {
		XQspiPs_PolledTransfer(QspiPtr, &WriteEnableCmd, NULL,
				sizeof(WriteEnableCmd));

		WriteBuffer[COMMAND_OFFSET] = SEC_ERASE_CMD;
		WriteBuffer[ADDRESS_1_OFFSET] = (u8) (Address >> 16);
		WriteBuffer[ADDRESS_2_OFFSET] = (u8) (Address >> 8);
		WriteBuffer[ADDRESS_3_OFFSET] = (u8) (Address & 0xFF);

		XQspiPs_PolledTransfer(QspiPtr, WriteBuffer, NULL,
		SEC_ERASE_SIZE);

		while (1) {
			XQspiPs_PolledTransfer(QspiPtr, ReadStatusCmd, FlashStatus,
					sizeof(ReadStatusCmd));
			if ((FlashStatus[1] & 0x01) == 0) {
				break;
			}
		}

		Address += SECTOR_SIZE;
	}
}

void FlashWrite(XQspiPs *QspiPtr, u32 Address, u32 ByteCount, u8 Command) {
	u8 WriteEnableCmd = { WRITE_ENABLE_CMD };
	u8 ReadStatusCmd[] = { READ_STATUS_CMD, 0 };
	u8 FlashStatus[2];
	XQspiPs_PolledTransfer(QspiPtr, &WriteEnableCmd, NULL,
			sizeof(WriteEnableCmd));
	WriteBuffer[COMMAND_OFFSET] = Command;
	WriteBuffer[ADDRESS_1_OFFSET] = (u8) ((Address & 0xFF0000) >> 16);
	WriteBuffer[ADDRESS_2_OFFSET] = (u8) ((Address & 0xFF00) >> 8);
	WriteBuffer[ADDRESS_3_OFFSET] = (u8) (Address & 0xFF);

	XQspiPs_PolledTransfer(QspiPtr, WriteBuffer, NULL,
			ByteCount + OVERHEAD_SIZE);
	while (1) {
		XQspiPs_PolledTransfer(QspiPtr, ReadStatusCmd, FlashStatus,
				sizeof(ReadStatusCmd));
		if ((FlashStatus[1] & 0x01) == 0) {
			break;
		}
	}
}

void FlashRead(XQspiPs *QspiPtr, u32 Address, u32 ByteCount, u8 Command) {
	WriteBuffer[COMMAND_OFFSET] = Command;
	WriteBuffer[ADDRESS_1_OFFSET] = (u8) ((Address & 0xFF0000) >> 16);
	WriteBuffer[ADDRESS_2_OFFSET] = (u8) ((Address & 0xFF00) >> 8);
	WriteBuffer[ADDRESS_3_OFFSET] = (u8) (Address & 0xFF);

	if ((Command == FAST_READ_CMD) || (Command == DUAL_READ_CMD)
			|| (Command == QUAD_READ_CMD)) {
		ByteCount += DUMMY_SIZE;
	}
	XQspiPs_PolledTransfer(QspiPtr, WriteBuffer, ReadBuffer,
			ByteCount + OVERHEAD_SIZE);
}

#endif /* SRC_FLASH_H_ */
