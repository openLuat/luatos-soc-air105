/**************************************************************************
 *              Copyright (C), AirM2M Tech. Co., Ltd.
 *
 * Name:    iconv.h
 * Author:  liweiqiang
 * Version: V0.1
 * Date:    2013/7/15
 *
 * Description:
 *          �ַ�����ת��
 **************************************************************************/

#ifndef __ICONV_H__
#define __ICONV_H__

#include "stddef.h"

/* Identifier for conversion method from one codeset to another.  */
typedef void *iconv_t;

/* Allocate descriptor for code conversion from codeset FROMCODE to
   codeset TOCODE.  */
extern iconv_t iconv_open_ext (const char *__tocode, const char *__fromcode);

/* Convert at most *INBYTESLEFT bytes from *INBUF according to the
   code conversion algorithm specified by CD and place up to
   *OUTBYTESLEFT bytes in buffer at *OUTBUF.  */
extern size_t iconv_ext (iconv_t __cd, char ** __inbuf,
		     size_t * __inbytesleft,
		     char ** __outbuf,
		     size_t * __outbytesleft);

/* Free resources allocated for descriptor CD for code conversion.  */
extern int iconv_close_ext (iconv_t __cd);

#endif/*__ICONV_H__*/
