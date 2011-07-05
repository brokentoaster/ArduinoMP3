/**
	@file	vs1001.h
	@brief	VS1001 interface library
	@author	Jesper Hansen 
	@date	2000
 
	$Id: vs1001.h,v 1.6 2009/01/10 22:38:51 brokentoaster Exp $
 
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

/* 
 * Modified for Arduino MP3 30/12/2009	
 */
#ifndef __VS1001_H
#define __VS1001_H

#define DREQ_DDR	DDRC
#define BSYNC_DDR	DDRD    
#define xDCS_DDR	DDRD 
#define MP3_DDR		DDRD
#define RESET_DDR	DDRC

#define DREQ_PORT	PINC
#define BSYNC_PORT	PORTD    
#define xDCS_PORT	PORTD    
#define MP3_PORT	PORTD
#define RESET_PORT	PORTC

#define DREQ_PIN	1				// DREQ signal
#define	BSYNC_PIN	2				// BSYNC signal
#define	xDCS_PIN	2				// xDCS signal (new mode)
#define MP3_PIN		3				// MP3 control bit
#define RESET_PIN	0				// -RESET signal

//TODO: add code to use SCI_STATUS to determine 

#define VS1000_NEW					// Use the new mode for VS1053 and newer
//#define VS1001
#define VS1011
//#define VS1053


// defines for Mode control of vs1001k
#ifdef VS1001
#define SM_DIFF 	1
#define SM_FFWD 	2
#define SM_RESET 	4
#define SM_MP12 	8
#define SM_PDOWN 	16
#define SM_DAC 		32
#define SM_DACMONO 	64
#define SM_BASS 	128
#define SM_DACT 	256
#define SM_IBMODE	512
#define SM_IBCLK	1024
#endif

#ifdef VS1011            //Bit  Function Value Description 
#define SM_DIFF         (1<<0)  //  Differential 0 normal in-phase audio 1 left channel inverted 
#define SM_LAYER12      (1<<1)  //  Allow MPEG layers I & II 0 no 1 yes 
#define SM_RESET        (1<<2)  //  Soft reset 0 no reset 1 reset 
#define SM_OUTOFWAV     (1<<3)  //  Jump out of WAV decoding 0 no 1 yes 
#define SM_SETTOZERO1   (1<<4)  //  set to zero 0 right 1 wrong 
#define SM_TESTS        (1<<5)  //  Allow SDI tests 0 not allowed 1 allowed 
#define SM_STREAM       (1<<6)  //  Stream mode 0 no 1 yes 
#define SM_SETTOZERO2   (1<<7)  //  set to zero 0 right 1 wrong 
#define SM_DACT         (1<<8)  //  DCLK active edge 0 rising 1 falling 
#define SM_SDIORD       (1<<9)  //  SDI  bit order 0 MSb ﬁrst 1 MSb last 
#define SM_SDISHARE     (1<<10) //  Share SPI chip select 0 no 1 yes 
#define SM_SDINEW       (1<<11) //  VS1002 native SPI modes 0 no 1 yes 
#define SM_SETTOZERO3   (1<<12) //  set to zero 0 right 1 wrong 
#define SM_SETTOZERO4   (1<<13) //  set to zero 0 right 1 wrong 
#endif 

#ifdef VS1053            //Bit  Function Value Description 
#define SM_DIFF         (1<<0)  //  Differential 0 normal in-phase audio 1 left channel inverted 
#define SM_LAYER12      (1<<1)  //  Allow MPEG layers I & II 0 no 1 yes 
#define SM_RESET        (1<<2)  //  Soft reset 0 no reset 1 reset 
#define SM_CANCEL       (1<<3)  //  Cancel decoding current ﬁle 0 no 1 yes 
#define SM_EARSPEAKER_LO (1<<4) //  EarSpeaker low setting  0 off 1 active 
#define SM_TESTS        (1<<5)  //  Allow SDI tests 0 not allowed 1 allowed 
#define SM_STREAM       (1<<6)  //  Stream mode 0 no 1 yes 
#define SM_EARSPEAKER_HI (1<<7) //  EarSpeaker high setting  0 off 1 active 
#define SM_DACT         (1<<8)  //  DCLK active edge 0 rising 1 falling 
#define SM_SDIORD       (1<<9)  //  SDI  bit order 0 MSb ﬁrst 1 MSb last 
#define SM_SDISHARE     (1<<10) //  Share SPI chip select 0 no 1 yes 
#define SM_SDINEW       (1<<11) //  VS1002 native SPI modes 0 no 1 yes 
#define SM_ADPCM        (1<<12) //  ADPCM recording active 0 no 1 yes 
#define SM_LINE1        (1<<14) //  MIC / LINE1 selector 0 MICP 1 LINE1
#define SM_CLK_RANGE    (1<<15) //  Input clock range 0 12..13 MHz 
#endif

// defines for SCI registers
#ifdef VS1001
#define SCI_MODE		0
#define SCI_STATUS		1
#define SCI_INT_FCTLH	2
#define SCI_CLOCKF		3
#define SCI_DECODE_TIME 4
#define SCI_AUDATA		5
#define SCI_WRAM		6
#define SCI_WRAMADDR	7
#define SCI_HDAT0		8
#define SCI_HDAT1		9
#define SCI_AIADDR		10
#define SCI_VOL			11
#endif

#if defined (VS1011) || defined (VS1053)
//                      Reg           Type Reset Time1    Description 
#define SCI_MODE        0x0         // rw  0     70 CLKI4 Mode control 
#define SCI_STATUS      0x1         // rw  0x2C3 40 CLKI  Status of VS1011e 
#define SCI_BASS        0x2         // rw  0   2100 CLKI  Built-in bass/treble enhancer 
#define SCI_CLOCKF      0x3         // rw  0     80 XTALI Clock freq + multiplier 
#define SCI_DECODE_TIME 0x4         // rw  0     40 CLKI  Decode time in seconds 
#define SCI_AUDATA      0x5         // rw  0   3200 CLKI  Misc. audio data 
#define SCI_WRAM        0x6         // rw  0     80 CLKI  RAM write/read 
#define SCI_WRAMADDR    0x7         // rw  0     80 CLKI  Base address for RAM write/read 
#define SCI_HDAT0       0x8         // r   0       -      Stream header data 0 
#define SCI_HDAT1       0x9         // r   0       -      Stream header data 1 
#define SCI_AIADDR      0xA         // rw  0   3200 CLKI2 Start address of application 
#define SCI_VOL         0xB         // rw  0   2100 CLKI  Volume control 
#define SCI_AICTRL0     0xC         // rw  0     50 CLKI2 Application control register 0 
#define SCI_AICTRL1     0xD         // rw  0     50 CLKI2 Application control register 1 
#define SCI_AICTRL2     0xE         // rw  0     50 CLKI2 Application control register 2 
#define SCI_AICTRL3     0xF         // rw  0     50 CLKI2 Application control register 3
#endif

typedef enum {
	SOFT_RESET,
	HARD_RESET
} reset_e;

class vs1001
    {
    public:


    // setup I/O pins and directions for
    // communicating with the VS1001
    void init_io(void);

    // setup the VS1001 chip for decoding
    void init_chip(void);

    // reset the VS1001
    void reset(reset_e r);

    // send a number of zero's to the VS1001
    void nulls(unsigned int nNulls);

    void read(uint8 address, uint16 count, uint16 *pData);

    //
    // write one or more word(s) to the VS1001 Control registers
    //
    void write(uint8 address, uint16 count, uint16 *pData);

    void setvolume(unsigned char left, unsigned char right);

    // send MP3 data
    void send_data(unsigned char b);
    void send_32(unsigned char *p);

    // test with beeps
    void sine_test(void);
            
    };


#endif

