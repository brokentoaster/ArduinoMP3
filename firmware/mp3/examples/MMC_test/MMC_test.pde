//
// 
// Arduino MP3 Shield SD/MMC test file
//
//


/*
Copyright (C) 2010, 2011 Nick Lott <brokentoaster@users.sf.net>
http://www.brokentoaster.com/

This example program is based on software developed for the 
Butterfly MP3 Project.

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
*/

#define BAUD 115200
#define FAT_NumberedSong_EN 1

#include <utils.h>
#include <mmc.h>
#include <avrfat16.h>

MMC     card;                           ///< The physical MMC/SD device
FAT16   vol;                            ///< The Logical file system

// These are the pins we'll be using 
char mp3_cs     = 3;
char mp3_bsync  = 2;
char mp3_dreq   = 15;
char mp3_reset  = 14;
char debugpin   = 8;


//////////////////////////////////// SETUP
void setup()
{
        char r;                 // byte variable to hold the result from various function calls
        long rst_attempts;      // number of times rest is attempted with no response from card
        
        pinMode(debugpin,OUTPUT);           // setup debug trigger for a scope
        digitalWrite(debugpin, HIGH);       // .. or logic analyser

        digitalWrite(mp3_dreq, HIGH);       // turn on pullup resistors
        digitalWrite(mp3_bsync, LOW);       // turn off BSYNC
        digitalWrite(mp3_reset, LOW);       // turn off VS1003 
        digitalWrite(mp3_cs, HIGH);         // turn off VS10xx SPI

        pinMode(mp3_dreq, INPUT);           // set pin to input
        pinMode(mp3_bsync,OUTPUT);          // set pin to output
        pinMode(mp3_reset,OUTPUT);          // set pin to output
        pinMode(mp3_cs,OUTPUT);             // set pin to output

        Serial.begin(BAUD);                                     // open the serial port
        Serial.println("Arduino MP3 Shield SD/MMC test");	// say hello  to show that serial is working

        debug_trigger(3);					// trigger the scope with 3 pulses

        // reset the card and echo the result
        r = card.reset();
        Serial.print("Initial card reset returned: ");
        Serial.println (r,DEC); 
        
        // some cards do not respond on the first attempt
        // .. if no card is inserted or detected we stay in this loop forever.
        rst_attempts = 1; // count the number of resets we need before card wakes up
        while(r!=0){
                rst_attempts++;
                r = card.reset();
        }          
        Serial.print("Card responded after reset count of : ");
        Serial.println (rst_attempts,DEC);  
        

        // Run a series of standard commands to test the SD/MMC
        r=MMC_tester();      
        Serial.print("MMC test returned: ");
        Serial.println(r,DEC);

        // set up the FAT variables
        vol.FAT_buffer = card.mmc_sbuf; 	// 512 byte buffer for sector reads/writes
        vol.FAT_scratch = card.mmc_scratch;     // smaller buffer for regster read and writes
        vol.rawDev = &card;                     // pointer to card read block function 
        vol.FAT16_entryMAX =-1;                 // this will be set when FAT is initalised
        vol.FAT16_entryMIN = 0;                 // Set minimum at the first entry
        vol.gFAT_entry = 0;                     // Start at the first entry

        // Red the FAT from disk and setup all associated variables
        r= vol.FAT_initFat16();                 
        Serial.print("FAT Init returned:");
        Serial.println(r,DEC);

        // Stop here if we failed to read the FAT properly.
        if (r!=0) while(1); // HALT         
        
        // Run the File system Tests
        FAT_tester();
        
        Serial.println("DONE.");
}



//////////////////////////////////// LOOP
void loop() 
{
  // do nothing
}



/***************************************************************************
*       Name:	      hexprint
*	Description:  print a byte in HEX 2 digit format to the serial
*	Parameters:   <d> char to print
*	Returns:      none
***************************************************************************/
void hexprint(unsigned char d){
        if (d<16 ) Serial.print('0',BYTE);    // print a leading 0 if we need it
        Serial.print(d,HEX);                  // print the byte
}



/***************************************************************************
*       Name:	      dump_buffer
*	Description:  Dumps the mmc_sbuf[] to the Uart
*	Parameters:   <lines> # of lines (16 Bytes) to send starting from 0x00
*	Returns:      none
***************************************************************************/
void dump_buffer(unsigned char lines, unsigned char buffer[])
{
	unsigned char c,i;
	for (c=0;c<lines;c++){
		Serial.println();
		Serial.print(c,HEX);
		Serial.print(": ");
		for (i=0;i<16;i++){
			hexprint(buffer[i+c*16]);
			Serial.print(' ',BYTE);
		}
		for (i=0;i<16;i++){
			if ((buffer[i+c*16] > 31) && (buffer[i+c*16] <= 'z'))
				Serial.print(buffer[i+c*16],BYTE);	
			else
				Serial.print('.',BYTE);
			
		}
	}
}



/***************************************************************************
*       Name:		MMC_tester
*	Description:	Check out basic MMC functions
*	Parameters:	none
*	Returns:	Status byte, non-zero for failure.
***************************************************************************/
unsigned char MMC_tester(void)
{
	unsigned char c;                // result from each test.
	unsigned short cstatus;         // card status 
	unsigned long cap;              // card capacity
	
	c =  card.reset();                                      // init mmc
	Serial.print("MMC_RESET returned ");
	Serial.print(c,DEC);
	Serial.println();
        
	if (c==0){
		cstatus = card.check();				// check status
		Serial.print("MMC_SEND_STATUS returned ");
		Serial.print(cstatus,DEC);
		c = (unsigned char) cstatus;
		Serial.println();
	}
	if (c==0){
		c = card.identify();
		Serial.print("MMC_SEND_CID returned ");
		Serial.print(c,DEC);
		if (c==0){ // identity OK
			dump_buffer(2,card.mmc_scratch);// dump 2 lines from the buffer
		}
		c=0;
		Serial.println();
	}
	if (c==0){
		c = card.cardType();
		Serial.print("MMC_SEND_CSD returned ");
		Serial.print(c,DEC);
		if (c==0){ // CSD OK
			dump_buffer(2,card.mmc_scratch);// dump 2 lines from the buffer
		}
		c=0;
		Serial.println();
	}
	if (c==0){
		cap = card.capacity()>>10;
		Serial.print("MMC_Capacity returned ");
		Serial.print(cap,DEC);
		Serial.println();
	}
	
	if (c==0){
		c = card.name();
		Serial.print("MMC_Name returned ");
		Serial.print(c,DEC);
		Serial.print(" ");
		if (c==0){
			Serial.print((char*)card.mmc_scratch);
		}	
		Serial.println();
	}

	if (c == 0) {

		c =  card.read(0x00);// read first sector
		Serial.print("MMC_Read returned ");
		Serial.print(c,DEC);
		Serial.println();
		if (c==0){
			Serial.print("MMC First Sector: ");
			dump_buffer(32,card.mmc_sbuf); // dump the sector
		}
	}
			
	Serial.println();
	return c;
}



/***************************************************************************
*       Name:	      debug_trigger
*	Description:  pulse the debug pin low then high again as fast as you can
*	Parameters:   <x> char number of times to pulse the line.
*	Returns:      none
***************************************************************************/
void debug_trigger(char x)
{
        char i;
        for (i=0;i<x;i++){
                digitalWrite(debugpin, LOW);
                digitalWrite(debugpin, HIGH);  
        }
}



/***************************************************************************
*       Name:		FAT_tester
*	Description:	Routines to test filesystem functions to the mmc
*	Parameters:	none
*	Returns:	error code
***************************************************************************/
uint8	FAT_tester(void)
{
	uint8 result,i;
	uint8 record;
	uint8 attrib;
	uint16 sector=0;
	
	// set up the FAT variables
	vol.FAT_buffer = card.mmc_sbuf;                 // 512 byte buffer for sector reads/writes
	vol.rawDev = &card;
	
	result =  card.reset();				// init mmc
	if (result) return result;
	
	result = vol.FAT_initFat16();
	if (result) return result; // abort on non-zero reply 
	
	// print Fat info
	Serial.print("FAT boot Sector info");Serial.println();
	Serial.print("FAT begins at sector ");
	Serial.print(vol.FAT16_fat_begin_lba);Serial.println();
	Serial.print("Clusters begin at sector ");
	Serial.print(vol.FAT16_cluster_begin_lba);Serial.println();
	Serial.print("Sectors per cluster = ");
	Serial.print(vol.FAT16_sectors_per_cluster,DEC);Serial.println();
	Serial.print("Root dir starts at sector ");
	Serial.print(vol.FAT16_root_dir_first_sector);Serial.println();
	
	//show volume label
	result = vol.FAT_get_label(card.mmc_scratch);
	if (!result){
		Serial.print("Volume Name is ");
		Serial.print((char *)card.mmc_scratch);
		Serial.println();
	}
	
	// read the root dir
	sector = vol.FAT16_root_dir_first_sector;
	result = card.read(sector);

	record =0;
	while((result==0) && card.mmc_sbuf[record*32]!=0){
		 // check firstByte
		if (card.mmc_sbuf[record*32] != 0xe5){ // not used (aka deleted)
			
			// get the attrib byte
			attrib = card.mmc_sbuf[(record*32)+11];
			
			if (attrib == FILE_TYPE_FILE || attrib == FILE_TYPE_DIR ){ // entry is normal 8.3 entry
				if (attrib == FILE_TYPE_DIR) Serial.print("[");
				
				// construct short filename string
				for (i=0;i<8;i++){ 
					card.mmc_scratch[i] = card.mmc_sbuf[(record*32)+i];
				}
				card.mmc_scratch[8] = '.';
				card.mmc_scratch[9] = card.mmc_sbuf[(record*32)+8];
				card.mmc_scratch[10] = card.mmc_sbuf[(record*32)+9];
				card.mmc_scratch[11] = card.mmc_sbuf[(record*32)+10];
				card.mmc_scratch[12] = 0x00;
				Serial.print((char *)card.mmc_scratch);		
				if (attrib == FILE_TYPE_DIR) Serial.print("]");
				Serial.print("\t");
				
				// get Cluster 
				Serial.print(card.mmc_scratch[13] = card.mmc_sbuf[(record*32)+0x15],HEX);
				Serial.print(card.mmc_scratch[14] = card.mmc_sbuf[(record*32)+0x14],HEX);
				Serial.print(card.mmc_scratch[15] = card.mmc_sbuf[(record*32)+0x1B],HEX);
				Serial.print(card.mmc_scratch[16] = card.mmc_sbuf[(record*32)+0x1A],HEX);
				Serial.print("\t");
				
				//get fileSize
				Serial.print(card.mmc_scratch[17] = card.mmc_sbuf[(record*32)+0x1f],HEX);
				Serial.print(card.mmc_scratch[18] = card.mmc_sbuf[(record*32)+0x1e],HEX);
				Serial.print(card.mmc_scratch[19] = card.mmc_sbuf[(record*32)+0x1d],HEX);
				Serial.print(card.mmc_scratch[20] = card.mmc_sbuf[(record*32)+0x1c],HEX);
				Serial.print("\t");
				
				//get filenumber
				Serial.print(record+(sector<<4),HEX);
                                Serial.println();	
			}				
		}
		
		// next record or on to next sector
		record++;
		if (record==16){
			record = 0;
			sector++;
			result = card.read(sector);
		}
	}
	
	Serial.println();
	return result;
}


