/**************************************************************************
 * TTPCom Software Copyright (c) 1997-2005 TTPCom Ltd
 * Licensed to TTP_CLIENT_NAME
 **************************************************************************
 *   $Id: //central/releases/simcom/branch_release_16_0_4_B_simcom/tplgsm/kiinc/config.h#1 $
 *   $Revision: #1 $
 *   $DateTime: 2006/11/27 07:18:36 $
 **************************************************************************
 * File Description : Global configuration parameters
 **************************************************************************/

#if !defined (CONFIG_H)
#define       CONFIG_H

/****************************************************************************
 *  WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
 *
 *  TTPCom do not add new items to this file unless they are system wide
 *  and non-Application specific. They should be nothing to do with GSM,
 *  GPRS, 3G, Bluetooth or any other application.
 *
 *  WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
 ***************************************************************************/
/****************************************************************************
 * Definitions
 ****************************************************************************/

/*
** Mechanism for conditional compiling code dependant on PC/Target
** environment. Default is on target.
*/
/* Detect Borland PC compiler */
#if !defined (ON_PC) && defined (__WIN32__)
#   define ON_PC
#endif
/* Detect Microsoft PC compiler */
#if !defined (ON_PC) && defined (WIN32)
//#   define ON_PC
#endif

#endif
/* END OF FILE */
