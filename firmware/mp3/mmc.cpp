/**
	@file	mmc.c
	@brief	MultiMedia Card low level Functions
	@author	Nick Lott brokentoaster@users.sf.net
	@date	September 2004
 
	$Id: mmc.c,v 1.18 2009/01/10 22:40:12 brokentoaster Exp $

	This file represents a convergence of a number of code snippets found 
	on the web, some of the Yampp system by Jesper Hansen and the work
	done by Sylvain.Bissonnette@microsyl.com. The goal is to produce an 
	adaptable library for doing low level MMC activities over the SPI bus.
	This code was written with the Atmega169V in mind (aka "Butterfly"). 
	http://butterflymp3.sf.net
	
	For details Concerning the MMC spec see www.sandisk.com. I refered to 
	"SanDisk MultiMediaCard and Reduced-Size MultiMediaCard Product Manual"
	Doc No. 80-36-00320 v1.0 during the writing of this code.
	
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
	
History:

24-26 Sep 2004:
	Initial write and port of functions collected over last few 
	months 
	
March 2005:		
	Add new improvements based around Circuit cellar article.
	Updated Comments for Doxygen

December 2009:
    Adapted fr Arduino Library
**/

#include "WProgram.h"

#include "mmc.h"



//#include "delay.h"
//#include "main.h"
//#include "uart.h"
//#include "vs1001.h" 


//unsigned char MMC::mmc_sbuf[512];	///< 512 byte sector buffer in internal RAM
//unsigned char MMC::mmc_scratch[32]; 	///< 32 byte Scratch buffer for CSD/CID/ ops


MMC::MMC(void)
{
    SpiInit();
    
}

/**
*	Setup Pin configuration of SPI bus for the AVR
**/
void MMC::SpiInit(void)
{
	char dummy;
	
	SBI(SPI_DDR, SPI_SS);		// SS must be output for Master mode to work
	SBI(SPI_DDR, SPI_MOSI);		// set MOSI a output
	SBI(SPI_DDR, SPI_SCK);		// set SCK as output
	CBI(SPI_DDR, SPI_MISO); 	// MISO is input
	//CBI(SPI_PORT, SPI_SCK);		// set SCK low
	
	SBI(MMC_PORT,MMC_CS);
	SBI(MMC_DDR,MMC_CS);
	
	/* enable SPI as master, set clk divider fosc/4 */
	// Although the MMC card is capable of clock speeds up to 20Mhz
	// We are limited to 2Mhz for the VS1001k until we set the clock doubler 
	// after which we are limited to 4Mhz by the butterfly.
	/* later we will enable 2X mode once vS1001 has been initialized.*/
//	SPCR = (0<<SPIE)|(1<<SPE)|(0<<DORD)|(1<<MSTR)|(0<<CPOL)|(0<<CPHA)|(0<<SPR1)|(0<<SPR0);     
	SPCR = (0<<SPIE)|(1<<SPE)|(0<<DORD)|(1<<MSTR)|(0<<CPOL)|(0<<CPHA)|(0<<SPR1)|(0<<SPR0);     
	SPSR = (0<<SPI2X);
	dummy = SPSR;	// clear status
	dummy = SPDR;
}


/**
* Main SPI routine
*  - transmits a byte and receives a byte simultaneously
*  - received byte is returned
*  - if you only want to read a byte, put a dummy
*    (say 0xff) in the transmit slot
*
*	@param byte Byte to transmit
* 	@return byte clocked in during transmit
**/
unsigned char MMC::SpiByte(unsigned char byte)
{
	char i,dummy;

	SPDR = byte;              	 	// put byte to send in SPDR, which initiates xmit  
	
	loop_until_bit_is_set(SPSR, SPIF); 	//wait for completion 
	
	//delay(5); //wait 5 microseconds after sending data to control port
	for (i=0;i<8;i++) // 2 + 5i clock cycles -> 42 clocks at 8 Mhz -> 5.25uS
		asm volatile("nop");
		
	i = SPSR;	// clear status
	dummy = SPDR;
	return dummy;               	// return with byte shifted in from slave
	
}


/**
*	Retrieve data from then MMC.
*	Pings the card until it gets a non-0xff value
*
*  	@return first byte of data usualy a data token.
**/
unsigned char MMC::get(void)
{
	unsigned char i,bByte;
	
	for (i=0;i<8;i++){                      
        bByte = MMC::SpiByte(0xff);
//        mmc_scratch[0]=bByte;
//        mmc_scratch[1]=42;
        if (bByte != 0xff) return bByte;
	}
	
	return bByte;
}

/**
* 	Send a control command to the MMC.
*	send one byte of 0xff, then issue command + params + (fake) crc
* 	eat up the one command of nothing after the CRC
*   Clears the MMC_CS and BSYNC_PIN lines.
*	MMC must be de-selected outside of this code !!
*
* 	@param command 	MMC Command to be sent
* 	@param px		first byte of command parameters
* 	@param py		second byte of command parameters
* 	@param pz		third byte of command parameters
*
**/
void MMC::command(unsigned char command_byte, char px, char py, char pz)
{
	CBI(MMC_PORT,MMC_CS);	// Select card
//	CBI( BSYNC_PORT,   BSYNC_PIN ); 	// byte sync lo

    MMC::SpiByte(0xff);
	MMC::SpiByte(command_byte | 0x40);
	MMC::SpiByte(px);
	MMC::SpiByte(py);
	MMC::SpiByte(pz);
	MMC::SpiByte(0x00);
	MMC::SpiByte(0x95);            
	/* 
	correct CRC for first command in SPI          
	after that CRC is ignored, so no problem with 
	always sending 0x95
    */
	/*
	 the reason I did not re set the MMC_CS line was because I didn't want to 
	 de-assert the line after the command while waiting for a response.  
	 The card is selected here as a backup to make sure it is selected. 
	 I might go through and remove places where it is redundant when 
	 i get a chance.  
		
	 <<!! the following behaviour has been removed as redundent
		I may add it back in a new mode compatable form if needed.
	 
	The BSYNC line is held low to ensure that the spi data is sent as a command 
	 to the MMC and not as data to teh vs1001 as I have combinned both the 
	 interfaces on the vs1001 to save pins. 
	 
	 !!>>
		
	Any data on the spi lines is read as data by the vs1001 when the
	 BSYNC line is high.
	*/
}


/**
*	Perform a Hardware then software reset of the MMC.
*
*	@return 00 if reset successful
*	@return FF if reset unsuccessful
**/
char MMC::reset(void)			
{
	unsigned char bByte;
	unsigned int i;

	SBI(MMC_PORT,MMC_CS);
	//CBI( BSYNC_PORT,   BSYNC_PIN ); 	// byte sync lo
	
	/* start off with 80 bits of high data with card deselected */
	for(i=0;i<10;i++)
		 MMC::SpiByte(0xff);
		
	/* now send CMD0 - go to idle state, try up to 100 times */
    MMC::command(0,0,0,0);  /* software reset command */
    
	if ((bByte = MMC::get() ) != 1)
	  {
		  SBI(MMC_PORT ,MMC_CS);
		  MMC::SpiByte(bByte);
		  return -1;  // MMC Not detected
	  }
	
	 
	/* send CMD1 until we get a 0 back, indicating card is done initializing */
	i =0xffff;    /* but only do it up to 1000 times */
	bByte = 1;
	
	while (bByte && i--){
	   MMC::command(1,0,0,0);
        bByte = MMC::get();
	   }
	
	SBI(MMC_PORT,MMC_CS);
	MMC::SpiByte(0xff); // Clear SPI
	
	if (bByte){
	  return -2;  // Init Fail
	  }
	  

	return 0;
}

/**
 *	Checks if there is a memory card in the slot
 *	This requires a pull down resistor on the CS 
 *  line of about 100K.
 *
 *	@return	FALSE		card not detected
 *	@return TRUE		card detected
 **/
char MMC::detect(void)
{
	char i;
	#if (REV >='B') // MMC detect only available on rev_b boards.
	MMC_DDR &= ~(1<<MMC_CS); // set as input
	MMC_PORT &= ~(1<<MMC_CS); // disable pullup
	
	//wait 18 microseconds after sending data to control port
	for (i=0;i<29;i++) // 2 + 5i clock cycles -> 147 clocks at 8 Mhz -> 18.375uS
		asm volatile("nop");
		
	if((MMC_PIN & (1<<MMC_CS)) == 0)	
	{
		MMC_PORT |= (1<<MMC_CS); 
		MMC_DDR |= (1<<MMC_CS);
		return FALSE;
	}
	MMC_PORT |= (1<<MMC_CS); 
	MMC_DDR |= (1<<MMC_CS);	
	#endif
	return TRUE;
}


/**
*	Send the get status command to the MMC and returns the 
*	result
*
*	@return 2 byte value from MMC status register
**/
int MMC::check(void)
{
	int word;

	command(MMC_SEND_STATUS,0,0,0); // check card ststus
	word = MMC::get();
	word = (word<<8) + MMC::SpiByte(0xff); // NOTE spibyte and not MMCGET!!!
	
	SBI(MMC_PORT,MMC_CS); // Deselect card
    MMC::SpiByte(0xff); // Clear SPI
	
	return word;
}

/**
*	Grab the Serial number & info from the card.
*	Returns status response from card.
*	if successful CID is in mmc_scratch[1..16]
*	
*	@return byte response from the CID command
*
**/

unsigned char MMC::identify(void)
{
	unsigned char byte,data,i;
	

	command(MMC_SEND_CID,0,0,0); // send cmd
	byte = MMC::get(); // check reponse
	
	if (byte==0){ // get the CID...
		for (i=0;i<32;i++){
			data = MMC::SpiByte(0xff); 
			mmc_scratch[i]=data;
		}
	}

	SBI(MMC_PORT,MMC_CS); // Deselect card
    MMC::SpiByte(0xff); // Clear SPI
	
	return byte;
}


/**
*	Retrieves the CSD Register from the mmc and stores it in
*	mmc_scratch[1..17]
*
*	@return		Status response from cmd
**/

unsigned char MMC::cardType(void)
{
	unsigned char byte,data,i;
	
	command(MMC_SEND_CSD,0,0,0); // send cmd
	byte = MMC::get(); // check reponse
	
	if (byte==0){ // get the CID...
		for (i=0;i<32;i++){
			data = MMC::SpiByte(0xff); 
			mmc_scratch[i]=data;
		}
	}

	SBI(MMC_PORT,MMC_CS); // Deselect card
	MMC::SpiByte(0xff); // Clear SPI
	
	return byte;
}


/**
*	Calculates the capacity of the MMC in blocks
*
*	@return	uint32 capacity of MMC in blocks or -1 in error;
**/

unsigned long  MMC::capacity(void)
{
	char byte,data,multi,blk_len;
	unsigned long  c_size;
	unsigned long  capacity;
	byte = MMC::cardType();
	if (byte==0) {// got info okay
		blk_len = 0x0F & mmc_scratch[6]; // this should equal 9 -> 512 bytes
		/*	; get size into reg 
			;	  7				8			9
			; xxxx xxxx    xxxx xxxx    xxxx xxxx
			;        ^^    ^^^^ ^^^^    ^^ 
		*/
		data =(mmc_scratch[7] & 0x03)<<6;
		data |= (mmc_scratch[8] >> 2);
		c_size = data << 4;
		data =(mmc_scratch[8] << 2 ) | ((mmc_scratch[9] & 0xC0)>>6 );
		c_size |= data;
		
		/*	; get multiplier
			;	10			11
			; xxxx xxxx    xxxx xxxx
			;        ^^    ^
		*/
		multi = ((mmc_scratch[10] & 0x03 ) << 1 ) ;
		multi |= ((mmc_scratch[11] & 0x80) >> 7);
		// sectors = (size+1)<<(multiplier+2)
		capacity = (c_size + 1)<<(multi + blk_len + 2);

		return capacity;
	}else if (byte==1){
        blk_len = 0x0F & mmc_scratch[6]; // this should equal 9 -> 512 bytes
		/*	; get size into reg 
         ;	  7				8			9
         ; xxxx xxxx    xxxx xxxx    xxxx xxxx
         ;        ^^    ^^^^ ^^^^    ^^ 
         */
		data =(mmc_scratch[7] & 0x03)<<6;
		data |= (mmc_scratch[8] >> 2);
		c_size = data << 4;
		data =(mmc_scratch[8] << 2 ) | ((mmc_scratch[9] & 0xC0)>>6 );
		c_size |= data;
		
		/*	; get multiplier
         ;	10			11
         ; xxxx xxxx    xxxx xxxx
         ;        ^^    ^
         */
		multi = ((mmc_scratch[10] & 0x03 ) << 1 ) ;
		multi |= ((mmc_scratch[11] & 0x80) >> 7);
		// sectors = (size+1)<<(multiplier+2)
		capacity = (c_size + 1)<<(multi + blk_len + 2);
        
		return capacity;
        
    }
	return 0xFFFFFFFF;
}


/**
*	Read the OEM/Manufcatures MMC Name into the scratchpad.
*
*	@return		status of MMC from SEND_CID cmd.
**/
unsigned char MMC::name(void)
{
	unsigned char byte,i;
	
	byte = MMC::identify(); // Grab CID into mmc_scratch 

	if (byte==0){
		for (i=0;i<6;i++)
			mmc_scratch[i]=mmc_scratch[i+4];
		mmc_scratch[6]=0;
	}
	
	return byte;
}
    
	
/***************************************************************************
*	MMC_Read	
*	Read one sector from the memory card into mmc_sbuf[]
*	Note this assumes block length of 512 Bytes
*	@Param		<lba> the number of the sector to read
*	@Return		Status from the read call.
***************************************************************************/
unsigned char MMC::read(unsigned long lba)
{
	unsigned char bbyte,dta,px,py,pz;
	unsigned int sec,i;
	unsigned int word;
#ifdef USE_MMC_CACHE
	static uint32 last_read_lba=0xffffffff; // remember last sector for caching purposes

	if (last_read_lba == lba){ // exit ASSUMING BUFFER HAS NOT BEEN ALTERED
		return 0x00; // OK
	}
#endif
	
	MMC::SpiByte(0xff);
	CBI(MMC_PORT,MMC_CS);	// Select card
	command(MMC_SEND_STATUS,0,0,0); // check card status
	word = MMC::get();
	word = (word<<8) + MMC::SpiByte(0xff);
	
	
	lba <<=9; // * 512
	pz = (lba>>8 & 0xff);
	py =( lba>>16 & 0xff);
	px =( lba>>24 & 0xff);
	
	MMC::SpiByte(0xff);
	CBI(MMC_PORT,MMC_CS);	// Select card
	command(MMC_READ_SINGLE_BLOCK, px , py, pz);
	bbyte = MMC::get(); // check reponse
	
	//TODO:  if i get a byte == 0x05 should I resend?

    
	if (bbyte==0){ // then everything okay
		i=500; // wait up to 255 bytes for response
		dta = MMC::SpiByte(0xff);
		
		while ((dta != MMC_STARTBLOCK_READ) && (i--)){
			dta = MMC::SpiByte(0xff);
		}
        
		if (i!=0){ // read okay. start grabbing the data
			for(sec=0;sec<512;sec++)
				MMC::mmc_sbuf[sec] = MMC::SpiByte(0xff);
			MMC::SpiByte(0xff); // flush MMC CRC 
			MMC::SpiByte(0xff); // 
			MMC::SpiByte(0xff); //Clear SPI
			MMC::SpiByte(0xff); //
		}else{
			bbyte = 0xff;// signal MMC Coms error
		}
	//}else{
	//	PRINT("MMC_READ_SINGLE_BLOCK\t");
	//	PRINT("\t");UART_Printfu32(lba<<9);
	//	PRINT("\t");UART_Printfu16(word);
	//	PRINT("\t");UART_Printfu08(byte);
	//	PRINT("\t");UART_Printfu08(SpiByte(0xff));
	//	EOL();
		
	}

    
	SBI(MMC_PORT,MMC_CS); // Deselect card
	MMC::SpiByte(0xff); // Clear SPI

#ifdef USE_MMC_CACHE
	if (bbyte == 0){
			last_read_lba = lba;// save for next time.
	}
#endif
	return bbyte;
}

 /*************************************************************************
 *	char MMCWriteSector(uint32lba)
 *
 * - writes a sector to the card (512 bytes)
 * - takes sector # as param
 * - data is in mmc_sbuf[] 
 * - returns success/failure status of operation.
 **************************************************************************/
 // ****** UNTESTED ******
//char MMC_Write(uint32 lba)
//{
//	unsigned char px,py,pz;
//	unsigned int i;
//	lba <<=1;
//	pz = (lba & 0xff);
//	lba >>= 8;
//	py =( lba & 0xff);
//	lba >>= 8;
//	px =( lba & 0xff);
//	
//	CBI(MMC_PORT,MMC_CS);	// Select card
////	command(MMC_WRITE_BLOCK, (lba>>7)& 0xffff, (lba<<9)& 0xffff);
//	if (MMCMMC::get() == 0xff) return 0xff;
//	
//	SpiByte(0xfe);  // Send Start Byte
//	
//	for (i=0;i<512;i++)       // read the sector 
//	   {
//		SpiByte(mmc_sbuf[i]);
//	   }
//	SpiByte(0xff);          // checksum -> don't care about it for now 
//	SpiByte(0xff);       // checksum -> don't care about it for now 
//	SpiByte(0xff);       // Read "data response byte"                 
//	
//	if (MMCGet() == 0xff) return 0xff;
//	
//	SBI(MMC_PORT,MMC_CS); // Deselect card
//	SpiByte(0xff); // Clear SPI
//	
//	return 0;
//}

//void MMC_Flush(void)
//{
//	char i;
//	SBI(MMC_PORT,MMC_CS);
//	/* start off with 80 bits of high data with card deselected */
//	for(i=0;i<10;i++)
//		SpiByte(0xff);
//	//CBI(MMC_PORT,MMC_CS);        /* select card */
//}


/***************************************************************************
*   Name:			MMC_WriteProtect
*	Description:	Set or clear the MMC Writeprotect
*	Parameters:		<set> determines weather to set or clear the write protection
*	Returns:		error code.
***************************************************************************/
unsigned char MMC::writeProtect(unsigned char set){
	unsigned char byte;
	
	if (set){
        MMC::command(MMC_SET_WRITE_PROT,0,0,0); // send cmd
	}else{
		MMC::command(MMC_CLR_WRITE_PROT,0,0,0); // send cmd
	}
	byte = MMC::get(); // check reponse

	SBI(MMC_PORT,MMC_CS); // Deselect card
    MMC::SpiByte(0xff); // Clear SPI
	
	return byte;
}
