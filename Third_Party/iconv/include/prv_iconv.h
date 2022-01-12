/**************************************************************************
 *              Copyright (C), AirM2M Tech. Co., Ltd.
 *
 * Name:    prv_iconv.h
 * Author:  liweiqiang
 * Version: V0.1
 * Date:    2013/7/15
 *
 * Description:
 *          �ַ�����ת���ڲ������ļ�
 **************************************************************************/

#ifndef __PRV_ICONV_H__
#define __PRV_ICONV_H__


typedef size_t (*iconv_fct) (char ** __inbuf,
                      size_t * __inbytesleft,
                      char ** __outbuf,
                      size_t * __outbytesleft);


#endif/*__PRV_ICONV_H__*/
