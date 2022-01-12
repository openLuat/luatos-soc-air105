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

#include "pid.h"

VAR_TYPE PID_IncrementalCal(PID_VarType *PIDVar)
{
	VAR_TYPE out;
	VAR_TYPE ep, ei, ed;

    PIDVar->e0 = PIDVar->Target - PIDVar->Measure;
    ep = PIDVar->e0  - PIDVar->e1;
    ei = PIDVar->e0;
    ed = PIDVar->e0 - 2*PIDVar->e1 + PIDVar->e2;
    out = PIDVar->Kp*ep + PIDVar->Ki*ei + PIDVar->Kd*ed;
    if (out > PIDVar->Scope)	//输出限幅
    {
    	out = PIDVar->Scope;
    }
    else if (out < -PIDVar->Scope)
    {
    	out = -PIDVar->Scope;
    }
    PIDVar->e2 = PIDVar->e1;
    PIDVar->e1 = PIDVar->e0;
    return out;
}

VAR_TYPE PID_PositionCal(PID_VarType *PIDVar)
{
	VAR_TYPE pe, ie, de;
	VAR_TYPE out = 0;

    PIDVar->e0 = PIDVar->Target - PIDVar->Measure;      //计算当前误差

    PIDVar->Sum += PIDVar->e0;       //误差积分

    de = PIDVar->e0 - PIDVar->e1;     //误差微分

    pe = PIDVar->e0;
    ie = PIDVar->Sum;

    PIDVar->e1 = PIDVar->e0;

    out = pe*(PIDVar->Kp) + ie*(PIDVar->Ki) + de*(PIDVar->Kd);
    if (out > PIDVar->Scope) //输出限幅
    {
    	out = PIDVar->Scope;
    }
    else if (out < -PIDVar->Scope)
    {
    	out = -PIDVar->Scope;
    }
    return out;
}
