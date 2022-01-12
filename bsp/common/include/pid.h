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

#ifndef __PID_H__
#define __PID_H__
#include "bsp_common.h"
#define VAR_TYPE	double
typedef struct{
	VAR_TYPE Scope;        //输出限幅量
	VAR_TYPE Target;       //传感器目标输入量
	VAR_TYPE Measure;     //测量到的实际输入量，每次测量后变更
	VAR_TYPE Kp;
	VAR_TYPE Ki;
	VAR_TYPE Kd;
	VAR_TYPE Sum;  		//累计误差
	VAR_TYPE e0;          //当前误差
	VAR_TYPE e1;          //上一次误差
	VAR_TYPE e2;          //上上次误差
}PID_VarType;

/*
 * 增量PID，输出值是控制器输出量的增量
 */
VAR_TYPE PID_IncrementalCal(PID_VarType *PIDVar);
/*
 * 位置PID，输出值是控制器输出量
 */
VAR_TYPE PID_PositionCal(PID_VarType *PIDVar);

#endif
