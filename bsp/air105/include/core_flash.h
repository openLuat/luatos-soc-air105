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

#ifndef __CORE_FLASH_H__
#define __CORE_FLASH_H__
/**
  * @brief  4K擦除，会关闭中断来保证操作的原子性
  * @param  address: 擦除地址
  * @param  NeedCheck: 是否做0xff检测
  * @retval 0成功，其他失败
  */
int Flash_EraseSector(uint32_t address, uint8_t NeedCheck);
/**
  * @brief  写入任意数据，最少4B
  * @param  address: 写入地址
  * @param  Data: 写入的数据
  * @param  Len: 写入的数据长度
  * @param  NeedCheck:  是否做数据一致性检测
  * @retval 0成功，其他失败
  */
int Flash_ProgramData(uint32_t address, uint32_t *Data, uint32_t Len, uint8_t NeedCheck);

/**
  * @brief  任意长度擦除，最少4K，APP中会关闭中断来保证操作的原子性，BL不会
  * @param  Address: 擦除起始地址
  * @param  Length: 擦除长度
  * @retval 0成功，其他失败
  */
int32_t Flash_Erase(uint32_t Address, uint32_t Length);
/**
  * @brief  写入任意数据量，最少1B，APP中会关闭中断来保证操作的原子性，BL不会
  * @param  Address: 写入地址
  * @param  pBuf: 写入的数据
  * @param  Len: 写入的数据长度
  * @retval 0成功，其他失败
  */
int32_t Flash_Program(uint32_t Address, const uint8_t *pBuf, uint32_t Len);
#endif
