//
// 
// Arduino MP3 Shield VS1011 and MMC Test file
//
//

/*
Copyright (C) 2011 Nick Lott <brokentoaster@users.sf.net>
http://www.brokentoaster.com/

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

// The integration is a complete mess. 
// This is just a basic cut paste test file to check the hardware is functional.
// If you can tidy this up and make it more Arduino/C++ like then please share your code :) 

// Test the VS1011, MMC/SD Card and then play first song found

#define BAUD 115200
#define FAT_NumberedSong_EN 1
#define ID3_EN

#include <utils.h>
#include <mmc.h>
#include <avrfat16.h>
#include <vs1001.h>
#include <types.h>

MMC     card;                           ///< The physical MMC/SD device
FAT16   vol;                            ///< The Logical file system
vs1001  player;                         ///< The MP3 decoder chip

// These are the pins we'll be using 
char mp3_cs     = 3;
char mp3_bsync  = 2;
char mp3_dreq   = 15;
char mp3_reset  = 14;
char debugpin   = 8;

unsigned char	cluster_pos=0;		///< position in cluster 
unsigned char	buff_pos=16;		///< position in buffer

//////////////////////////////////// SETUP
void setup()
{
        char r;                // byte variable to hold the result from various function calls
        unsigned long lr;      // long variable to hold the result from various function calls
        long rst_attempts;     // number of times rest is attempted with no response from card
        char i;                // loop index

        pinMode(debugpin,OUTPUT);           // setup debug trigger
        digitalWrite(debugpin, HIGH);

        digitalWrite(mp3_dreq, HIGH);       // turn on pullup resistors
        digitalWrite(mp3_bsync, LOW);       // turn off BSYNC
        digitalWrite(mp3_reset, LOW);       // turn off VS1003 /
        digitalWrite(mp3_cs, HIGH);         // turn off VS10xx SPI

        pinMode(mp3_dreq, INPUT);           // set pin to input
        pinMode(mp3_bsync,OUTPUT);          // set pin to output
        pinMode(mp3_reset,OUTPUT);          // set pin to output
        pinMode(mp3_cs,OUTPUT);             // set pin to output
  
        // Init Serial comms and send a test message
        Serial.begin(BAUD);
        Serial.println("Arduino MP3 Shield VS1011 and MMC Test");       
        Serial.println();

        // reset the card and echo the result
        r = card.reset();
        Serial.print("Initial card reset returned: ");
        Serial.println (r,DEC); 
        
        // some cards do not respond on the first attempt
        // .. if no card is inserted or detected we stay in this loop forever.
        rst_attempts = 1; // count the number of resets we need before card wakes up
        while(r != 0){
                rst_attempts++;
                r = card.reset();
        }          
        Serial.print("Card responded after reset count of : ");
        Serial.println (rst_attempts,DEC);  
        Serial.println();
        
        // Run a series of standard commands to test the SD/MMC
        Serial.println("Testing MMC...");
        r = MMC_tester();      
        Serial.print("MMC test returned: ");
        Serial.println(r,DEC);
        Serial.println();

        // set up the FAT variables
        vol.FAT_buffer = card.mmc_sbuf; 	// 512 byte buffer for sector reads/writes
        vol.FAT_scratch = card.mmc_scratch;     // smaller buffer for regster read and writes
        vol.rawDev = &card;                     // pointer to card read block function 
        vol.FAT16_entryMAX =-1;                 // this will be set when FAT is initalised
        vol.FAT16_entryMIN = 0;                 // Set minimum at the first entry
        vol.gFAT_entry = 0;                     // Start at the first entry

        // Red the FAT from disk and setup all associated variables
        Serial.println("Initializing FAT...");
        r= vol.FAT_initFat16();                 
        Serial.print("FAT Init returned:");
        Serial.println(r,DEC);
        Serial.println();
        
        // Stop here if we failed to read the FAT properly.
        if (r != 0) while(1); // HALT   
        
        // Run the File system Tests
        Serial.println("Running FAT tests...");
        FAT_tester();
        lr = open_Dir(vol.FAT16_root_dir_first_sector); // open the directory and find the first file.
        Serial.print("Total directory entries found: ");
        Serial.println(lr);
        
        // get first song in the directory
        vol.gFAT_entry =  vol.FAT_getNextSong(vol.FAT16_entryMIN,vol.FAT16_dir_first_sector);
        Serial.print("Found file # ");
        Serial.println(  vol.gFAT_entry,HEX );
        Serial.print("Filename: ");
        // read file details and print long filename
        vol.FAT_readFile(vol.gFAT_entry,vol.FAT16_dir_first_sector);
        for (i=0; vol.FAT16_longfilename [i] != 0; i++){
                Serial.print( vol.FAT16_longfilename[i] );
        }
        Serial.println();
        Serial.println();
        
        Serial.println("Initializing VS1011.");
        player.init_io();       // set up IO to the chip
        player.init_chip();     // start up the chip 
        
        Serial.println("Starting beep test");
        player.sine_test();     // test vs1011 is all ok
        player.setvolume(0, 0); // max volume

        Serial.println("Attempting to play File");
        cue_file();             // read start of file and fill buffer
        
        Serial.println("DONE.");
        Serial.println();
      
}



//////////////////////////////////// LOOP
void loop() 
{
        // Just keep streaming the MP3 file
        streaming();
}



/***************************************************************************
*       Name:	      open_Dir
*	Description:  open a directory and count no of entries
*	Parameters:   <dir_lba> address of the directory to open
*	Returns:      byte number of files found in directory 
***************************************************************************/
unsigned char open_Dir(unsigned long dir_lba){
        unsigned char files;
        files = vol.FAT_scanDir_lba(dir_lba);   // scan the directory
	vol.gFAT_entry = vol.FAT16_entryMIN;    // set the current file 
	cue_file();                             // load the first file into memory
	return files;
}



/***************************************************************************
*       Name:	      hexprint
*	Description:  print a byte in HEX 2 digit format to the serial
*	Parameters:   <d> char to print
*	Returns:      none
***************************************************************************/
void hexprint(unsigned char d){
        if (d < 16) Serial.print('0',BYTE);    // print a leading 0 if we need it
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
	for (c=0; c<lines; c++){
		Serial.println();
		Serial.print(c,HEX);
		Serial.print(": ");
		for (i=0; i<16; i++){
			hexprint(buffer[i+c*16]);
			Serial.print(' ',BYTE);
		}
		for (i=0; i<16; i++){
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
        
	if (c == 0){
		cstatus = card.check();				// check status
		Serial.print("MMC_SEND_STATUS returned ");
		Serial.print(cstatus,DEC);
		c = (unsigned char) cstatus;
		Serial.println();
	}
	if (c == 0){
		c = card.identify();
		Serial.print("MMC_SEND_CID returned ");
		Serial.print(c,DEC);
		if (c == 0){ // identity OK
			dump_buffer(2,card.mmc_scratch);// dump 2 lines from the buffer
		}
		c = 0;
		Serial.println();
	}
	if (c == 0){
		c = card.cardType();
		Serial.print("MMC_SEND_CSD returned ");
		Serial.print(c,DEC);
		if (c == 0){ // CSD OK
			dump_buffer(2,card.mmc_scratch);// dump 2 lines from the buffer
		}
		c = 0;
		Serial.println();
	}
	if (c == 0){
		cap = card.capacity()>>10;
		Serial.print("MMC_Capacity returned ");
		Serial.print(cap,DEC);
		Serial.println();
	}
	
	if (c == 0){
		c = card.name();
		Serial.print("MMC_Name returned ");
		Serial.print(c,DEC);
		Serial.print(" ");
		if (c == 0){
			Serial.print((char*)card.mmc_scratch);
		}	
		Serial.println();
	}

	if (c == 0) {

		c =  card.read(0x00);// read first sector
		Serial.print("MMC_Read returned ");
		Serial.print(c,DEC);
		Serial.println();
		if (c == 0){
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
        for (i=0; i<x; i++){
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
	uint16 sector = 0;
 	
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

	record = 0;
	while((result == 0) && card.mmc_sbuf[record*32] != 0){
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
		if (record == 16){
			record = 0;
			sector++;
			result = card.read(sector);
		}
	}
	
	Serial.println();
	return result;
}



/***************************************************************************
*       Name:		cue_file
*	Description:	get a file ready to be played
*	Parameters:	none
*	Returns:	none
*
*	Gets file info for file pointed to by gFAT_entry and inits MP3 chip
*       Checks for MMC card as well. Reads ID3v2 tag for title and artist
*	If successful then gFile_good is set
*
***************************************************************************/
void 	cue_file(void)
{
	uint8   check;
	uint32  ID3index;
	uint8   i;
	uint16  j;
	uint32  offset;
	uint16  ID3_Bytes;
	uint32  ID3_clusters;
	uint16  ID3_sectors;
	uint32  nextCluster=0;
	
	
	// read file data and check gFAT_entry is aValid file entry.
	check = vol.FAT_readFile(vol.gFAT_entry,vol.FAT16_dir_first_sector);
	if (vol.FAT16_filetype & FILE_TYPE_DIR) return;

	nextCluster = 0;
	
	// get first Cluster 
	vol.FAT_Scratch2Cluster();
	
	vol.gFileSectorSize += vol.FAT_scratch[17];
	vol.gFileSectorSize <<= 8;
	vol.gFileSectorSize += vol.FAT_scratch[18];
	vol.gFileSectorSize <<= 8;
	vol.gFileSectorSize += vol.FAT_scratch[19];

	cluster_pos = 0;
	buff_pos = 0;
	vol.FAT_readCluster(vol.gCluster,0);
	
	vol.gFile_good = TRUE;
	vol.gFileSectorsPlayed = 0;
	
	// Detect and Skip ID3 Info at start of file
	// (we ignore tags at end of file)
	
	// Version 1.x 
	if (vol.FAT_buffer[0] == 'T' && vol.FAT_buffer[1] == 'A' && vol.FAT_buffer[2]=='G') {
		Serial.println("ID3 v1 TAG");
		// jump to byte 128
		buff_pos = 4; 
	}else
		
	// Version 2.x - now with very basic interpretation!
	if (vol.FAT_buffer[0] == 'I' && vol.FAT_buffer[1] == 'D' 
			  && vol.FAT_buffer[2]=='3'){ 
		Serial.print("ID3 v2.");
		Serial.print(vol.FAT_buffer[5],HEX);
		Serial.print(vol.FAT_buffer[4],HEX);
		Serial.println(" TAG");

		/*
		An ID3v2 tag can be detected with the following pattern:
		$49 44 33 yy yy xx zz zz zz zz
		Where yy is less than $FF, xx is the 'flags' byte and zz is less than $80.
		z = ID3 tag length.
		 */
		
		offset = 0;
		for(i=6;i<10;i++){
			offset <<= 7; // only shift by 7 as MSB is unused.
			offset += vol.FAT_buffer[i];
		}
      
		offset += 10; //include length of header
		Serial.print(offset,HEX);
                Serial.println(" Bytes");

                // offset is now equal to the the length of the TAG
                ID3index = 10; // skip header
#ifdef ID3_EN
                switch (vol.FAT_buffer[3]){
                        case 2:
                                // Search for song / artist frame and replace long filename with them
                                while ((ID3index < offset) && (ID3index < 512)){
                                        if (vol.FAT_buffer[ID3index]   == 'T'  &&
                                            vol.FAT_buffer[ID3index+1] == 'T'  &&
                                            vol.FAT_buffer[ID3index+2] == '2') {
                                                Serial.print("SONG: ");          

                                                // For some reason, all strings have a Null at the front of the 
                                                // name - I think this indicates the encoding type - we'll assume
                                                // its always 00, string! (this is what the -1 is for)
                                                // other -1 is for the null on the end of the string
                                                for (i=0; (i<vol.FAT_buffer[ID3index+5]-1) && (i<LFN_TITLE_LENGTH-1); i++)
                                                {
                                                        vol.FAT16_longfilename[LFN_TITLE_INDEX+i] = vol.FAT_buffer[ID3index+7+i]; 
                                                        Serial.print( vol.FAT16_longfilename[LFN_TITLE_INDEX+i]);
                                                }
                                                vol.FAT16_longfilename[LFN_TITLE_INDEX+i] = '\0';

                                                // Recalculate string length / add a blank
                                                vol.FAT16_LFNTitleLen = strLen(vol.FAT16_longfilename+LFN_TITLE_INDEX);

                                                // Indicate TITLE found
                                                vol.FAT16_longfilename[LFN_TYPE_INDEX] |= LFN_TYPE_ID3_TITLE;
                                        }
                                        else if (vol.FAT_buffer[ID3index]   == 'T'   &&
                                                 vol.FAT_buffer[ID3index+1] == 'P' &&
                                                 vol.FAT_buffer[ID3index+2] == '1') {
                                                Serial.print("ARTIST: ");
                                                for (i=0; (i<vol.FAT_buffer[ID3index+5]-1) && (i<LFN_ARTIST_LENGTH-1); i++){
                                                        vol.FAT16_longfilename[LFN_ARTIST_INDEX+i] = vol.FAT_buffer[ID3index+7+i]; 
                                                        Serial.print( vol.FAT16_longfilename[LFN_ARTIST_INDEX+i] );
                                                }
                                                vol.FAT16_longfilename[LFN_ARTIST_INDEX+i] = '\0';

                                                // Recalculate string length / add a blank
                                                vol.FAT16_LFNArtistLen = strLen(vol.FAT16_longfilename+LFN_ARTIST_INDEX);

                                                // Indicate ARTIST found
                                                vol.FAT16_longfilename[LFN_TYPE_INDEX] |= LFN_TYPE_ID3_ARTIST;
                                                Serial.println();
                                        }
                                        // Skip to next ID3 'frame'
                                        ID3index += vol.FAT_buffer[ID3index+5] + 6;
                                        //Serial.print(" I:");
                                        //UART_Printfu08(ID3index);
                                }
                                break;

                        case 3:
                        case 4:
                                // Search for song / artist frame and replace long filename with them
                                while ((ID3index < offset) && (ID3index < 512)) {
                                        if (vol.FAT_buffer[ID3index]   == 'T' &&
                                            vol.FAT_buffer[ID3index+1] == 'I' &&
                                            vol.FAT_buffer[ID3index+2] == 'T' &&
                                            vol.FAT_buffer[ID3index+3] == '2') {
                                                Serial.print("SONG: ");          

                                                // For some reason, all strings have a Null at the front of the 
                                                // name - I think this indicates the encoding type - we'll assume
                                                // its always 00, string! (this is what the -1 is for)
                                                // other -1 is for the null on the end of the string
                                                for (i=0; (i<vol.FAT_buffer[ID3index+7]-1) && (i<LFN_TITLE_LENGTH-1); i++) {
                                                        vol.FAT16_longfilename[LFN_TITLE_INDEX+i] = vol.FAT_buffer[ID3index+11+i]; 
                                                        Serial.print( vol.FAT16_longfilename[LFN_TITLE_INDEX+i]);
                                                }
                                                vol.FAT16_longfilename[LFN_TITLE_INDEX+i] = '\0';

                                                // Recalculate string length / add a blank
                                                vol.FAT16_LFNTitleLen = strLen(vol.FAT16_longfilename+LFN_TITLE_INDEX);

                                                // Indicate TITLE found
                                                vol.FAT16_longfilename[LFN_TYPE_INDEX] |= LFN_TYPE_ID3_TITLE;
                                                Serial.println();
                                        }
                                        else if (vol.FAT_buffer[ID3index] == 'T' &&
                                                 vol.FAT_buffer[ID3index+1] == 'P' &&
                                                 vol.FAT_buffer[ID3index+2] == 'E' &&
                                                 vol.FAT_buffer[ID3index+3] == '1') {
                                                Serial.print("ARTIST: ");
                                                for (i=0; (i<vol.FAT_buffer[ID3index+7]-1) && (i<LFN_ARTIST_LENGTH-1); i++) {
                                                        vol.FAT16_longfilename[LFN_ARTIST_INDEX+i] = vol.FAT_buffer[ID3index+11+i]; 
                                                        Serial.print( vol.FAT16_longfilename[LFN_ARTIST_INDEX+i] );
                                                }
                                                vol.FAT16_longfilename[LFN_ARTIST_INDEX+i] = '\0';

                                                // Recalculate string length / add a blank
                                                vol.FAT16_LFNArtistLen = strLen(vol.FAT16_longfilename+LFN_ARTIST_INDEX);

                                                // Indicate ARTIST found
                                                vol.FAT16_longfilename[LFN_TYPE_INDEX] |= LFN_TYPE_ID3_ARTIST;
                                                Serial.println( );
                                        }

                                        // Skip to next ID3 'frame'
                                        ID3index += vol.FAT_buffer[ID3index+7] + 10;
                                        //PRINT(" I:");
                                        //UART_Printfu08(ID3index);
                                }
                                break;
                        default:
                                ; // Unknown version, do nothing
                }
#endif
		// skip through to end of ID3 
		ID3_clusters = (offset/512) / vol.FAT16_sectors_per_cluster;
		ID3_sectors = (offset/512) % vol.FAT16_sectors_per_cluster;
		ID3_Bytes = offset % 512;
		
		// find first cluster of actual song
		for (j=0; j<ID3_clusters; j++){
			
			nextCluster = vol.FAT_NextCluster(vol.gCluster);
		//	UART_Printfu16(j);UART_SendByte(0x09);
		//	UART_Printfu32(nextCluster);EOL();
			if (nextCluster == 0xffffffff){
				vol.gFile_good = FALSE;
				PRINT("EOF: ID3 Tag @ Clstr ");
				Serial.print(vol.gCluster,HEX);
				EOL();
				return;
			}
			vol.gCluster = nextCluster;
		}

		
		// Adjust variables for new position.
		vol.gFileSectorsPlayed = (offset / 512);
		buff_pos = (ID3_Bytes/32);
		cluster_pos = ID3_sectors;

		/*
		while(vol.FAT_readCluster(gCluster,cluster_pos)){
			card.MMC_Reset();	
		}
		*/	
		
		/*
                Serial.print("ID3 Tag skipped. First ten bytes of MP3 stream are ... ");
		for (i=0;i<10;i++){
			Serial.print(vol.FAT_buffer[i+ID3_Bytes],HEX);
		}
		Serial.println();
		*/
		j = ID3_Bytes % 32;
		if (j){//we need to play(skip;) some bytes
			buff_pos++; // skips to next 32 byte chunk of buffer
		}
	}else{ //dump  first 10 bytes of file
		/*
		for (i=0;i<10;i++){
			Serial.print(vol.FAT_buffer[i],HEX);
		}
		Serial.println("");
                */
	}
	
	return;
}

/***************************************************************************
*       Name:		streaming
*	Description:    Stream MP3 data from MMC to Decoder
*	Parameters:	none
*	Returns:	none
*	
*	Take care of feeding data to mp3 chip and reading 
*	mmc in free time. AKA playing.
***************************************************************************/  
void 	streaming (void)
{
	uint8	abort=0;
	uint32  nextfile;

	if (!vol.gFile_good) return;                                            // only run if we have a valid file.
	
	if (buff_pos>15) {                                                      // we need to get a new sector
		vol.gFileSectorsPlayed++;
		
		if (cluster_pos>vol.FAT16_sectors_per_cluster-1){               // need new cluster
			cluster_pos = 0;
			vol.gCluster = vol.FAT_NextCluster(vol.gCluster);
		}
		
		if (vol.gCluster == 0xffffffff) {                               // finished song. (EOF)
			// Stop all playing and reset everything
			vol.gFile_good = FALSE;
			card.reset();
			return;
		}
		
		// Attempt to read sector from MMC up to 255 times
		while (vol.FAT_readCluster(vol.gCluster,cluster_pos)&& (--abort)){
			card.reset(); // reset if read failed.
		}
		buff_pos = 0;
		cluster_pos++; // increment for next time
	}
	
	// need to send data to mp3?
	// send at most the rest of sector in memory.
	while (bit_is_set(DREQ_PORT,DREQ_PIN) && (buff_pos<16)){
		player.send_32(vol.FAT_buffer+(32*buff_pos++));
	}
	
	return;
}

