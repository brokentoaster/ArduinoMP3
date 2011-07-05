/**
	@file	vs1001.c
	@brief	VS1001 interface library
	@author	Jesper Hansen 
	@date	2000
 
	$Id: vs1001.c,v 1.11 2009/01/10 22:38:51 brokentoaster Exp $
 
  Copyright (C) 2000 Jesper Hansen <jesperh@telia.com>.

  This file is part of the yampp system.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation, 
  Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
**/

/**
 * Modified on 13/07/2004 for ButterflyMP3 project ( atMega169 )
 * by Nick Lott.
 **/

#include "WProgram.h"
#include "types.h"
#include "vs1001.h"


//
// VS1001 commands
//
#define VS1001_READ	    0x03
#define VS1001_WRITE	0x02

    


uint8 write_byte_spi(uint8 data)
{
	uint8 i,dummy;
    
	SPDR = data;              	 	// put byte to send in SPDR, which initiates xmit  
	
    loop_until_bit_is_set(SPSR, SPIF); 	//wait for completion 
	
	//delay(5); //wait 5 microseconds after sending data to control port
	for (i=0;i<8;i++) // 2 + 5i clock cycles -> 42 clocks at 8 Mhz -> 5.25uS
		asm volatile("nop");
    
	i = SPSR;	// clear status
	dummy = SPDR;
	return dummy;               	// return with byte shifted in from slave
}	






///
/// read one or more word(s) from the VS1001 Control registers
///
void vs1001::read(uint8 address, uint16 count, uint16 *pData)
{
	uint8 i;

#ifdef VS1000_NEW
	SBI( xDCS_PORT, xDCS_PIN ); 	// xDCS hi
#else
	CBI( BSYNC_PORT, BSYNC_PIN ); 	// byte sync lo
#endif
	
	CBI( MP3_PORT, MP3_PIN);	// xCS lo
	write_byte_spi(VS1001_READ);
	write_byte_spi(address);

	while (count--)
	{
		*pData = write_byte_spi(0) << 8;
		*pData++ |= write_byte_spi(0);
	}

	SBI( MP3_PORT, MP3_PIN);	// xCS hi

	//this is absolutely neccessary!
	//delay(5); //wait 5 microseconds after sending data to control port
	for (i=0;i<8;i++)
		asm volatile("nop");

    
}



///
/// write one or more word(s) to the VS1001 Control registers
///
void vs1001::write(uint8 address, uint16 count, uint16 *pData)
{
	uint8 i;
	
#ifdef VS1000_NEW
	SBI( xDCS_PORT, xDCS_PIN );	// xDCS hi
#else
	CBI( BSYNC_PORT, BSYNC_PIN );	// byte sync lo
#endif	
	
	CBI( MP3_PORT, MP3_PIN);	// xCS lo
	
	write_byte_spi(VS1001_WRITE);
	write_byte_spi(address);

	while (count--)
	{
		write_byte_spi((uint8)((*pData) >> 8));
		write_byte_spi((uint8)*pData);
		pData++;
	}
	
	SBI( MP3_PORT, MP3_PIN);	// xCS hi

	//this is absolutely neccessary!
	//delay(5); //wait 5 microseconds after sending data to control port
	for (i=0;i<16;i++)
		asm volatile("nop");

    //Note: VS1011e sets DREQ low after each SCI operation. The duration depends on the operation. It is 
    // not allowed to start a new SCI/SDI operation before DREQ is high again. 
	loop_until_bit_is_set(DREQ_PORT, DREQ_PIN);  
}

/****************************************************************************
**
** MPEG Data Stream
**
****************************************************************************/


/* !!! WARNING !!!
	
ALL data on the spi lines is read as
input to the MPEG Stream by the vs1001 when :
-the BSYNC line is high and compiled in standard (vs1001 compatability) mode.
-the xDCS line is low in new (VS1002) mode.

If in New mode 

*/

///
/// send a byte to the VS1001 MPEG stream
///
void vs1001::send_data(unsigned char b)
{
	char i;
#ifdef VS1000_NEW
	CBI( xDCS_PORT,   xDCS_PIN );		//  XDCS lo
#else
	SBI( BSYNC_PORT,   BSYNC_PIN ); 	// byte sync hi
#endif
	//	outp(b, SPDR);			// send data
	SPDR = b;				// send data
	
	// release BSYNC before end of byte
#ifndef VS1000_NEW
	asm volatile("nop");
	asm volatile("nop"); 
	asm volatile("nop"); 
	CBI( BSYNC_PORT,   BSYNC_PIN ); 	// byte sync lo
#endif	
	// wait for data to be sent
	loop_until_bit_is_set(SPSR, SPIF); 
	
	//release xDCS after byte has been sent
#ifdef VS1000_NEW
	SBI( xDCS_PORT,   xDCS_PIN );		// byte XDCS hi
#endif
	i = SPDR; 				// clear SPIF
	

}


///
/// send a burst of 32 data bytes to the VS1001 MPEG stream
///
void vs1001::send_32(unsigned char *p)
{
	int j;

	
#ifdef VS1000_NEW
	CBI( xDCS_PORT,   xDCS_PIN ); 		// xDCS lo
#else
	SBI( BSYNC_PORT,   BSYNC_PIN ); 	// byte sync hi
#endif
	for (j=0;j<31;j++) 
	{
		SPDR = *p++ ;		// send data
		// wait for data to be sent
		loop_until_bit_is_set(SPSR, SPIF);   
	}
		
	SPDR = *p++ ;		// send last byte
		
#ifndef VS1000_NEW
	// release BSYNC before last bit of last byte.
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop"); 
	CBI( BSYNC_PORT,   BSYNC_PIN ); 	// byte sync lo
#endif	
	// wait for data to be sent
	loop_until_bit_is_set(SPSR, SPIF);   

#ifdef VS1000_NEW
	SBI( xDCS_PORT,   xDCS_PIN );		// xDCS hi
#endif
	j = SPDR; // clear SPIF
}



/****************************************************************************
**
** Init and helper functions
**
****************************************************************************/


/// setup I/O pins and directions for
/// communicating with the VS1001
void vs1001::init_io(void)
{
    char dummy;
#ifdef VS1000_NEW
	// setup xDCS (same as pin as BSYNC AFAIK)
	SBI( xDCS_DDR , xDCS_PIN );		// pin is output for xDCS 
	SBI( xDCS_PORT, xDCS_PIN );		// output High
    
     //send a pulse to ensure BSYNC Counter has been reset
    CBI( xDCS_PORT, xDCS_PIN );		// output Low
    SBI( xDCS_PORT, xDCS_PIN );		// output High
#else
	// setup BSYNC
	SBI( BSYNC_DDR , BSYNC_PIN );		// pin is output for BSYNC
	CBI( BSYNC_PORT, BSYNC_PIN );		// output low

    //send a pulse to ensure BSYNC Counter has been reset
    SBI( BSYNC_PORT, BSYNC_PIN );		// output High
    CBI( BSYNC_PORT, BSYNC_PIN );		// output low
#endif
	// set the MP3/ChipSelect pin hi
	SBI( MP3_DDR , MP3_PIN); 		// pin output for xCS
	SBI( MP3_PORT, MP3_PIN); 		// output hi (deselect MP3)

	// set the /Reset pin hi
	SBI( RESET_DDR , RESET_PIN); 		// pin output 
	SBI( RESET_PORT, RESET_PIN); 		// output hi

	// Setup DREQ Pin
	CBI(DREQ_DDR , DREQ_PIN); 		// pin input
	CBI(DREQ_PORT, DREQ_PIN);		// no pullup

    // Setup the SPI Hardware ( often done in MMC libs, may confict)
    SPCR = (0<<SPIE)|(1<<SPE)|(0<<DORD)|(1<<MSTR)|(0<<CPOL)|(0<<CPHA)|(0<<SPR1)|(1<<SPR0);     
	SPSR = (1<<SPI2X);
    // SPEED = FOSC/8 =>  arduino =>2MHZ
	dummy = SPSR;	// clear status
	dummy = SPDR;
}


/// setup the VS1001 chip for decoding
void vs1001::init_chip(void)
{
	//we use a hardware reset, works much better 
	//than software rest, but makes a click noise.

	delay(3);
	vs1001::reset(HARD_RESET);

	delay(100);
	vs1001::nulls(32);
	vs1001::reset(SOFT_RESET);

   // vs1001::sine_test();
}



/// reset the VS1001
void vs1001::reset(reset_e r)
{

	uint16 buf[2];

	if (r == SOFT_RESET)
	{
		
	//	delay(200);		// 200 mS
		SPSR = (0<<SPI2X);			//set spi to Fosc/4
		
		// set SW reset bit	
		buf[0] = SM_RESET ;
		vs1001::write(SCI_MODE,1,buf);	// set bit 2

		delay(2);		// 2 mS

		while( !((DREQ_PORT) & (1<<DREQ_PIN)) ); //wait for DREQ
	    
        CBI( MP3_PORT, MP3_PIN); 		// output low (select MP3)
#ifdef VS1000_NEW        
        //send a pulse to ensure BSYNC Counter has been reset
        CBI( xDCS_PORT, xDCS_PIN );		// output Low
        SBI( xDCS_PORT, xDCS_PIN );		// output High
#else
        //send a pulse to ensure BSYNC Counter has been reset
        SBI( BSYNC_PORT, BSYNC_PIN );		// output High
        CBI( BSYNC_PORT, BSYNC_PIN );		// output low
#endif
        SBI( MP3_PORT, MP3_PIN); 		// output hi (deselect MP3)
        
#ifdef VS1000_NEW
        buf[0] = SM_SDINEW ; 
        vs1001::write(SCI_MODE,1, buf);
#endif
        // set CLOCKF for 24.576 MHz
		// change to doubler //nick 7/7/04
		buf[0] = 0x9800;
		vs1001::write(SCI_CLOCKF,1,buf);	
   		vs1001::write(SCI_CLOCKF,1,buf);	
#ifdef VS1001
		// Force clock doubler see pg32 of VS10XX appl.notes
		buf[0] = 0x8008;
		vs1001::write(SCI_INT_FCTLH,1,buf);
#endif
        while( !((DREQ_PORT) & (1<<DREQ_PIN)) ); //wait for DREQ
        
		vs1001::nulls(32);
	    
		SPSR = (1<<SPI2X);			//set spi to Fosc/2
	}
	else if (r == HARD_RESET)
	{
		CBI(RESET_PORT, RESET_PIN);	// RESET- lo
		delay(1);	// 1 mS	    
		SBI(RESET_PORT, RESET_PIN);	// RESET- hi
		delay(5);	// 5 mS	    
	}
}

///
/// send a number of zero's to the VS1001
///
void vs1001::nulls(unsigned int nNulls)
{
	unsigned int n;
    n = nNulls;
    while (n--){
		vs1001::send_data(0);
    }
}

///
/// Set the VS1001 volume
///
void vs1001::setvolume(unsigned char left, unsigned char right)
{
	uint16 buf[2];

	buf[0] = (((uint16)left) << 8) | (uint16)right;

	vs1001::write(SCI_VOL, 1, buf);
}
	
///
/// send a sine test (5 beeps)
///
void vs1001::sine_test(void)
{
    uint16 buf[2];
    int i,j ;
    // used for the sine_test function
    const uint8  SINETESTSTART[8]  = {0x53,0xef,0x6e,0x22,0,0,0,0};//1KHZ
    const uint8  SINETESTSTOP[8]   = {0x45,0x78,0x69,0x74,0,0,0,0};
    
    	
    buf[0]=0x0800;
    
#ifdef VS1000_NEW
//    • Set New Mode by writing four SCI bytes: 0x2, 0x0, 0x8, 0x20 
    buf[0] |= SM_SDINEW ;
#endif
#if defined (VS1011) || defined(VS1053)
    buf[0] |= SM_TESTS;
#endif
    
    vs1001::write(SCI_MODE,1,buf);
    
     while( !((DREQ_PORT) & (1<<DREQ_PIN)) ); //wait for DREQ
    
    for(j=0; j<5;j++){
    //  • Activate the sine test by writing the following eight bytes to SDI: 0x53 0xEF 0x6E 0x22, 0, 0, 0, 0
        for (i=0;i<8;i++) {
            vs1001::send_data( SINETESTSTART [i] );   
        }  
    //  • Wait for example 500 ms. 
        delay(500);
                             
    //  • Deactivate the sine test by writing to SDI: 0x45 0x78 0x69 0x74. 
        for (i=0;i<8;i++){
            vs1001::send_data( SINETESTSTOP[i] );   
         }          
                             
    //  • Wait for example 500 ms. 
         delay(500);                    
    }
    
}
