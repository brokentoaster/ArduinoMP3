/**

  @file	 	utils.c
  @author 	RA Sewell
  @brief 	Butterfly MP3 utility routines

<pre>
  Target(s)...: ATmega169
  Compiler....: AVR-GCC 3.3.1; avr-libc 1.0
  Revisions...: 1.0

  YYYYMMDD - VER. - COMMENT                                       - SIGN.
  20060304 - 1.0  - Created                                       - KS
 </pre>

 $Id: utils.c,v 1.3 2007/06/04 15:12:17 brokentoaster Exp $

**/


//#include "main.h"
#include "types.h"


/*****************************************************************************
*
*   Function name : strLen
*
*   @return       Length of string
*
*   Purpose :       Find the length of a string (excluding NULL char)
*
*****************************************************************************/
uint16 strLen(uint8 *str)
{
   uint16 len;

   for (len = 0; str[len] != 0x00; len++);

   return (len);
}

/*****************************************************************************
*
*   Function name : strCatChar
*
*   Purpose :       Append a byte to a string buffer
*
*****************************************************************************/
void strCatChar(uint8 *str, uint8 byte)
{
   uint16 len;

   len = strLen(str);
   str[len] = byte;
   str[len+1] = '\0';
}



