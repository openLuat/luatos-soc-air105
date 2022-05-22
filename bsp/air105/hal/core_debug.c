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

#include "user.h"

#define JTT_PACK_FLAG		(0x7e)
#define JTT_PACK_CODE		(0x7d)
#define JTT_PACK_CODE_F1	(0x02)
#define JTT_PACK_CODE_F2	(0x01)

#ifdef __BUILD_APP__
#define DBG_BUF_SIZE (2048)
#define DBG_TXBUF_SIZE DBG_BUF_SIZE
#define DBG_RXBUF_SIZE (0)
#define DBG_RXBUF_BAND (1)
#else
#define DBG_BUF_SIZE (4090)
#define DBG_TXBUF_SIZE DBG_BUF_SIZE
#define DBG_RXBUF_SIZE DBG_BUF_SIZE
#define DBG_RXBUF_BAND (3)
#endif

typedef struct
{
	Buffer_Struct LogBuffer;
	Buffer_Struct RxBuffer;
	CBFuncEx_t Fun;
	CBDataFun_t TxFun;
	//Loop_Buffer IrqBuffer;
	uint8_t Data[DBG_BUF_SIZE];
#ifdef __RUN_IN_RAM__
	uint8_t CacheData[__FLASH_BLOCK_SIZE__];
#else
	uint8_t CacheData[DBG_BUF_SIZE * 2];
#endif
#ifdef __RUN_IN_RAM__
	uint8_t RxData[__FLASH_BLOCK_SIZE__];
#else
	uint8_t RxData[DBG_BUF_SIZE * 2];
#endif
	uint8_t TxBuf[DBG_TXBUF_SIZE];
	uint8_t RxDMABuf[DBG_RXBUF_BAND][DBG_RXBUF_SIZE + 16];
	uint8_t AppMode;
	uint8_t InitDone;
	uint8_t InsertBusy;
	uint8_t TxBusy;
	uint8_t RxDMASn;
}DBG_CtrlStruct;
static DBG_CtrlStruct prvDBGCtrl;

static void prvDBG_Out(void)
{
	uint32_t TxLen;
SEND_AGAIN:
	if (prvDBGCtrl.LogBuffer.Pos)
	{
		prvDBGCtrl.TxBusy = 1;
		TxLen = (prvDBGCtrl.LogBuffer.Pos > sizeof(prvDBGCtrl.TxBuf))?sizeof(prvDBGCtrl.TxBuf):prvDBGCtrl.LogBuffer.Pos;
		memcpy(prvDBGCtrl.TxBuf, prvDBGCtrl.LogBuffer.Data, TxLen);
		OS_BufferRemove(&prvDBGCtrl.LogBuffer, TxLen);
		Uart_DMATx(DBG_UART_ID, DBG_UART_TX_DMA_STREAM, prvDBGCtrl.TxBuf, TxLen);
//		if (!Uart_BufferTx(DBG_UART_ID, prvDBGCtrl.TxBuf, TxLen))
//		{
//			prvDBGCtrl.TxBusy = 0;
//			goto SEND_AGAIN;
//		}
	}
	else
	{
		if (prvDBGCtrl.LogBuffer.MaxLen > (DBG_BUF_SIZE * 8))
		{
			OS_ReInitBuffer(&prvDBGCtrl.LogBuffer, (DBG_BUF_SIZE * 4));
		}
	}
}

static void add_trace_data(uint8_t *data, uint32_t len)
{
	Buffer_StaticWrite(&prvDBGCtrl.LogBuffer, data, len);
}

void add_printf_data(uint8_t *data, uint32_t len)
{
#ifdef __BUILD_APP__

	uint32_t Critical = OS_EnterCritical();
	ASSERT(prvDBGCtrl.InsertBusy == 0);
	prvDBGCtrl.InsertBusy = 1;
	if ((prvDBGCtrl.LogBuffer.Pos + len) > prvDBGCtrl.LogBuffer.MaxLen)
	{
		OS_ReSizeBuffer(&prvDBGCtrl.LogBuffer, prvDBGCtrl.LogBuffer.MaxLen * 2);
	}
	OS_BufferWrite(&prvDBGCtrl.LogBuffer, data, len);
	prvDBGCtrl.InsertBusy = 0;
	if (prvDBGCtrl.TxBusy || !prvDBGCtrl.InitDone || !prvDBGCtrl.AppMode)
	{
		OS_ExitCritical(Critical);
		return;
	}
	prvDBG_Out();
	OS_ExitCritical(Critical);
#endif
}

extern const uint8_t ByteToAsciiTable[16];

void DBG_Trace(const char* format, ...)
{
#ifndef __RAMRUN__
	char c;
	char *s = 0;
	int d = 0, i = 0, p = 0;
	uint32_t slen;
	unsigned int ud = 0;
	unsigned char ch[36] = {0};
	unsigned char int_max;
	unsigned char flag;
	va_list ap;
#ifdef __BUILD_APP__
	__disable_irq();
	if (prvDBGCtrl.AppMode)
	{
		prvDBGCtrl.AppMode = 0;
		//关闭掉APP的DMA UART输出
		//
		DMA_StopStream(DBG_UART_TX_DMA_STREAM);
		Buffer_StaticInit(&prvDBGCtrl.LogBuffer, prvDBGCtrl.Data, sizeof(prvDBGCtrl.Data));
	}
#endif
	va_start(ap, format);

	while (* format)
	{
		if (* format != '%')
		{
			add_trace_data((uint8_t *)format, 1);
			format += 1;
			continue;
		}
		slen = 0;
repeat:
		switch (*(++format))
		{
		case '*':
			slen = va_arg(ap, int);
			goto repeat;
			break;
		case 's':
		case 'S':
			s = va_arg(ap, char *);
			if (!slen)
			{
				for ( ; *s; s++)
				{
					add_trace_data((uint8_t *) s, 1);

				}
			}
			else
			{
				add_trace_data((uint8_t *) s, slen);
			}

			break;
		case 'c':
		case 'C':
			c = va_arg(ap, int);
			add_trace_data((uint8_t *)&c, 1);

			break;
		case 'x':
		case 'X':
			ud = va_arg(ap, int);
			p = 8;
			for(i = 0; i < p; i++)
			{
				ch[i] = ByteToAsciiTable[(unsigned char)(ud&0x0f)];
				ud >>= 4;
			}

			for(i = p; i > 0; i--)
			{
				add_trace_data(&ch[i -1], 1);
			}
			break;
		case 'u':
		case 'U':
			ud = va_arg(ap, unsigned int);
			int_max = 0;
			for(i = 0; i < 16; i++)
			{

				ch[i] = ud%10 + '0';
				ud = ud/10;
				int_max++;
				if (!ud)
					break;
			}
			for(i = int_max; i > 0; i--)
			{
				add_trace_data(&ch[i -1], 1);

			}
			break;
		case 'd':
		case 'D':
			d = va_arg(ap, int);
			if (d < 0) {
				flag = 1;
			} else {
				flag = 0;
			}
			d = abs(d);
			int_max = 0;
			for(i = 0; i < 16; i++)
			{

				ch[i] = d%10 + '0';
				d = d/10;
				int_max++;
				if (!d)
					break;
			}
			if (flag)
			{
				flag = '-';
				add_trace_data(&flag, 1);

			}
			for(i = int_max; i > 0; i--)
			{
				add_trace_data(&ch[i -1], 1);

			}
			break;
		case 'b':
		case 'B':
			ud = va_arg(ap, int);
			if (ud > 0xffff)
			{
				p = 32;
			}
			else if (ud > 0xff)
			{
				p = 16;
			}
			else
			{
				p = 8;
			}
			for(i = 0; i < p; i++)
			{
				ch[i] = ByteToAsciiTable[(unsigned char)(ud&0x01)];
				ud >>= 1;
			}

			for(i = p; i > 0; i--)
			{
				add_trace_data(&ch[i -1], 1);
			}

			break;
		default:
			//add_trace_data((uint8_t *)format, 1);
			goto repeat;
			break;
		}
		format++;
	}
	va_end(ap);
	add_trace_data("\r\n", 2);
//	va_list ap;
//	va_start(ap, format);
//	prvDBGCtrl.TxPos = vsnprintf(prvDBGCtrl.TxBuf, sizeof(prvDBGCtrl.TxBuf) - 2, format, ap);
//	va_end(ap);
//	prvDBGCtrl.TxBuf[prvDBGCtrl.TxPos] = '\r';
//	prvDBGCtrl.TxBuf[prvDBGCtrl.TxPos + 1] = '\n';
	Uart_BlockTx(DBG_UART_ID, prvDBGCtrl.LogBuffer.Data, prvDBGCtrl.LogBuffer.Pos);
	prvDBGCtrl.LogBuffer.Pos = 0;
#ifdef __BUILD_APP__
	__enable_irq();
#endif
#endif
}
void DBG_BlHexPrintf(void *Data, unsigned int len)
{
	uint8_t *data = (uint8_t *)Data;
	int8_t uart_buf[128];
    uint32_t i,j;
    j = 0;


    if (!len)
    {
    	return ;
    }


    for (i = 0; i < len; i++)
    {
    	uart_buf[j++] = ByteToAsciiTable[(data[i] & 0xf0) >> 4];
    	uart_buf[j++] = ByteToAsciiTable[data[i] & 0x0f];
    	uart_buf[j++] = ' ';
    }
    uart_buf[j++] = '\r';
    uart_buf[j++] = '\n';
    add_trace_data(uart_buf, j);
	Uart_BlockTx(DBG_UART_ID, prvDBGCtrl.LogBuffer.Data, prvDBGCtrl.LogBuffer.Pos);
	prvDBGCtrl.LogBuffer.Pos = 0;
}

#ifdef __BUILD_OS__

void DBG_HexPrintf(void *Data, unsigned int len)
{
	uint8_t *data = (uint8_t *)Data;
	int8_t *uart_buf;
    uint32_t i,j;
    j = 0;

	if (!prvDBGCtrl.AppMode) return;

    if (!len)
    {
    	return ;
    }
    uart_buf = OS_Zalloc(len * 3 + 2);

    if (!uart_buf)
    {
    	return;
    }
    for (i = 0; i < len; i++)
    {
    	uart_buf[j++] = ByteToAsciiTable[(data[i] & 0xf0) >> 4];
    	uart_buf[j++] = ByteToAsciiTable[data[i] & 0x0f];
    	uart_buf[j++] = ' ';
    }
    uart_buf[j++] = '\r';
    uart_buf[j++] = '\n';

    prvDBGCtrl.TxFun(uart_buf, len * 3 + 2);
	OS_Free(uart_buf);
}

void DBG_Printf(const char* format, ...)
{
	char *buf = NULL;
	char isr_buf[256];
	int len;
	va_list ap;
	if (!prvDBGCtrl.AppMode) return;
	va_start(ap, format);
	if (OS_CheckInIrq())
	{
		buf = isr_buf;
		len = vsnprintf_(buf, 255, format, ap);
	}
	else
	{
		buf = OS_Zalloc(1024);
		len = vsnprintf_(buf, 1023, format, ap);
	}
	va_end(ap);
#ifndef __DEBUG__
    prvDBGCtrl.TxFun( buf, len);
#else
    add_printf_data(buf, len);
#endif
	if (!OS_CheckInIrq())
	{
		OS_Free(buf);
	}
}

void DBG_DirectOut(void *Data, uint32_t Len)
{
	add_printf_data(Data, Len);
}

void DBG_Send(void)
{
	uint32_t Critical;
	uint32_t TxLen;
	if (prvDBGCtrl.TxBusy || !prvDBGCtrl.InitDone || !prvDBGCtrl.AppMode)
	{
		return;
	}

	Critical = OS_EnterCritical();
	prvDBG_Out();
	OS_ExitCritical(Critical);
}

int _write(int file, char *ptr, int len)
{
    add_printf_data(ptr, len);
    return len;
}

#endif

int32_t DBG_UartIrqCB(void *pData, void *pParam)
{
	uint32_t Length;
	uint32_t State = (uint32_t)pParam;
	uint8_t LastRxSn;
	switch (State)
	{

	case UART_CB_TX_BUFFER_DONE:
	case UART_CB_TX_ALL_DONE:

#ifdef __BUILD_APP__
	case DMA_CB_DONE:
		{
			DMA_ClearStreamFlag(DBG_UART_TX_DMA_STREAM);
			prvDBGCtrl.TxBusy = 0;
			if (!prvDBGCtrl.InsertBusy)
			{
				DBG_Send();
			}
		}
#endif
		break;
	case UART_CB_RX_BUFFER_FULL:
	case UART_CB_RX_TIMEOUT:
#ifdef __BUILD_APP__
		Uart_RxBufferCB(DBG_UART_ID, prvDBGCtrl.Fun);
		break;
#endif
	case UART_CB_RX_NEW:
#ifndef __BUILD_APP__
	case DMA_CB_DONE:
		DMA_StopStream(DBG_UART_RX_DMA_STREAM);
		Length = DMA_GetDataLength(DBG_UART_RX_DMA_STREAM, &prvDBGCtrl.RxDMABuf[prvDBGCtrl.RxDMASn][0]);
		Length += Uart_FifoRead(DBG_UART_ID, &prvDBGCtrl.RxDMABuf[prvDBGCtrl.RxDMASn][Length]);
		LastRxSn = prvDBGCtrl.RxDMASn;
		prvDBGCtrl.RxDMASn = (prvDBGCtrl.RxDMASn + 1)%DBG_RXBUF_BAND;
		Uart_DMARx(DBG_UART_ID, DBG_UART_RX_DMA_STREAM, &prvDBGCtrl.RxDMABuf[prvDBGCtrl.RxDMASn][0], DBG_RXBUF_SIZE);
		if (Length)
		{
			prvDBGCtrl.Fun(&prvDBGCtrl.RxDMABuf[LastRxSn][0], Length);
		}
#else
//		Length = Uart_FifoRead(DBG_UART_ID, &prvDBGCtrl.RxDMABuf[prvDBGCtrl.RxDMASn][0]);
//		LastRxSn = prvDBGCtrl.RxDMASn;
//		prvDBGCtrl.RxDMASn = (prvDBGCtrl.RxDMASn + 1)%DBG_RXBUF_BAND;
#endif

		break;
	case UART_CB_ERROR:
//		DBG("%x", pParam);
		break;
	}
}


void DBG_Response(uint8_t Cmd, uint8_t Result, uint8_t *Data, uint8_t Len)
{
	uint8_t Temp[260];
	uint16_t TxLen, Check;
	Temp[0] = Cmd;
	Temp[1] = Result;
	Temp[2] = Len;
	memcpy(Temp + 3, Data, Len);
	Check = CRC16Cal(Temp, Len + 3, CRC16_CCITT_SEED, CRC16_CCITT_GEN, 0);
	BytesPutLe16(&Temp[Len + 3], Check);
	TxLen = TransferPack(JTT_PACK_FLAG, JTT_PACK_CODE, JTT_PACK_CODE_F1, JTT_PACK_CODE_F2, Temp, Len + 5, prvDBGCtrl.TxBuf);
	Uart_BufferTx(DBG_UART_ID, prvDBGCtrl.TxBuf, TxLen);
}

static int32_t DBG_RebootTimeoutCB(void *pData, void *pParam)
{
	NVIC_SystemReset();
}

static void DBG_Reboot(uint8_t *Data, uint32_t Len)
{
#ifdef __BUILD_APP__
	timer_t *Timer = Timer_Create(DBG_RebootTimeoutCB, NULL, NULL);
	Timer_StartMS(Timer, 100, 1);
	Uart_DeInit(DBG_UART_ID);
	prvDBGCtrl.AppMode = 0;
#else
	NVIC_SystemReset();
#endif
}

static void DBG_FlashDownloadStart(uint8_t *Data, uint32_t Len)
{
#ifdef __BUILD_APP__
#else
	if (Len < 12)
	{
		DBG_Response(DBG_CMD_FLASHDOWNLOADSTART, ERROR_PARAM_INVALID, &Len, 4);
		return;
	}
	uint32_t StartAddress = BytesGetLe32(Data);
	uint32_t TotalLen = BytesGetLe32(Data + 4);
	uint32_t CRC32 = BytesGetLe32(Data + 8);
	if (BL_StartNewDownload(StartAddress, TotalLen, CRC32))
	{
		DBG_Response(DBG_CMD_FLASHDOWNLOADSTART, ERROR_PERMISSION_DENIED, &Len, 4);
		return;
	}
	else
	{
		DBG_Response(DBG_CMD_FLASHDOWNLOADSTART, ERROR_NONE, &Len, 4);
		return;
	}
#endif
}

static void DBG_FlashDownload(uint8_t *Data, uint32_t Len)
{
#ifdef __BUILD_APP__
#else
	if (Len < 7)
	{
		DBG_Response(DBG_CMD_FLASHDOWNLOAD, ERROR_PARAM_INVALID, &Len, 4);
		return;
	}
	uint32_t PacketSn = BytesGetLe32(Data);
	uint32_t NextPacketSn;
	int Result;
	uint16_t PacketLen = BytesGetLe32(Data + 4);
	if ( (PacketLen + 6) != Len )
	{
		DBG_Response(DBG_CMD_FLASHDOWNLOAD, ERROR_PARAM_INVALID, &Len, 4);
		return;
	}
	else
	{
		Result = BL_DownloadAddData(PacketSn, Data + 6, PacketLen, &NextPacketSn);
		if (Result >= 0)
		{
			DBG_Response(DBG_CMD_FLASHDOWNLOAD, ERROR_NONE, &Result, 4);
		}
		else
		{
			DBG_Response(DBG_CMD_FLASHDOWNLOAD, ERROR_PARAM_OVERFLOW, &NextPacketSn, 4);
		}
		return;
	}
#endif
}

static void DBG_FlashDownloadEnd(uint8_t *Data, uint32_t Len)
{
#ifdef __BUILD_APP__
#else
	uint8_t Result = BL_DownloadEnd();
	DBG_Response(DBG_CMD_FLASHDOWNLOADEND, Result, &Result, 1);
#endif
}

static void DBG_RunApp(uint8_t *Data, uint32_t Len)
{
#ifdef __BUILD_APP__
#else
	uint32_t Result = BL_RunAPP();
	DBG_Response(DBG_CMD_RUNAPP, Result, &Result, 4);
#endif
}

static void DBG_FlashZipBlockEnd(uint8_t *Data, uint32_t Len)
{
#ifdef __BUILD_APP__
#else
	uint8_t Result = BL_RunLzmaBlock(Data + 1, Data[0]);
	DBG_Response(DBG_CMD_FLASHLZMABLOCKEND, Result, &Result, 1);
#endif
}

static const CBDataFun_t DBG_ProtoclFunlist[] =
{
		DBG_Reboot,
		DBG_FlashDownloadStart,
		DBG_FlashDownload,
		DBG_FlashDownloadEnd,
		DBG_RunApp,
		DBG_FlashZipBlockEnd,
};

static int32_t DBG_DummyRx(void *pData, void *pParam)
{
	uint32_t DelLen;
	uint32_t i, DealLen;
	uint16_t Check, Check2;
	//Buffer_Struct Buffer;
	uint8_t FindHead = 0;
	uint8_t FindEnd = 0;
	uint8_t NoMoreData = 0;
	int32_t Result;
	Buffer_Struct *RxBuf = &prvDBGCtrl.RxBuffer;
	DBG("%u", pParam);
	if (pParam)
	{

#ifdef __BUILD_APP__
		OS_BufferWrite(&prvDBGCtrl.RxBuffer, (uint8_t *)pData, (uint32_t)pParam);
#else
		Buffer_StaticWrite(&prvDBGCtrl.RxBuffer, (uint8_t *)pData, (uint32_t)pParam);
#endif

		while(!NoMoreData)
		{
			if (!RxBuf->Pos)
			{
//				DBG("no more net rx data");
				break;
			}
			if (!FindHead)
			{
				for(i = 0; i < RxBuf->Pos; i++)
				{
					if (JTT_PACK_FLAG == RxBuf->Data[i])
					{
						OS_BufferRemove(RxBuf, i);
						FindHead = 1;
						break;
					}
				}
				if (!FindHead)
				{
					NoMoreData = 1;
					RxBuf->Pos = 0;
					continue;
				}
			}
			FindEnd = 0;
			DealLen = 0;
			DelLen = 0;
			for(i = 1; i < RxBuf->Pos; i++)
			{
				if (JTT_PACK_FLAG == RxBuf->Data[i])
				{
					DelLen = i + 1;
//					DBG("msg rec len %dbyte", DelLen);
					DealLen = i + 1;
					FindEnd = 1;
					break;
				}
			}
			if (!FindEnd)
			{
//				DBG("rest %dbyte", RxBuf->Pos);
				NoMoreData = 1;
				continue;
			}
			if (2 == DealLen) //连续2个0x7E，去除第一个，第二个作为头开始重新搜索
			{
//				DBG("!");
				OS_BufferRemove(RxBuf, 1);
				FindHead = 1;
				continue;
			}

			if (FindEnd)
			{
				FindHead = 0;
#ifdef __RUN_IN_RAM__
				if (DelLen >= sizeof(prvDBGCtrl.CacheData))
#else
				if (DelLen > DBG_BUF_SIZE)
#endif
				{
					DBG("msg too long, %u", DelLen);
					OS_BufferRemove(RxBuf, DelLen);
					continue;
				}
				DealLen = TransferUnpack(JTT_PACK_FLAG, JTT_PACK_CODE, JTT_PACK_CODE_F1, JTT_PACK_CODE_F2,
						&RxBuf->Data[1], DealLen - 2, prvDBGCtrl.RxData);
				OS_BufferRemove(RxBuf, DelLen);
				if (DealLen >= 5)
				{
					Check = CRC16Cal(prvDBGCtrl.RxData, DealLen - 2, CRC16_CCITT_SEED, CRC16_CCITT_GEN, 0);
					Check2 = BytesGetLe16(&prvDBGCtrl.RxData[DealLen - 2]);
					if (Check != Check2)
					{
//						DBG_BlHexPrintf(Data, Len);
						DBG("check error %04x %04x", Check, Check2);
						DBG_Response(prvDBGCtrl.RxData[0], ERROR_PROTOCL, &Result, 4);
						RxBuf->Pos = 0;
						return 0;
					}
					else
					{
						DealLen -= 2;
						if (prvDBGCtrl.RxData[0] < sizeof(DBG_ProtoclFunlist)/sizeof(CBDataFun_t))
						{
							DBG_ProtoclFunlist[prvDBGCtrl.RxData[0]](prvDBGCtrl.RxData + 1, DealLen - 1);
						}
					}
				}
				else
				{
					DBG("%d", DealLen);
				}
			}
		}
	}
	return 0;
}

void DBG_Init(uint8_t AppMode)
{
	prvDBGCtrl.AppMode = AppMode;
	GPIO_Iomux(DBG_UART_TX_PIN, DBG_UART_TX_AF);
	GPIO_Iomux(DBG_UART_RX_PIN, DBG_UART_RX_AF);
#ifdef __BUILD_APP__
	if (AppMode)
	{
		OS_InitBuffer(&prvDBGCtrl.LogBuffer, DBG_BUF_SIZE * 4);
		OS_InitBuffer(&prvDBGCtrl.RxBuffer, DBG_BUF_SIZE);
		//Buffer_StaticInit(&prvDBGCtrl.LogBuffer, prvDBGCtrl.Data, sizeof(prvDBGCtrl.Data));
		Uart_BaseInit(DBG_UART_ID, DBG_UART_BR, 1, UART_DATA_BIT8, UART_PARITY_NONE, UART_STOP_BIT1, DBG_UartIrqCB);
		Uart_EnableRxIrq(DBG_UART_ID);
		DMA_TakeStream(DBG_UART_TX_DMA_STREAM);
		Uart_DMATxInit(DBG_UART_ID, DBG_UART_TX_DMA_STREAM, DBG_UART_TX_DMA_CHANNEL);
		prvDBGCtrl.TxFun = add_printf_data;
	}
	else
#endif
	{
		Buffer_StaticInit(&prvDBGCtrl.LogBuffer, prvDBGCtrl.Data, sizeof(prvDBGCtrl.Data));
		Buffer_StaticInit(&prvDBGCtrl.RxBuffer, prvDBGCtrl.CacheData, sizeof(prvDBGCtrl.CacheData));
		Uart_BaseInit(DBG_UART_ID, BL_UART_BR, 0, UART_DATA_BIT8, UART_PARITY_NONE, UART_STOP_BIT1, DBG_UartIrqCB);
		DMA_TakeStream(DBG_UART_RX_DMA_STREAM);
		Uart_DMARxInit(DBG_UART_ID, DBG_UART_RX_DMA_STREAM, DBG_UART_RX_DMA_CHANNEL);
		Uart_DMARx(DBG_UART_ID, DBG_UART_RX_DMA_STREAM, &prvDBGCtrl.RxDMABuf[prvDBGCtrl.RxDMASn][0], DBG_RXBUF_SIZE);

	}
	prvDBGCtrl.InitDone = 1;
	prvDBGCtrl.Fun = DBG_DummyRx;
	PM_SetDriverRunFlag(PM_DRV_DBG, 1);
}

void DBG_SetRxCB(CBFuncEx_t cb)
{
	prvDBGCtrl.Fun = cb;
}

void DBG_SetTxCB(CBDataFun_t cb)
{
	prvDBGCtrl.TxFun = cb;
}
