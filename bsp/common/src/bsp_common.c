/*
 * Copyright (c) 2022 OpenLuat & AirM2M
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "bsp_common.h"

const uint8_t ByteToAsciiTable[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

void LoopBuffer_Init(Loop_Buffer *Buf, void *Src, uint32_t MaxLen, uint32_t DataSize)
{
	uint8_t *Data = (uint8_t *)Src;
	Buf->Data = Data;
	Buf->Len = 0;
	Buf->MaxLength = MaxLen;
	Buf->Offset = 0;
	Buf->DataSize = DataSize;
}

uint32_t LoopBuffer_Query(Loop_Buffer *Buf, void *Src, uint32_t Len)
{
	uint32_t i, p;
	uint8_t *Data = (uint8_t *)Src;
	if (Buf->Len < Len)
	{
		Len = Buf->Len;
	}
	if (Buf->DataSize > 1)
	{
		for (i = 0, p = Buf->Offset; i < Len; i++, p++)
		{
			if (p >= Buf->MaxLength)
			{
				p -= Buf->MaxLength;
			}
			memcpy(Data + (i * Buf->DataSize), Buf->Data + (p * Buf->DataSize), Buf->DataSize);
		}
	}
	else
	{
		for (i = 0, p = Buf->Offset; i < Len; i++, p++)
		{
			if (p >= Buf->MaxLength)
			{
				p -= Buf->MaxLength;
			}
			Data[i] = Buf->Data[p];
		}
	}

	return Len;
}

uint32_t LoopBuffer_Read(Loop_Buffer *Buf, void *Src, uint32_t Len)
{
	uint32_t l;
	uint8_t *Data = (uint8_t *)Src;
	l = LoopBuffer_Query(Buf, Data, Len);
	Buf->Len -= l;
	Buf->Offset += l;
	if (Buf->Offset >= Buf->MaxLength)
	{
		Buf->Offset -= Buf->MaxLength;

	}
	if (!Buf->Len) {
		Buf->Offset = 0;
	}
	return l;
}

void LoopBuffer_Del(Loop_Buffer *Buf, uint32_t Len)
{
	if (Buf->Len < Len)
	{
		Len = Buf->Len;
	}

	Buf->Len -= Len;
	Buf->Offset += Len;
	if (Buf->Offset >= Buf->MaxLength)
	{
		Buf->Offset -= Buf->MaxLength;
	}

	if (!Buf->Len) {
		Buf->Offset = 0;
	}
}

uint32_t LoopBuffer_Write(Loop_Buffer *Buf, void *Src, uint32_t Len)
{
	uint32_t i, p, cut_off = 0;
	uint8_t *Data = (uint8_t *)Src;
	if (!Buf->Len && !Buf->Offset && (Len <= Buf->Len))
	{
		memcpy(Buf->Data, Data, Len);
		Buf->Len = Len;
		return Len;
	}
	cut_off = Buf->MaxLength - Buf->Len;
	if (cut_off >= Len)
	{
		cut_off = 0;
	}
	else
	{
		cut_off = Len - cut_off;
	}

	if (Buf->DataSize > 1)
	{
		for (i = 0, p = Buf->Offset + Buf->Len; i < Len; i++, p++)
		{
			if (p >= Buf->MaxLength)
			{
				p -= Buf->MaxLength;
			}
			memcpy(Buf->Data + (p * Buf->DataSize), Data + (i * Buf->DataSize), Buf->DataSize);
		}
	}
	else
	{
		for (i = 0, p = Buf->Offset + Buf->Len; i < Len; i++, p++)
		{
			if (p >= Buf->MaxLength)
			{
				p -= Buf->MaxLength;
			}

			Buf->Data[p] = Data[i];
		}
	}


	Buf->Offset += cut_off;
	if (Buf->Offset >= Buf->MaxLength)
		Buf->Offset -= Buf->MaxLength;

	Buf->Len += Len;
	if (Buf->Len > Buf->MaxLength)
		Buf->Len = Buf->MaxLength;

	return Len;

}


void Buffer_StaticInit(Buffer_Struct *Buf, void *Src, uint32_t MaxLen)
{
	Buf->Data = Src;
	Buf->Pos = 0;
	Buf->MaxLen = MaxLen;
}

int32_t Buffer_StaticWrite(Buffer_Struct *Buf, void *Data, uint32_t Len)
{
	if (!Len)
	{
		return -1;
	}
	if (!Buf)
	{
		return -1;
	}
	if ((Buf->Pos + Len) > Buf->MaxLen)
	{
		Len = Buf->MaxLen - Buf->Pos;
	}
	if (Len)
	{
		memcpy(&Buf->Data[Buf->Pos], Data, Len);
	}
	Buf->Pos += Len;
	return Len;
}

void DBuffer_Init(DBuffer_Struct *DBuf, uint32_t Size)
{
	memset(DBuf, 0, sizeof(DBuffer_Struct));
	DBuf->pCache[0] = malloc(Size);
	DBuf->pCache[1] = malloc(Size);
	DBuf->MaxLen = Size;
}

void DBuffer_ReInit(DBuffer_Struct *DBuf, uint32_t Size)
{
	if (DBuf->pCache[0]) free(DBuf->pCache[0]);
	if (DBuf->pCache[1]) free(DBuf->pCache[1]);
	DBuffer_Init(DBuf, Size);
}

void DBuffer_DeInit(DBuffer_Struct *DBuf)
{
	free(DBuf->pCache[0]);
	free(DBuf->pCache[1]);
	DBuf->pCache[0] = NULL;
	DBuf->pCache[1] = NULL;
	DBuf->MaxLen = 0;
}

void *DBuffer_GetCache(DBuffer_Struct *DBuf, uint8_t IsCurrent)
{
	return DBuf->pCache[IsCurrent?DBuf->CurCacheSn:!DBuf->CurCacheSn];
}

void DBuffer_SwapCache(DBuffer_Struct *DBuf)
{
	DBuf->CurCacheSn = !DBuf->CurCacheSn;
}

void DBuffer_SetDataLen(DBuffer_Struct *DBuf, uint32_t Len, uint8_t IsCurrent)
{
	DBuf->pCacheLen[IsCurrent?DBuf->CurCacheSn:!DBuf->CurCacheSn] = Len;
}

uint32_t DBuffer_GetDataLen(DBuffer_Struct *DBuf, uint8_t IsCurrent)
{
	return DBuf->pCacheLen[IsCurrent?DBuf->CurCacheSn:!DBuf->CurCacheSn];
}

//void Buffer_Remove(Buffer_Struct *Buf, uint32_t Len)
//{
//	uint32_t RestLen;
//	uint32_t i;
//	if (!Buf)
//		return ;
//	if (!Buf->Data)
//		return ;
//	if (Len >= Buf->Pos)
//	{
//		Buf->Pos = 0;
//		return ;
//	}
//	RestLen = Buf->Pos - Len;
//	memmove(Buf->Data, Buf->Data + Len, RestLen);
//	Buf->Pos = RestLen;
//}


/*****************************************************************************
* FUNCTION
*   command_parse_param()
* DESCRIPTION
*    Parse AT command string to parameters
* PARAMETERS
*   char* pStr
* RETURNS
*  pCmdParam
*****************************************************************************/
uint32_t CmdParseParam(int8_t* pStr, CmdParam *CP, int8_t Cut)
{
	uint32_t paramStrLen = strlen((char *)pStr);
	uint32_t paramIndex = 0;
	uint32_t paramCharIndex = 0;
	uint32_t index = 0;

	while ((pStr[index] != '\r')
		&& (index < paramStrLen)
		&& (paramIndex < CP->param_max_num)) {
		if (pStr[index] == Cut) {
			/* Next param string */
			paramCharIndex = 0;
			paramIndex++;
		}
		else {
			if (pStr[index] != '"')
			{
				if (paramCharIndex >= CP->param_max_len)
					return (0);

				/*Get each of command param char, the param char except char ' " '*/
				CP->param_str[paramIndex * CP->param_max_len + paramCharIndex] = pStr[index];
				paramCharIndex++;
			}
		}
		index++;
	}

	CP->param_num = paramIndex + 1;

	return (1);
}

__attribute__((weak)) uint8_t OS_CheckInIrq(void)
{
	return __get_IPSR();
}

#include "bget.h"
#ifdef __BUILD_OS__
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
HANDLE OS_MutexCreate(void)
{
	return xSemaphoreCreateBinary();
}

HANDLE OS_MutexCreateUnlock(void)
{
	HANDLE Sem = xSemaphoreCreateBinary();
	xSemaphoreGive(Sem);
	return Sem;
}

void OS_MutexLock(HANDLE Sem)
{
	uint8_t suspend = !OS_IsSchedulerRun();
	if (suspend)
	{
		xTaskResumeAll();
	}
	xSemaphoreTake(Sem, portMAX_DELAY);
	if (suspend)
	{
		vTaskSuspendAll();
	}
}

int32_t OS_MutexLockWtihTime(HANDLE Sem, uint32_t TimeoutMs)
{
	uint8_t suspend = !OS_IsSchedulerRun();
	if (suspend)
	{
		xTaskResumeAll();
	}
	int result = xSemaphoreTake(Sem, TimeoutMs);
	if (suspend)
	{
		vTaskSuspendAll();
	}
	if (pdTRUE != result)
	{
		return -ERROR_OPERATION_FAILED;
	}
	else
	{
		return ERROR_NONE;
	}
}

void OS_MutexRelease(HANDLE Sem)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
//	xSemaphoreGiveFromISR(Sem, &xHigherPriorityTaskWoken);
	if (OS_CheckInIrq())
	{
		xSemaphoreGiveFromISR(Sem, &xHigherPriorityTaskWoken);
		if (xHigherPriorityTaskWoken)
		{
			portYIELD_WITHIN_API();
		}
	}
	else
	{
		xSemaphoreGive(Sem);
	}
}

void OS_MutexDelete(HANDLE Sem)
{
	vSemaphoreDelete(Sem);
}

uint8_t OS_IsSchedulerRun(void)
{
	return (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING);
}

void OS_SuspendTask(HANDLE taskHandle)
{
	if (taskHandle)
	{
		vTaskSuspend(taskHandle);
	}
	else
	{
		vTaskSuspendAll();
	}
}

void OS_ResumeTask(HANDLE taskHandle)
{
	if (taskHandle)
	{
		vTaskResume(taskHandle);
	}
	else
	{
		xTaskResumeAll();
	}
}
#endif
static uint8_t prvOSRunFlag;
extern const uint32_t __os_heap_start;
extern const uint32_t __ram_end;
__attribute__((weak)) void OS_SetStartFlag(void)
{
	prvOSRunFlag = 1;
}
__attribute__((weak)) uint32_t OS_EnterCritical(void)
{
#ifdef __BUILD_OS__
	if (prvOSRunFlag)
	{
		if (__get_IPSR())
		{
			return taskENTER_CRITICAL_FROM_ISR();
		}
		else
		{
			taskENTER_CRITICAL();
			return 0;
		}
	}
	else
#endif
	{
		__disable_irq();
	}
}
__attribute__((weak)) void OS_ExitCritical(uint32_t Critical)
{
#ifdef __BUILD_OS__
	if (prvOSRunFlag)
	{
		if (__get_IPSR())
		{
			taskEXIT_CRITICAL_FROM_ISR(Critical);
		}
		else
		{
			taskEXIT_CRITICAL();
		}
	}
	else
#endif
	{
		__enable_irq();
	}
}

__attribute__((weak)) void *OS_Malloc(uint32_t Size)
{
	void *p;
	uint32_t Critical = OS_EnterCritical();
	p = bget(Size);
	OS_ExitCritical(Critical);
	return p;
}

__attribute__((weak)) void *OS_Zalloc(uint32_t Size)
{
	void *p;
	uint32_t Critical = OS_EnterCritical();
	p = bgetz(Size);
	OS_ExitCritical(Critical);
	return p;
}

__attribute__((weak)) void *OS_Calloc(uint32_t count, uint32_t eltsize)
{
	void *p;
	uint32_t Critical = OS_EnterCritical();
	p = bgetz(count * eltsize);
	OS_ExitCritical(Critical);
	return p;
}

__attribute__((weak)) void OS_Free(void *p)
{
	if (((uint32_t)p >= (uint32_t)(&__os_heap_start)) && ((uint32_t)p <= (uint32_t)(&__ram_end)))
	{
		uint32_t Critical = OS_EnterCritical();
		brel(p);
		OS_ExitCritical(Critical);
	}

}

__attribute__((weak)) void *OS_Realloc(void *buf, uint32_t size)
{
	void *p;
	uint32_t Critical = OS_EnterCritical();
//	p = bget(size);
//	memcpy(p, buf, size);
//	brel(buf);
	p = bgetr(buf, size);
	OS_ExitCritical(Critical);
	return p;
}

__attribute__((weak)) void OS_MemInfo(uint32_t *curalloc, uint32_t *totfree, uint32_t *maxfree)
{
	unsigned long  nget, nrel;
	uint32_t Critical = OS_EnterCritical();
	bstats(curalloc, totfree, maxfree, &nget, &nrel);
	OS_ExitCritical(Critical);
}

__attribute__((weak)) int32_t OS_InitBuffer(Buffer_Struct *Buf, uint32_t Size)
{
	if (!Buf)
		return 0;
	Buf->Data = OS_Zalloc(Size);
	if (!Buf->Data)
	{
		Buf->MaxLen = 0;
		Buf->Pos = 0;
		return 0;
	}
	Buf->MaxLen = Size;
	Buf->Pos = 0;
	return Size;
}

__attribute__((weak)) void OS_DeInitBuffer(Buffer_Struct *Buf)
{
	if (Buf->Data)
	{
		OS_Free(Buf->Data);
	}
	Buf->Data = NULL;
	Buf->MaxLen = 0;
	Buf->Pos = 0;
}

__attribute__((weak)) int32_t OS_ReInitBuffer(Buffer_Struct *Buf, uint32_t Size)
{
	if (!Buf)
		return 0;

	if (Buf->Data)
	{
		OS_Free(Buf->Data);
	}
	Buf->Data = OS_Zalloc(Size);
	if (!Buf->Data)
	{
		Buf->MaxLen = 0;
		Buf->Pos = 0;
		return 0;
	}
	Buf->MaxLen = Size;
	Buf->Pos = 0;
	return Size;
}

__attribute__((weak)) int32_t OS_ReSizeBuffer(Buffer_Struct *Buf, uint32_t Size)
{
//	uint8_t *Old;
	uint8_t *New;

	if (!Buf)
		return 0;

//	Old = Buf->Data;
//	if (Size < Buf->Pos)
//	{
//		Size = Buf->Pos;
//	}
//	New = OS_Zalloc(Size);
//	if (!New)
//	{
//		return 0;
//	}
//	if (Old)
//	{
//		memcpy(New, Old, Buf->Pos);
//		OS_Free(Old);
//	}
	uint32_t Critical = OS_EnterCritical();
	New = bgetr(Buf->Data, Size);
	if (New)
	{
		Buf->Data = New;
		Buf->MaxLen = Size;
	}
	OS_ExitCritical(Critical);


	return Size;
}

__attribute__((weak)) int32_t OS_BufferWrite(Buffer_Struct *Buf, void *Data, uint32_t Len)
{
	uint32_t WriteLen;
	if (!Len)
	{
		return ERROR_NONE;
	}
	if (!Buf)
	{
		return -ERROR_PARAM_INVALID;
	}
	if (!Buf->Data)
	{
		Buf->Data = OS_Zalloc(Len);
		if (!Buf->Data)
		{
			return -ERROR_NO_MEMORY;
		}
		Buf->Pos = 0;
		Buf->MaxLen = Len;
	}
	WriteLen = Buf->Pos + Len;
	if (WriteLen > Buf->MaxLen)
	{
		if (!OS_ReSizeBuffer(Buf, WriteLen))
		{
			return -ERROR_NO_MEMORY;
		}
	}
	memcpy(&Buf->Data[Buf->Pos], Data, Len);
	Buf->Pos += Len;
	return ERROR_NONE;
}

__attribute__((weak)) int32_t OS_BufferWriteLimit(Buffer_Struct *Buf, void *Data, uint32_t Len)
{
	uint32_t WriteLen;
	if (!Len)
	{
		return ERROR_NONE;
	}
	if (!Buf)
	{
		return -ERROR_PARAM_INVALID;
	}
	if (!Buf->Data)
	{
		Buf->Data = OS_Zalloc(Len);
		if (!Buf->Data)
		{
			return -ERROR_NO_MEMORY;
		}
		Buf->Pos = 0;
		Buf->MaxLen = Len;
	}
	WriteLen = Buf->Pos + Len;
	if (WriteLen > Buf->MaxLen)
	{
		return -ERROR_NO_MEMORY;
	}
	memcpy(&Buf->Data[Buf->Pos], Data, Len);
	Buf->Pos += Len;
	return ERROR_NONE;
}

__attribute__((weak)) void OS_BufferRemove(Buffer_Struct *Buf, uint32_t Len)
{
	uint32_t RestLen;
	uint32_t i;
	if (!Buf)
		return ;
	if (!Buf->Data)
		return ;
	if (Len >= Buf->Pos)
	{
		Buf->Pos = 0;
		return ;
	}
	RestLen = Buf->Pos - Len;
	memmove(Buf->Data, Buf->Data + Len, RestLen);
	Buf->Pos = RestLen;
}



int32_t BSP_SetBit(uint8_t *Data, uint32_t Sn, uint8_t Value)
{
	uint32_t Mask,Pos1,Pos2;

	Pos1 = Sn/8;
	Pos2 = Sn%8;

	Mask = ~(1 << Pos2);
	if (Value)
	{
		Value = (1 << Pos2);
	}
	Data[Pos1] = (Data[Pos1] & Mask) | Value;
	//DBG("%d %d %d %d", Sn, Pos1, Pos2, Value);
	return 0;
}

int32_t BSP_GetBit(uint8_t *Data, uint32_t Sn, uint8_t *Value)
{
	uint32_t Mask,Pos1,Pos2;

	Pos1 = Sn/8;
	Pos2 = Sn%8;
	Mask = (1 << Pos2);
	if (Data[Pos1] & Mask)
	{
		*Value = 1;
	}
	else
	{
		*Value = 0;
	}
	return -1;
}

uint8_t BSP_TestBit(uint8_t *Data, uint32_t Sn)
{
	uint32_t Mask,Pos1,Pos2;

	Pos1 = Sn/8;
	Pos2 = Sn%8;
	Mask = (1 << Pos2);
	if (Data[Pos1] & Mask)
	{
		return 1;
	}
	return 0;
}

uint8_t XorCheck(void *Src, uint32_t Len, uint8_t CheckStart)
{
	uint8_t Check = CheckStart;
	uint8_t *Data = (uint8_t *)Src;
	uint32_t i;
	for (i = 0; i < Len; i++)
	{
		Check ^= Data[i];
	}
	return Check;
}

uint8_t SumCheck(uint8_t *Data, uint32_t Len)
{
	uint8_t Check = 0;
	uint32_t i;
	for (i = 0; i < Len; i++)
	{
		Check += Data[i];
	}
	return Check;
}


uint8_t CRC8Cal(void *Data, uint16_t Len, uint8_t CRC8Last, uint8_t CRCRoot, uint8_t IsReverse)
{
	uint16_t i;
	uint8_t CRC8 = CRC8Last;
	uint8_t wTemp = CRCRoot;
	uint8_t *Src = (uint8_t *)Data;
	if (IsReverse)
	{
		CRCRoot = 0;
		for (i = 0; i < 8; i++)
		{
			if (wTemp & (1 << (7 - i)))
			{
				CRCRoot |= 1 << i;
			}
		}
		while (Len--)
		{

			CRC8 ^= *Src++;
			for (i = 0; i < 8; i++)
			{
				if ((CRC8 & 0x01))
				{
					CRC8 >>= 1;
					CRC8 ^= CRCRoot;
				}
				else
				{
					CRC8 >>= 1;
				}
			}
		}
	}
	else
	{
		while (Len--)
		{

			CRC8 ^= *Src++;
			for (i = 8; i > 0; --i)
			{
				if ((CRC8 & 0x80))
				{
					CRC8 <<= 1;
					CRC8 ^= CRCRoot;
				}
				else
				{
					CRC8 <<= 1;
				}
			}
		}
	}
	return CRC8;
}

/************************************************************************/
/*  CRC16                                                                */
/************************************************************************/
uint16_t CRC16Cal(void *Data, uint16_t Len, uint16_t CRC16Last, uint16_t CRCRoot, uint8_t IsReverse)
{
	uint16_t i;
	uint16_t CRC16 = CRC16Last;
	uint16_t wTemp = CRCRoot;
	uint8_t *Src = (uint8_t *)Data;
	if (IsReverse)
	{
		CRCRoot = 0;
		for (i = 0; i < 16; i++)
		{
			if (wTemp & (1 << (15 - i)))
			{
				CRCRoot |= 1 << i;
			}
		}
		while (Len--)
		{
			for (i = 0; i < 8; i++)
			{
				if ((CRC16 & 0x0001) != 0)
				{
					CRC16 >>= 1;
					CRC16 ^= CRCRoot;
				}
				else
				{
					CRC16 >>= 1;
				}
				if ((*Src&(1 << i)) != 0)
				{
					CRC16 ^= CRCRoot;
				}
			}
			Src++;
		}
	}
	else
	{
		while (Len--)
		{
			for (i = 8; i > 0; i--)
			{
				if ((CRC16 & 0x8000) != 0)
				{
					CRC16 <<= 1;
					CRC16 ^= CRCRoot;
				}
				else
				{
					CRC16 <<= 1;
				}
				if ((*Src&(1 << (i - 1))) != 0)
				{
					CRC16 ^= CRCRoot;
				}
			}
			Src++;
		}
	}

	return CRC16;
}

uint32_t AsciiToU32(uint8_t *Src, uint32_t Len)
{
	uint32_t i = 0;
	uint32_t Temp = 0;
	for (i = 0; i < Len; i++)
	{

		if (Src[i])
		{
			Temp *= 10;
			Temp += Src[i] - '0';
		}
		else
		{
			break;
		}
	}
	return Temp;
}


/**
* @brief  反转数据
* @param  ref 需要反转的变量
* @param	ch 反转长度，多少位
* @retval N反转后的数据
*/
static LongInt Reflect(LongInt ref, uint8_t ch)
{
	LongInt value = 0;
	LongInt i;
	for (i = 1; i < (LongInt)(ch + 1); i++)
	{
		if (ref & 1)
			value |= (LongInt)1 << (ch - i);
		ref >>= 1;
	}
	return value;
}

/**
* @brief  建立CRC32的查询表
* @param  Tab 表缓冲
* @param	Gen CRC32根
* @retval None
*/
void CRC32_CreateTable(uint32_t *Tab, uint32_t Gen)
{
	uint32_t crc;
	uint32_t i, j, temp, t1, t2, flag;
	if (Tab[1] != 0)
		return;
	for (i = 0; i < 256; i++)
	{
		temp = Reflect(i, 8);
		Tab[i] = temp << 24;
		for (j = 0; j < 8; j++)
		{
			flag = Tab[i] & 0x80000000;
			t1 = Tab[i] << 1;
			if (0 == flag)
			{
				t2 = 0;
			}
			else
			{
				t2 = Gen;
			}
			Tab[i] = t1 ^ t2;
		}
		crc = Tab[i];
		Tab[i] = Reflect(crc, 32);
	}
}


/**
* @brief  计算buffer的crc校验码
* @param  CRC32_Table CRC32表
* @param  Buf 缓冲
* @param	Size 缓冲区长度
* @param	CRC32 初始CRC32值
* @retval 计算后的CRC32
*/
uint32_t CRC32_Cal(uint32_t *CRC32_Table, uint8_t *Buf, uint32_t Size, uint32_t CRC32Last)
{
	uint32_t i;
	for (i = 0; i < Size; i++)
	{
		CRC32Last = CRC32_Table[(CRC32Last ^ Buf[i]) & 0xff] ^ (CRC32Last >> 8);
	}
	return CRC32Last;
}


/************************************************************************/
/*时间与时间戳转换，C语言实现                                                                    */
/************************************************************************/
/************************************************************************/
uint8_t IsLeapYear(uint32_t Year)
{
	if ((Year % 400) == 0)
		return 1;
	if ((((Year % 4) == 0) && (Year % 100) != 0))
		return 1;
	else
		return 0;
}

const uint32_t DayTable[2][12] = { { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 }, { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 } };
//const uint32_t DayTable[2][12] = { { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 }, { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 } };
LongInt UTC2Tamp(Date_UserDataStruct *Date, Time_UserDataStruct *Time)
{

	LongInt DYear, DDay, DSec;
	uint32_t Year100;
	DYear = Date->Year - 1970;
	if (DYear)	//1970年以后,1972是第一个闰年,1973年是第一年需要增加一天，2100年是非闰年
	{
		DDay = DYear * 365 + ((DYear + 1) / 4) + DayTable[IsLeapYear(Date->Year)][Date->Mon - 1] + (Date->Day - 1);
//		if (IsLeapYear(Date->Year))
//		{
//			DDay--;
//		}
		if (Date->Year >= 2100)
		{
			Year100 = Date->Year - 2100;
			DDay -= (1 + Year100 / 100);
			if (Date->Year >= 2400)
			{
				Year100 = Date->Year - 2400;
				DDay += 1 + Year100 / 400;
			}

		}

	}
	else
	{
		DDay = DayTable[IsLeapYear(Date->Year)][Date->Mon - 1] + (Date->Day - 1);
	}
	DSec = DDay * 86400 + Time->Hour * 3600 + Time->Min * 60 + Time->Sec;
	return DSec;
}
#define YEAR_1_DAY_BEFORE2000		365
#define YEAR_2_DAY_BEFORE2000		730
#define YEAR_3_DAY_BEFORE2000		1096


#define YEAR_1_DAY_AFTER2000		365
#define YEAR_2_DAY_AFTER2000		730
#define YEAR_3_DAY_AFTER2000		1095

#define YEAR_4_DAY		1461
#define YEAR_31_DAY		11323

#define YEAR_100_DAY	36524
#define YEAR_400_DAY	146097

uint32_t Tamp2UTC(LongInt Sec, Date_UserDataStruct *Date, Time_UserDataStruct *Time, uint32_t LastDDay)
{

	uint32_t DYear,i, LeapFlag, Temp;
	uint32_t DDay;
	DDay = Sec / 86400;
	if (DDay != LastDDay)
	{
		DYear = 0;
		Time->Week = (4 + DDay) % 7;
		if (DDay >= YEAR_31_DAY)
		{
			DDay -= YEAR_31_DAY;
			DYear = 31;

			if (DDay >= YEAR_400_DAY)
			{
				Temp = DDay / YEAR_400_DAY;
				DYear += Temp * 400;
				DDay -= Temp * YEAR_400_DAY;
			}

			if (DDay >= YEAR_100_DAY)
			{
				Temp = DDay / YEAR_100_DAY;
				DYear += Temp * 100;
				DDay -= Temp * YEAR_100_DAY;
			}

			if (DDay >= YEAR_4_DAY)
			{
				Temp = DDay / YEAR_4_DAY;
				DYear += Temp * 4;
				DDay -= Temp * YEAR_4_DAY;
			}

			if (DDay >= YEAR_3_DAY_AFTER2000)
			{
				DYear += 3;
				DDay -= YEAR_3_DAY_AFTER2000;
			}
			else if (DDay >= YEAR_2_DAY_AFTER2000)
			{
				DYear += 2;
				DDay -= YEAR_2_DAY_AFTER2000;
			}
			else if (DDay >= YEAR_1_DAY_AFTER2000)
			{
				DYear += 1;
				DDay -= YEAR_1_DAY_AFTER2000;
			}

		}
		else
		{
			if (DDay >= YEAR_4_DAY)
			{
				Temp = DDay / YEAR_4_DAY;
				DYear += Temp * 4;
				DDay -= Temp * YEAR_4_DAY;
			}

			if (DDay >= YEAR_3_DAY_BEFORE2000)
			{
				DYear += 3;
				DDay -= YEAR_3_DAY_BEFORE2000;
			}
			else if (DDay >= YEAR_2_DAY_BEFORE2000)
			{
				DYear += 2;
				DDay -= YEAR_2_DAY_BEFORE2000;
			}
			else if (DDay >= YEAR_1_DAY_BEFORE2000)
			{
				DYear += 1;
				DDay -= YEAR_1_DAY_BEFORE2000;
			}
		}

		Date->Year = DYear + 1970;
		LeapFlag = IsLeapYear(Date->Year);
		Date->Mon = 12;
		for (i = 1; i < 12; i++)
		{
			if (DDay < DayTable[LeapFlag][i])
			{
				Date->Mon = i;
				break;
			}
		}
		Date->Day = DDay - DayTable[LeapFlag][Date->Mon - 1] + 1;
	}

	Sec = Sec % 86400;
	Time->Hour = Sec / 3600;
	Sec = Sec % 3600;
	Time->Min = Sec / 60;
	Time->Sec = Sec % 60;
	return DDay;
}


/**
 * \brief get a byte (8bits) from a pointer
 *
 * Caller should ensure parameters are valid.
 *
 * \param ptr           the pointer
 * \return              the byte value
 */
uint8_t BytesGet8(const void *ptr)
{
    const uint8_t *p = (const uint8_t *)ptr;
    return p[0];
}

/**
 * \brief put a byte (8bits) to a pointer
 *
 * Caller should ensure parameters are valid.
 *
 * \param ptr           the pointer
 * \param v             the byte value
 */
void BytesPut8(void *ptr, uint8_t v)
{
    uint8_t *p = (uint8_t *)ptr;
    p[0] = v;
}

/**
 * \brief get a big endian short (16bits) from a pointer
 *
 * Caller should ensure parameters are valid.
 *
 * \param ptr           the pointer, may be unaligned
 * \return              the short value
 */
uint16_t BytesGetBe16(const void *ptr)
{
    const uint8_t *p = (const uint8_t *)ptr;
    return (p[0] << 8) | p[1];
}

/**
 * \brief put a big endian short (16bits) to a pointer
 *
 * Caller should ensure parameters are valid.
 *
 * \param ptr           the pointer, may be unaligned
 * \param v             the short value
 */
void BytesPutBe16(void *ptr, uint16_t v)
{
    uint8_t *p = (uint8_t *)ptr;
    p[0] = (v >> 8) & 0xff;
    p[1] = v & 0xff;
}

/**
 * \brief get a big endian word (32bits) from a pointer
 *
 * Caller should ensure parameters are valid.
 *
 * \param ptr           the pointer, may be unaligned
 * \return              the word value
 */
uint32_t BytesGetBe32(const void *ptr)
{
    const uint8_t *p = (const uint8_t *)ptr;
    return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

/**
 * \brief put a big endian word (32bits) to a pointer
 *
 * Caller should ensure parameters are valid.
 *
 * \param ptr           the pointer, may be unaligned
 * \param v             the word value
 */
void BytesPutBe32(void *ptr, uint32_t v)
{
    uint8_t *p = (uint8_t *)ptr;
    p[0] = (v >> 24) & 0xff;
    p[1] = (v >> 16) & 0xff;
    p[2] = (v >> 8) & 0xff;
    p[3] = v & 0xff;
}

/**
 * \brief get a little endian short (16bits) from a pointer
 *
 * Caller should ensure parameters are valid.
 *
 * \param ptr           the pointer, may be unaligned
 * \return              the short value
 */
uint16_t BytesGetLe16(const void *ptr)
{
    const uint8_t *p = (const uint8_t *)ptr;
    return p[0] | (p[1] << 8);
}

/**
 * \brief put a little endian short (16bits) to a pointer
 *
 * Caller should ensure parameters are valid.
 *
 * \param ptr           the pointer, may be unaligned
 * \param v             the short value
 */
void BytesPutLe16(void *ptr, uint16_t v)
{
    uint8_t *p = (uint8_t *)ptr;
    p[0] = v & 0xff;
    p[1] = (v >> 8) & 0xff;
}

/**
 * \brief get a little endian word (32bits) from a pointer
 *
 * Caller should ensure parameters are valid.
 *
 * \param ptr           the pointer, may be unaligned
 * \return              the word value
 */
uint32_t BytesGetLe32(const void *ptr)
{
    const uint8_t *p = (const uint8_t *)ptr;
    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

/**
 * \brief put a little endian word (32bits) to a pointer
 *
 * Caller should ensure parameters are valid.
 *
 * \param ptr           the pointer, may be unaligned
 * \param v             the word value
 */
void BytesPutLe32(void *ptr, uint32_t v)
{
    uint8_t *p = (uint8_t *)ptr;
    p[0] = v & 0xff;
    p[1] = (v >> 8) & 0xff;
    p[2] = (v >> 16) & 0xff;
    p[3] = (v >> 24) & 0xff;
}

/**
 * \brief get a little endian long long (64bits) from a pointer
 *
 * Caller should ensure parameters are valid.
 *
 * \param ptr           the pointer, may be unaligned
 * \return              the long long value
 */
uint64_t BytesGetLe64(const void *ptr)
{
    const uint8_t *p = (const uint8_t *)ptr;
    return BytesGetLe32(p) | ((uint64_t)BytesGetLe32(p + 4) << 32);
}

/**
 * \brief put a little endian long long (64bits) to a pointer
 *
 * Caller should ensure parameters are valid.
 *
 * \param ptr           the pointer, may be unaligned
 * \param v             the long long value
 */
void BytesPutLe64(void *ptr, uint64_t v)
{
    uint8_t *p = (uint8_t *)ptr;
    BytesPutLe32(p, v & 0xffffffff);
    BytesPutLe32(p + 4, (v >> 32) & 0xffffffff);
}

uint8_t BytesGet8FromBuf(Buffer_Struct *Buf)
{
	Buf->Pos++;
    return Buf->Data[Buf->Pos - 1];
}


void BytesPut8ToBuf(Buffer_Struct *Buf, uint8_t v)
{
	Buf->Data[Buf->Pos] = v;
	Buf->Pos++;
}

uint16_t BytesGetBe16FromBuf(Buffer_Struct *Buf)
{
	Buf->Pos += 2;
    return (Buf->Data[Buf->Pos - 2] << 8) | Buf->Data[Buf->Pos - 1];
}

void BytesPutBe16ToBuf(Buffer_Struct *Buf, uint16_t v)
{
	Buf->Data[Buf->Pos] = (v >> 8) & 0xff;
	Buf->Data[Buf->Pos + 1] = v & 0xff;
	Buf->Pos += 2;
}

uint32_t BytesGetBe32FromBuf(Buffer_Struct *Buf)
{
	Buf->Pos += 4;
    return (Buf->Data[Buf->Pos - 4] << 24) | (Buf->Data[Buf->Pos - 3] << 16) | (Buf->Data[Buf->Pos - 2] << 8) | Buf->Data[Buf->Pos - 1];
}

void BytesPutBe32ToBuf(Buffer_Struct *Buf, uint32_t v)
{
	Buf->Data[Buf->Pos] = (v >> 24) & 0xff;
	Buf->Data[Buf->Pos + 1] = (v >> 16) & 0xff;
	Buf->Data[Buf->Pos + 2] = (v >> 8) & 0xff;
	Buf->Data[Buf->Pos + 3] = v & 0xff;
	Buf->Pos += 4;
}


uint16_t BytesGetLe16FromBuf(Buffer_Struct *Buf)
{
	Buf->Pos += 2;
    return Buf->Data[Buf->Pos - 2] | (Buf->Data[Buf->Pos - 1] << 8);
}

void BytesPutLe16ToBuf(Buffer_Struct *Buf, uint16_t v)
{
	Buf->Data[Buf->Pos] = v & 0xff;
    Buf->Data[Buf->Pos + 1] = (v >> 8) & 0xff;
    Buf->Pos+= 2;
}

uint32_t BytesGetLe32FromBuf(Buffer_Struct *Buf)
{
	Buf->Pos += 4;
    return Buf->Data[Buf->Pos - 4] | (Buf->Data[Buf->Pos - 3] << 8) | (Buf->Data[Buf->Pos - 2] << 16) | (Buf->Data[Buf->Pos - 1] << 24);
}

void BytesPutLe32ToBuf(Buffer_Struct *Buf, uint32_t v)
{
	Buf->Data[Buf->Pos] = v & 0xff;
	Buf->Data[Buf->Pos + 1] = (v >> 8) & 0xff;
	Buf->Data[Buf->Pos + 2] = (v >> 16) & 0xff;
	Buf->Data[Buf->Pos + 3] = (v >> 24) & 0xff;
	Buf->Pos += 4;
}

uint64_t BytesGetLe64FromBuf(Buffer_Struct *Buf)
{
	uint64_t Temp = BytesGetLe32FromBuf(Buf);
    return Temp | ((uint64_t)BytesGetLe32FromBuf(Buf) << 32);
}

void BytesPutLe64ToBuf(Buffer_Struct *Buf, uint64_t v)
{

	BytesPutLe32ToBuf(Buf, v & 0xffffffff);
	BytesPutLe32ToBuf(Buf, (v >> 32) & 0xffffffff);
}

float BytesGetFloatFromBuf(Buffer_Struct *Buf)
{
	float Temp;
	Buf->Pos += 4;
	memcpy(&Temp, &Buf->Data[Buf->Pos - 4], 4);
    return Temp;
}

void BytesPutFloatToBuf(Buffer_Struct *Buf, float v)
{
	memcpy(&Buf->Data[Buf->Pos], &v, 4);
	Buf->Pos += 4;
}

double BytesGetDoubleFromBuf(Buffer_Struct *Buf)
{
	double Temp;
	Buf->Pos += 8;
	memcpy(&Temp, &Buf->Data[Buf->Pos - 8], 8);
    return Temp;
}

void BytesPutDoubleToBuf(Buffer_Struct *Buf, double v)
{
	memcpy(&Buf->Data[Buf->Pos], &v, 8);
	Buf->Pos += 8;
}

void BytesGetMemoryFromBuf(Buffer_Struct *Buf, uint8_t *Data, uint32_t Len)
{
	memcpy(Data, &Buf->Data[Buf->Pos], Len);
	Buf->Pos += Len;
}

/*
 * 转义打包
 * 标识Flag，即包头包尾加入Flag
 * 数据中遇到Flag -> Code F1
 * 数据中遇到Code -> Code F2
 */

uint32_t TransferPack(uint8_t Flag, uint8_t Code, uint8_t F1, uint8_t F2, uint8_t *InBuf, uint32_t Len, uint8_t *OutBuf)
{
	uint32_t TxLen = 0;
	uint32_t i;
	OutBuf[0] = Flag;
	TxLen = 1;
	for (i = 0; i < Len; i++)
	{
		if (InBuf[i] == Flag)
		{
			OutBuf[TxLen++] = Code;
			OutBuf[TxLen++] = F1;
		}
		else if (InBuf[i] == Code)
		{
			OutBuf[TxLen++] = Code;
			OutBuf[TxLen++] = F2;
		}
		else
		{
			OutBuf[TxLen++] = InBuf[i];
		}
	}
	OutBuf[TxLen++] = Flag;
	return TxLen;
}


/*
 * 转义解包
 * 标识Flag，即包头包尾加入Flag
 * 数据中遇到Code F1 -> Flag
 * 数据中遇到Code F2 -> Code
 * 数据中遇到Flag 出错返回0
 */
uint32_t TransferUnpack(uint8_t Flag, uint8_t Code, uint8_t F1, uint8_t F2, uint8_t *InBuf, uint32_t Len, uint8_t *OutBuf)
{
	uint32_t RxLen = 0;
	uint32_t i = 0;
	while (i < Len)
	{
		if (InBuf[i] == Code)
		{
			if (InBuf[i+1] == F1)
			{
				OutBuf[RxLen++] = Flag;
			}
			else if (InBuf[i+1] == F2)
			{
				OutBuf[RxLen++] = Code;
			}
			else
			{
				return 0;
			}
			i += 2;
		}
		else if (InBuf[i] == Flag)
		{
			return 0;
		}
		else
		{
			OutBuf[RxLen++] = InBuf[i++];
		}
	}
	return RxLen;
}

/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal llist manipulation where we know
 * the prev/next entries already!
 */
void __llist_add(llist_head *p,
                         llist_head *prev,
                         llist_head *next)
{
	next->prev = p;
	p->next = next;
	p->prev = prev;
	prev->next = p;
}

/**
 * llist_add - add a new entry
 * @new: new entry to be added
 * @head: llist head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
void llist_add(llist_head *p, llist_head *head)
{
	__llist_add(p, head, head->next);
}

/**
 * llist_add_tail - add a new entry
 * @new: new entry to be added
 * @head: llist head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
void llist_add_tail(llist_head *p, llist_head *head)
{
	__llist_add(p, head->prev, head);
}

/*
 * Delete a llist entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal llist manipulation where we know
 * the prev/next entries already!
 */
void __llist_del(llist_head * prev, llist_head * next)
{
	next->prev = prev;
	prev->next = next;
}

/**
 * llist_del - deletes entry from llist.
 * @entry: the element to delete from the llist.
 * Note: llist_empty on entry does not return true after this, the entry is
 * in an undefined state.
 */
void llist_del(llist_head *entry)
{
	if (entry->prev && entry->next)
	{
		__llist_del(entry->prev, entry->next);
	}
	entry->next = LLIST_POISON1;
	entry->prev = LLIST_POISON2;
}

/**
 * llist_del_init - deletes entry from llist and reinitialize it.
 * @entry: the element to delete from the llist.
 */
void llist_del_init(llist_head *entry)
{
	__llist_del(entry->prev, entry->next);
	INIT_LLIST_HEAD(entry);
}

/**
 * llist_move - delete from one llist and add as another's head
 * @llist: the entry to move
 * @head: the head that will precede our entry
 */
void llist_move(llist_head *llist, llist_head *head)
{
        __llist_del(llist->prev, llist->next);
        llist_add(llist, head);
}

/**
 * llist_move_tail - delete from one llist and add as another's tail
 * @llist: the entry to move
 * @head: the head that will follow our entry
 */
void llist_move_tail(llist_head *llist,
				  llist_head *head)
{
        __llist_del(llist->prev, llist->next);
        llist_add_tail(llist, head);
}

void *llist_traversal(llist_head *head, CBFuncEx_t cb, void *pData)
{
	llist_head *node = head->next;
	llist_head *del;
	int32_t result;
	while (!llist_empty(head) && (node != head))
	{
		result = cb((void *)node, pData);
		if (result > 0)
		{
			return node;
		}
		else
		{
			del = node;
			node = node->next;
			if (result < 0)
			{
				llist_del(del);
				free(del);
			}
		}
	}
	return NULL;
}

/**
 * llist_empty - tests whether a llist is empty
 * @head: the llist to test.
 */
int llist_empty(const llist_head *head)
{
	return head->next == head;
}

uint32_t llist_num(const llist_head *head)
{
	llist_head *node = head->next;
	uint32_t num = 0;
	if (!node)
		return num;
	while(node != head)
	{
		num++;
		node = node->next;
	}
	return num;
}

#define PP_HTONS(x) ((uint16_t)((((x) & (uint16_t)0x00ffU) << 8) | (((x) & (uint16_t)0xff00U) >> 8)))
#define PP_NTOHS(x) PP_HTONS(x)
#define PP_HTONL(x) ((((x) & (uint32_t)0x000000ffUL) << 24) | \
                     (((x) & (uint32_t)0x0000ff00UL) <<  8) | \
                     (((x) & (uint32_t)0x00ff0000UL) >>  8) | \
                     (((x) & (uint32_t)0xff000000UL) >> 24))
#define PP_NTOHL(x) PP_HTONL(x)

uint16_t BSP_Swap16(uint16_t n)
{
  return (uint16_t)PP_HTONS(n);
}

uint32_t BSP_Swap32(uint32_t n)
{
  return (uint32_t)PP_HTONL(n);
}

uint32_t utf8_to_unicode(uint8_t *string, uint32_t len, void *out, uint8_t is_only_16)
{
	uint32_t i = 0;
	uint32_t result = 0;
	uint8_t bit, n;
	if (is_only_16)
	{
		uint16_t *buf = (uint16_t *)out;
		while (i < len)
		{
			if (string[i] & 0x80)
			{
				if (!(string[i] & (1 << 5)))
				{
					n = 2;
					buf[result] = string[i] & ((1 << 5) - 1);
				}
				else
				{
					buf[result] = string[i] & ((1 << 4) - 1);
					n = 3;
				}

				for (bit = 1; bit < n; bit++)
				{
					buf[result] = (buf[result] << 6) | (string[i + bit] & 0x3f);
				}
				i += n;
			}
			else
			{
				buf[result] = string[i];
				i++;
			}
			result++;
		}
	}
	else
	{
		uint8_t table[7] = {0, 0, 0x1f, 0x0f, 0x07, 0x03, 0x01};
		uint32_t *buf = (uint32_t *)out;
		while (i < len)
		{
			if (string[i] & 0x80)
			{
				n = 7;
				for (bit = 5; bit >= 1; bit--)
				{
					if (!(string[i] & (1 << bit)))
					{
						n -= bit;
						break;
					}
				}
				if (n >= 7)
				{
					return result;
				}
				buf[result] = string[i] & table[n];

				for (bit = 1; bit < n; bit++)
				{
					buf[result] = (buf[result] << 6) | (string[i + bit] & 0x3f);
				}
				i += n;
			}
			else
			{
				buf[result] = string[i];
				i++;
			}
			result++;
		}
	}
	return result;
}

uint32_t unicode_to_utf8(void *in, uint32_t unicodelen, uint8_t *out, uint8_t is_only_16)
{
	uint32_t i = 0;
	uint32_t result = 0;
	uint8_t bit, n;
	if (is_only_16)
	{
		uint16_t *buf = (uint16_t *)in;
		while (i < unicodelen)
		{
			if (buf[i] <= 0x007f)
			{
				out[result] = buf[i];

				result++;
			}
			else
			{
				if (buf[i] >> 12)
				{
					out[result + 2] = (buf[i] & 0x3f) | 0x80;
					out[result + 1] = ((buf[i] >> 6) & 0x3f) | 0x80;
					out[result] = 0xe0 | (buf[i] >> 12);
					result += 3;
				}
				else
				{
					out[result + 1] = (buf[i] & 0x3f) | 0x80;
					out[result] = 0xc0 | (buf[i] >> 6);
					result += 2;
				}
			}
			i++;
		}

	}
	else
	{
		uint8_t table[7] = {0,0,0xc0,0xe0, 0xf0, 0xf8, 0xfc};
		uint8_t pos[7] = {0,0,6,12,18,24,30};
		uint32_t *buf = (uint32_t *)in;
		while (i < unicodelen)
		{
			if (buf[i] <= 0x007f)
			{
				out[result] = buf[i];

				result++;
			}
			else
			{
				n = 6;
				for (bit = 1; bit < 6; bit++)
				{
					if (!(buf[i] >> ((bit + 1) * 6)))
					{
						n = bit + 1;
						break;
					}
				}
				out[result] = table[n] | (buf[i] >> pos[n]);
				for (bit = 1; bit < n; bit++)
				{
					out[result + bit] = ( (buf[i] >> ((n - bit - 1) * 6)) & 0x3f) | 0x80;
				}
				result += n;
			}
			i++;
		}
	}
	return result;
}
