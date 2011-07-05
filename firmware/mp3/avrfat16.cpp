/**
	@file	avrfat16.c
	@brief	Fat16 Functions
	@author	Nick Lott 
	@date	September 2004
	
	$Id: avrfat16.c,v 1.30 2009/01/19 23:36:09 brokentoaster Exp $
 
	Copyright (C) 2004 Nick Lott <brokentoaster@users.sf.net>
 
	This is a simple implementation of the FAT16 file system. It is designed
	to be small for use with MP3 players and MMC cards. Currently it is 
	readonly.NOTE: The code acknowledges only the first partition on the drive.
	
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
	
 
	Target(s)...: ATmega169
 
	Compiler....: AVR-GCC 3.3.1; avr-libc 1.0
 
 **/

#ifndef __AVRFAT16_CPP_
#define __AVRFAT16_CPP_
#include "avrfat16.h"
#include "utils.h"

typedef unsigned char  u08;
typedef          char  s08;
typedef unsigned short u16;
typedef          short s16;
typedef unsigned long  u32;
typedef          long  s32;



void UART_SendByte(u08 Data)
{   
    /* wait for UART to become available */
	while ( (UCSR0A & (1<<UDRE0)) != (1<<UDRE0) ) ;
    
    /* Send character */
    UDR0 = Data;
}




void UART_PrintfU4(u08 Data)
{
    /* Send 4-bit hex value */
    u08 Character = Data&0x0f;
    if (Character>9)
    {
        Character+='A'-10;
    }
    else
    {
        Character+='0';
    }
    UART_SendByte(Character);
}

void UART_Printfu08(u08 Data)
{
    /* Send 8-bit hex value */
    UART_PrintfU4(Data>>4);
    UART_PrintfU4(Data);
}

void UART_Printfu16(u16 Data)
{
    /* Send 16-bit hex value */
    UART_Printfu08(Data>>8);
    UART_Printfu08(Data);
}

void UART_Printfu32(u32 Data)
{
    /* Send 32-bit hex value */
    UART_Printfu16(Data>>16);
    UART_Printfu16(Data);
}

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


//void FAT16::FAT_PrintRecord(uint8 *pRecord);



/**
 *   FAT16::FAT_initFat16
 *	@brief		read MBR and bootsector and set global fat variables
 *	@return 	non zero for error.
 **/
unsigned char FAT16::FAT_initFat16(void) 
{
	unsigned char 	result,type,lba_begin,sectors_per_cluster,number_of_fats;
	unsigned short	bytes_per_sector,number_of_reserved_sectors,BPB_RootEntCnt;
	unsigned short	sectors_per_fat;
	
	// Assume read and write routines have been setup and are not NULL :)
	
	// read the first sector (MBR)
	/*
	 check for 55 AA signature
	 Read Partition info at 
	 446+4 = type check it's a vfat etc
	 446+8..11 = LBA Begin
	 446+12..15 =# of Sectors
	 */
    gFAT_entry 	=0;
    
	result = FAT16::rawDev->read(0);
	
	if (result)
		return result; // abort on read error
	
	if (FAT16::FAT_buffer[510] != 0x55 || FAT16::FAT_buffer[511] != 0xAA)
		return 0xAA; // abort if signature bytes not found in MBR
	
	type = FAT16::FAT_buffer[PARTITION_START+4]; // get type code for partition one
	
	// TODO: check here for valid MMC types ie FAT16 = 0x
	
	// get start of FAT partition.
//	*((uint8 *)&lba_begin+3) = FAT16::FAT_buffer[PARTITION_START+11];
//	*((uint8 *)&lba_begin+2) = FAT16::FAT_buffer[PARTITION_START+10];
//	*((uint8 *)&lba_begin+1) = FAT16::FAT_buffer[PARTITION_START+9];
//	*((uint8 *)&lba_begin+0) = FAT16::FAT_buffer[PARTITION_START+8];
	
	lba_begin = FAT16::FAT_buffer[PARTITION_START+11];
	lba_begin <<=8;	
	lba_begin += FAT16::FAT_buffer[PARTITION_START+10];
	lba_begin <<=8;	
	lba_begin += FAT16::FAT_buffer[PARTITION_START+9];
	lba_begin <<=8;	
	lba_begin += FAT16::FAT_buffer[PARTITION_START+8];
	
	
	/*	Read Boot Sector at lba Begin
		bytes_per_sector at 0x0B
		sectors_per_cluster at 0x0D
		number_of_reserved_sectors at 0x0E
		number_of_fats	at 0x10
		secters_per_fat at 0x24
		root_director_first_sector 0x2c
		*/
	
	// read FAT Volume ID
	result = FAT16::rawDev->read(lba_begin);
	if (result) return result; // abort on read error
	
	//if (FAT16::FAT_buffer[510] != 0x55 || FAT16::FAT_buffer[511] != 0xAA)
	//	return 0xAA; // abort if signature bytes not found in MBR
	
	
	sectors_per_cluster= FAT16::FAT_buffer[0x0D];
	PRINT("SECTORS PER CLUSTOR ");UART_Printfu08(sectors_per_cluster);EOL();
	
	number_of_reserved_sectors = FAT16::FAT_buffer[ 0x0f];
	number_of_reserved_sectors <<=8;
	number_of_reserved_sectors += FAT16::FAT_buffer[ 0x0e];
	
	number_of_fats = FAT16::FAT_buffer[0x10];
	
	bytes_per_sector = FAT16::FAT_buffer[0x0c];
	bytes_per_sector <<=8;
	bytes_per_sector += FAT16::FAT_buffer[0x0b];
	
	PRINT("BYTES PERSECTOR ");UART_Printfu16(bytes_per_sector);EOL();
	
	// BPB_FATSz16
	sectors_per_fat = FAT16::FAT_buffer[0x17];
	sectors_per_fat <<=8;
	sectors_per_fat += FAT16::FAT_buffer[0x16];
	
	//BPB_RootEntCnt
	BPB_RootEntCnt = FAT16::FAT_buffer[0x12];
	BPB_RootEntCnt <<=8;
	BPB_RootEntCnt += FAT16::FAT_buffer[0x11];
	
	
	/*
	 FOR FAT32....
	 sectors_per_fat = FAT16::FAT_buffer[0x27];
	 sectors_per_fat <<=8;
	 sectors_per_fat += FAT16::FAT_buffer[0x26];
	 sectors_per_fat <<=8;
	 sectors_per_fat += FAT16::FAT_buffer[0x25];
	 sectors_per_fat <<=8;
	 sectors_per_fat += FAT16::FAT_buffer[0x24];
	 
	 root_director_first_sector = FAT16::FAT_buffer[0x2f];
	 root_director_first_sector <<=8;
	 root_director_first_sector += FAT16::FAT_buffer[0x2e];
	 root_director_first_sector <<=8;
	 root_director_first_sector += FAT16::FAT_buffer[0x2d];
	 root_director_first_sector <<=8;
	 root_director_first_sector += FAT16::FAT_buffer[0x2c];
	 */
	
	// note: FAT16 clusters start after root directory.
  FAT16::FAT16_fat_begin_lba = lba_begin + number_of_reserved_sectors;
  FAT16::FAT16_root_dir_first_sector = FAT16::FAT16_fat_begin_lba + ( number_of_fats * sectors_per_fat);
  FAT16::FAT16_cluster_begin_lba = FAT16::FAT16_root_dir_first_sector+
		((BPB_RootEntCnt*32) + (bytes_per_sector-1))/bytes_per_sector;
	//	((BPB_RootEntCnt*32) + (bytes_per_sector))/bytes_per_sector;
  FAT16::FAT16_sectors_per_cluster = sectors_per_cluster;
	
  FAT16::FAT16_dir_first_sector=FAT16_root_dir_first_sector;
  FAT16::FAT16_parent_dir_first_sector = FAT16::FAT16_root_dir_first_sector;
	
	return 0;
}

/**
*   FAT16::FAT_get_label
 *	@brief		copys the volume label
 *	@param		destentaion for label
 *	@return 	non zero for error.
 **/

uint8 FAT16::FAT_get_label(uint8 label[])
{
	uint8 result,i,j;
	
	// read root dir
	result = FAT16::rawDev->read(FAT16::FAT16_root_dir_first_sector);
	if (result) return result; // abort on fail
	
	// find entry with ATTR_VOLUME_ID
	for (i=0;i<512/32;i++){
		if (FAT16::FAT_buffer[(i*32)+11] == FILE_ATTR_VOLUME_ID){
			for (j=0;j<11;j++)
				label[j]=FAT_buffer[(i*32)+j];
			label[11]=0;
			return 0;// we have a volume
		}
	}
	
	return 1;
	
	
}


/**
*   FAT16::FAT_readCluster
 *	@brief		Reads a sector from a cluster into the buffer
 *	@param		cluster		Clusterto read from
 *	@param		sector_offset	Sector to read from within cluster
 *	@return		error code from FAT16::rawDev->read()
 **/
unsigned char FAT16::FAT_readCluster(unsigned long cluster, unsigned char sector_offset) 
{
	unsigned char	result=0;
	unsigned long lba_addr;
	
	// calculate sector to read
	lba_addr = FAT16::FAT16_cluster_begin_lba + ((cluster-2) * FAT16::FAT16_sectors_per_cluster);
	lba_addr += sector_offset;
	
	result = FAT16::rawDev->read(lba_addr);
	
	return result;
}


/**
*   FAT16::FAT_NextCluster
 *	@brief		looks up a cluster in fat
 *	@param		cluster 	current cluster
 *	@return		next cluster
 *	@return 	0 for error 
 *	@return  	0xffffff for end of file.
 **/
unsigned long FAT16::FAT_NextCluster(unsigned long cluster)
{
	unsigned char result=0;
	unsigned long sector,position,next;
	
	if (cluster ==0 ) cluster = 2; // check for reserved cluster mishap ;)
	
	//sector = cluster / 128; for FAT32
	sector = cluster / 256; // for FAT16
	
	result = FAT16::rawDev->read(FAT16::FAT16_fat_begin_lba+sector);
	if (result) return 0;
	
	//position = (cluster - (sector * 128)) * 4; for fat 32
	position = (cluster - (sector * 256)) * 2; // for fat 16
	
	/*
	 // read 4 bytes (FAT32)
	 next = FAT16::FAT_buffer[position+3];
	 next <<= 8;
	 next += FAT16::FAT_buffer[position+2];
	 next <<= 8;
	 next = FAT16::FAT_buffer[position+1];
	 next <<= 8;
	 next += FAT16::FAT_buffer[position];
	 */
	
	// read 2 Bytes (FAT16)
	next = FAT16::FAT_buffer[position+1];
	next <<= 8;
	next += FAT16::FAT_buffer[position];
//	*((char *)&next +1) = FAT16::FAT_buffer[position+1];
//	*((char *)&next +0) = FAT16::FAT_buffer[position];

	// mask off last 4 bits (FAT32)
	// next &= 0x0ffffff;
	
	if (next == 0xffff) next = 0xffffffff;
	
	return next;
}


/**
 *  FAT16::FAT_read
 *
 *	Read any directory of the disk and obtain details 
 *	about FAT entry number filenumber
 *
 *	error codes:			<br>
 *	0x00	mp3file found		<br>
 *	0x02	end of directory	<br>
 *	0x59	deleted file		<br>
 *
 *	@param		filenumber 32bit uInt pointing to file of interst.
 *	@param		dir_first_sector 32bit uInt pointing to first cluster of directory
 *	@return		error code
 **/
unsigned char	FAT16::FAT_readFile(unsigned long filenumber, unsigned long dir_first_sector)
{
	unsigned char flags;
#define FLG_ROOTDIR 0
	unsigned char result,i,lfn;
	unsigned char record,lfn_record,nmeChkSm,lfnchksm;
	unsigned char attrib,saveattrib;
	unsigned short lfn_entry,fat_entry; // position in buffer of entry 0-512
	unsigned short clustersadvance,c;
	unsigned long cluster,lfn_cluster;
	unsigned long sector,lfn_sector;
	//	unsigned short cluster;
	//	uint32 size;
	flags = 0x00;
	cluster =0;
	clustersadvance=0;
	record = filenumber & 0x0F;	// entry number in sector
	sector = filenumber >> 4;	// sector offset to entry
	fat_entry =record << 5;		// byte offset of entry
	
	// check if root directory
	if (dir_first_sector == FAT16::FAT16_root_dir_first_sector){ 	
		// Root dir is all contigous sectors
		sector += dir_first_sector;
		// read the dir
		result = FAT16::rawDev->read(sector);
		flags |= (1<<FLG_ROOTDIR); 
	}
	else{  // else need to use FAT to find correct cluster 
		clustersadvance = (sector / FAT16::FAT16_sectors_per_cluster);		
		sector %= FAT16::FAT16_sectors_per_cluster;
		cluster = FAT16::FAT_lba2cluster(dir_first_sector);

		// follow the cluster chain
		for (c = 0; c < clustersadvance; c++){
			cluster = FAT16::FAT_NextCluster(cluster);
			// UART_Printfu32(cluster);EOL();
		}

		// read the dir
		result = FAT16::rawDev->read(sector + FAT16::FAT_cluster2lba(cluster));
	}

	if (result) return result; // abort on non-zero reply 
	
	// check firstByte
	if (FAT16::FAT_buffer[fat_entry] == 0x00){ // end of directory
	  FAT16::FAT16_entryMAX = filenumber;
		return 2;
	}
	else if (FAT16::FAT_buffer[fat_entry] != 0xe5){ // not used (aka deleted)
		
		// exit on . and ..
		if (FAT16::FAT_buffer[fat_entry]=='.')
			return 0x59;
		
		// get the attrib byte
		attrib = FAT16::FAT_buffer[fat_entry+FAT16_LDIR_Attr];
		// PRINT("attr ");	UART_Printfu08(attrib);	EOL();
		
		if (attrib & FILE_ATTR_HIDDEN) //keep hidden files hidden
			return 0x59;
		
		saveattrib = attrib; // save this for later
			
		if (attrib & (FILE_ATTR_DIRECTORY + FILE_ATTR_ARCHIVE) ){ // entry is normal 8.3 entry
			
			// construct short filename string
			for (i=0;i<12;i++){ 
			  FAT16::FAT_scratch[i] = FAT16::FAT_buffer[fat_entry+i];
			}
		  FAT16::FAT_scratch[11] = '/';
			if (attrib == FILE_TYPE_FILE ){
			  FAT16::FAT_scratch[8] = '.';
			  FAT16::FAT_scratch[9] = FAT16::FAT_buffer[fat_entry+8];
			  FAT16::FAT_scratch[10] = FAT16::FAT_buffer[fat_entry+9];
			  FAT16::FAT_scratch[11] = FAT16::FAT_buffer[fat_entry+10];
			}
		  FAT16::FAT_scratch[12] = 0x00;
			
			
			
			// accept only *.mp3 files
			if((attrib & FILE_ATTR_DIRECTORY )||
			   ( FAT16::FAT_scratch[9] =='M' && 
			     FAT16::FAT_scratch[10] =='P' && 
			     FAT16::FAT_scratch[11] =='3')){
				
				
				// get Cluster 
			  FAT16::FAT_scratch[13] = FAT16::FAT_buffer[(fat_entry)+0x15];
			  FAT16::FAT_scratch[14] = FAT16::FAT_buffer[(fat_entry)+0x14];
			  FAT16::FAT_scratch[15] = FAT16::FAT_buffer[(fat_entry)+0x1B];
			  FAT16::FAT_scratch[16] = FAT16::FAT_buffer[(fat_entry)+0x1A];
				//UART_Printfu08(FAT16::FAT_scratch[13] = FAT16::FAT_buffer[(fat_entry)+0x15]);
				//UART_Printfu08(FAT16::FAT_scratch[14] = FAT16::FAT_buffer[(fat_entry)+0x14]);
				//UART_Printfu08(FAT16::FAT_scratch[15] = FAT16::FAT_buffer[(fat_entry)+0x1B]);
				//UART_Printfu08(FAT16::FAT_scratch[16] = FAT16::FAT_buffer[(fat_entry)+0x1A]);
				//PRINT("\t");
				
				//get fileSize
			  FAT16::FAT_scratch[17] = FAT16::FAT_buffer[(fat_entry)+0x1f];
			  FAT16::FAT_scratch[18] = FAT16::FAT_buffer[(fat_entry)+0x1e];
			  FAT16::FAT_scratch[19] = FAT16::FAT_buffer[(fat_entry)+0x1d];
			  FAT16::FAT_scratch[20] = FAT16::FAT_buffer[(fat_entry)+0x1c];
//				UART_Printfu08(FAT16::FAT_scratch[17] = FAT16::FAT_buffer[(fat_entry)+0x1f]);
//				UART_Printfu08(FAT16::FAT_scratch[18] = FAT16::FAT_buffer[(fat_entry)+0x1e]);
//				UART_Printfu08(FAT16::FAT_scratch[19] = FAT16::FAT_buffer[(fat_entry)+0x1d]);
//				UART_Printfu08(FAT16::FAT_scratch[20] = FAT16::FAT_buffer[(fat_entry)+0x1c]);
				
				//
				// Do Long Filename entries....
				
				//checksum of name for long entries..
				nmeChkSm=FAT_ChkSum(FAT16::FAT_buffer+fat_entry);
				
				lfn_record = record;
				lfn_sector = sector;
				lfn_entry = lfn_record << 5;
				lfn_cluster = cluster;
				
				// if we are in root sector is lba
				// so we can go back as many sectors as we need
				
				do{
					// look at previous FAT entry
					if (lfn_record > 0){
						lfn_record--;
					}else{
						if (lfn_sector > 0) {
							lfn_sector--;
							
							if (flags & (1<<FLG_ROOTDIR)){
								result = FAT16::rawDev->read(lfn_sector);
							}else{
								result = FAT16::rawDev->read(lfn_sector+FAT_cluster2lba(lfn_cluster));
							}

							if (result){ // abort on non-zero reply 
								PRINT("ERR RD SCTR");
								break;
							}
							
							lfn_record = 0x0F;// 15 records per sector FAT16
								
						}else{ 	// run out of sectors get prev cluster
							// should only happen on non root dir.
							
							lfn_cluster = FAT16::FAT_lba2cluster(dir_first_sector);
							
							// follow the cluster chain to 1 less cluster
							// than berfore
							for (c = 0; c < --clustersadvance; c++){
								lfn_cluster = FAT16::FAT_NextCluster(lfn_cluster);
							}
							
							// set to last sector in cluster
							lfn_sector = FAT16::FAT16_sectors_per_cluster-1;
							lfn_record = 0x0F;// 15 records per sector FAT16

							result=FAT16::rawDev->read(lfn_sector+  FAT16::FAT_cluster2lba(lfn_cluster));
							if (result){ // abort on non-zero reply 
								PRINT("ERR RD SCTR");
								break;
							}						 

		
						}
					}
					lfn_entry =	lfn_record << 5;
					
					// dump record for debugging
					//FAT_PrintRecord(FAT16::FAT_buffer+lfn_entry);EOL();
					
					// get the attrib byte
					attrib = FAT16::FAT_buffer[lfn_entry+FAT16_LDIR_Attr];
					
					//exit if no LFN entry found
					if (!(attrib & FILE_ATTR_LFN_TEXT)){
						PRINT("no LFN ");
						UART_Printfu08(attrib);
						EOL();
						break;
					}
					
					//get the chksm
					lfnchksm = FAT16::FAT_buffer[lfn_entry+FAT16_LDIR_Chksum];
					
					// exit if ChkSum doesn't Match
					if (lfnchksm != nmeChkSm){
						PRINT("CkSm err");
						UART_Printfu08(nmeChkSm);
						UART_Printfu08(lfnchksm);
						EOL();
						return 59;
						break;
					}
					
					//look at order, select lfn section
					lfn = ((FAT16::FAT_buffer[lfn_entry+FAT16_LDIR_Ord] & FILE_ATTR_LFN_MASK)-1) *13;
					// UART_Printfu08(FAT16::FAT_buffer[lfn_entry]);
					
					if (lfn < LFN_BUFFER_LENGTH-13)  // Only allow 208 byte max LFNs
					{
						// note: only look at lower byte of unicode text.
					  FAT16::FAT16_longfilename[13+lfn] = 0x00;
					  FAT16::FAT16_longfilename[12+lfn] = FAT16::FAT_buffer[lfn_entry+0x1e];
					  FAT16::FAT16_longfilename[11+lfn] = FAT16::FAT_buffer[lfn_entry+0x1c];
					  FAT16::FAT16_longfilename[10+lfn] = FAT16::FAT_buffer[lfn_entry+0x18];
					  FAT16::FAT16_longfilename[9+lfn] = FAT16::FAT_buffer[lfn_entry+0x16];
					  FAT16::FAT16_longfilename[8+lfn] = FAT16::FAT_buffer[lfn_entry+0x14];
					  FAT16::FAT16_longfilename[7+lfn] = FAT16::FAT_buffer[lfn_entry+0x12];
					  FAT16::FAT16_longfilename[6+lfn] = FAT16::FAT_buffer[lfn_entry+0x10];
					  FAT16::FAT16_longfilename[5+lfn] = FAT16::FAT_buffer[lfn_entry+0x0e];
					  FAT16::FAT16_longfilename[4+lfn] = FAT16::FAT_buffer[lfn_entry+0x09];
					  FAT16::FAT16_longfilename[3+lfn] = FAT16::FAT_buffer[lfn_entry+0x07];
					  FAT16::FAT16_longfilename[2+lfn] = FAT16::FAT_buffer[lfn_entry+0x05];
					  FAT16::FAT16_longfilename[1+lfn] = FAT16::FAT_buffer[lfn_entry+0x03];
					  FAT16::FAT16_longfilename[lfn] = FAT16::FAT_buffer[lfn_entry+0x01];
					}
					
					// test to see if we need to keep looking
				}while(FAT16::FAT_buffer[lfn_entry] < FILE_ATTR_LFN_MASK);	// not last entry
				
				
				// check if we made it through all entries
				if (FAT16::FAT_buffer[lfn_entry] < FILE_ATTR_LFN_MASK){ 
					// use short name
					for (i=0;i<13;i++){
					  FAT16::FAT16_longfilename[i]=FAT_scratch[i];
					}
				}
				/*		
					if (lfn_sector != sector) {// re read original FAT sector if changed
						result = FAT16::rawDev->read(FAT16::FAT16_root_dir_first_sector+sector);
						if (result) return result; // abort on non-zero reply 
					}
				*/
				
				//PRINT("\t");
				//UART_Printfu32(record+(sector<<4));EOL();
				//UART_Puts(FAT16::FAT16_longfilename);EOL();
				
				
				// RA Sewell - for ID3 tags and scrolling name on NOKIA
			  FAT16::FAT16_LFNTitleLen = strLen(FAT16::FAT16_longfilename);
			  FAT16::FAT16_longfilename[LFN_TYPE_INDEX] = LFN_TYPE_FILENAME;
			
				// prefix directories with a slash
				if (saveattrib & FILE_TYPE_DIR){
					if (FAT16::FAT16_LFNTitleLen<LFN_BUFFER_LENGTH){
					  FAT16::FAT16_LFNTitleLen++;
					}
					for (i=LFN_BUFFER_LENGTH;i>0;i--){
					  FAT16::FAT16_longfilename[i] = FAT16::FAT16_longfilename[i-1] ;
					}
				  FAT16::FAT16_longfilename[0] = '/';
					if (FAT16::FAT16_LFNTitleLen<LFN_BUFFER_LENGTH){
					  FAT16::FAT16_LFNTitleLen++;
					}
				  FAT16::FAT16_longfilename[FAT16_LFNTitleLen-1] = '/';
					
				}
				
				// don't show dotfiles like .temp and .Trash, etc.
				if (FAT16::FAT16_longfilename[0]=='.')
					return 0x59;
				
			  FAT16::FAT16_filetype = saveattrib;
				
				//dump_buffer(32,FAT_buffer);
				return 0x00;
			}
		}
		
		
	}
	
	return 0x59;
}

/**
 *  FAT16::FAT_readRoot
 *
 *	Read the root directory of the disk and obtain details 
 *	about FAT entry number <filenumber>
 *
 *	@param		filenumber 32bit uInt pointing to file of interst.
 *	@return		error code
 **/
//uint8  FAT16::FAT_readRoot(uint32 filenumber)
//{
//	return FAT16::FAT_readFile(filenumber,FAT16_root_dir_first_sector);
//}


/**
 *	GetNextSong
 *	
 *	@param filenumber uint32 index of current  file in directory 
 *	@param dir_lba uint32 lba of directory to search
 *
 *	@return new filenumber or 0=err
 *
 **/
unsigned long FAT16::FAT_getNextSong(unsigned long filenumber,unsigned long dir_lba){
	unsigned short i;
	unsigned char result=0;
	//uint32 nextfile;
	
	if (filenumber < FAT16::FAT16_entryMAX){
		filenumber++;
		
		// find next valid FAT entry
		i=1023;// time out after 1023 entries
			while ( (i--) && ((result = FAT16::FAT_readFile(filenumber++,dir_lba))== 0x59) );

			//dump_buffer(32,FAT_buffer);
			// read last FAT entry if valid
			if (i && (result==0) ){
				filenumber--;
				// PRINT("File #");UART_Printfu16(filenumber); EOL();
				return filenumber;
			}
	}
	return 0;
	
}


/**
 *	GetPrevSong
 *
 *	@param filenumber uint32 index of current  file in directory 
 *	@param dir_lba uint32 lba of directory to search
 *
 *	@return file number of next file or zero if none found.
 **/
unsigned long FAT16::FAT_getPrevSong(unsigned long filenumber,unsigned long dir_lba){
	unsigned short	i;
	unsigned char	result=0;
	
	if (filenumber>FAT16_entryMIN){
		filenumber--;
		
		// find previous valid FAT entry
		i=1023;
		while ( (filenumber>0) && (i--) && ((result = FAT16::FAT_readFile(filenumber--,dir_lba))== 0x59)   );
		
		//set if valid
		if ((result==0) && i && filenumber){
			filenumber++;
			// PRINT("File #");UART_Printfu16(filenumber); EOL();
			return filenumber;
		}
	}
	
	// return 0 if no new file found.
	return 0;
}

/**
 *	GetNumberedSong
 *
 *  Get the file number of a song given a 2 digit BCD of the first two digits 
 *  of the filename. eg given CHAR2BCD(8) as an argument the function will find 
 *  the first file in the current directory whos name begins with "08" and ends 
 *  with ".mp3". Can be removed from code by setting FAT_NumberedSong_EN to 0.
 *	
 *  @param songNumber   char    a BCD number 00 - 99 of song/sample
 *	@param dir_lba      uint32  lba of directory to search
 *
 *	@return new filenumber or 0=err
 *
 **/
unsigned long FAT16::FAT_getNumberedSong(char songNumber,unsigned long dir_lba)
{
    #if FAT_NumberedSong_EN 
	unsigned short i=1;
	unsigned char result=0;
    unsigned char found=0;
    unsigned long filenumber;
    
	
    filenumber = FAT16::FAT16_entryMIN;
    
    while (i && found==0 && (filenumber <= FAT16::FAT16_entryMAX))
    {
		// find next valid FAT entry
		i=1023;// time out after 1023 entries
        while ( (i--) && ((result = FAT16::FAT_readFile(filenumber++,dir_lba))== 0x59) );
        
        //dump_buffer(32,FAT_buffer);
        // read last FAT entry if valid
        if (i && (result==0) ) // we have a possible file
        {
            
            //check the name (first two chars only)
            if ( (((songNumber & 0x0F) +'0') == FAT16::FAT16_longfilename[1]) && 
                 (((songNumber >> 04) + '0') == FAT16::FAT16_longfilename[0] ))
            {   
                PRINT("FOUND :");UART_Puts(FAT16::FAT16_longfilename);EOL();
                filenumber--;
                return filenumber;
            }
        }
        
	}
    PRINT("NOT FOUND :");UART_Printfu08(songNumber);EOL();
    #endif
    return 0;
}


/** 
 *  FAT16::FAT_ChkSum() 
 * 
 * Returns an unsigned byte check sum computed on an unsignedbyte 
 * array. The array must be 11 bytes long and is assumed to contain 
 * a name stored in the format of a MS-DOS directory entry. 
 * 
 * @param	pFcbName Pointer to an unsigned byte array assumed to be 
 *		11 bytes long. 
 * @Return	An 8-bit unsigned checksum of the array pointed 
 *		to by pFcbName. 
 **/
unsigned char FAT16::FAT_ChkSum(unsigned char *pFcbName) 
{ 
	unsigned char FcbNameLen; 
	unsigned char Sum;

	Sum=0; 
	for(FcbNameLen=11;FcbNameLen!=0;FcbNameLen--){ 
		//NOTE:The operation is an unsignedchar rotate right 
		Sum=((Sum&1)?0x80:0)+(Sum>>1)+*pFcbName++; 
		
	} 

	return(Sum); 
} 


/*
 void FAT16::FAT_PrintRecord(uint8 *pRecord)
 {
	 uint8 i;
	 for (i=0;i<32;i++){
		 UART_Printfu08(*pRecord++);
	 }
	 EOL();
	 for (i=0;i<32;i++){
		 UART_SendByte(*pRecord++);
	 }
	 EOL();
 }*/

/**
 *  FAT16::FAT_cluster2lba
 *
 *	Calculate the sector address of a cluster.
 *
 *	@param	uint32 pointing to cluster
 *	@return	lba of first sector in cluster
 **/
unsigned long FAT16::FAT_cluster2lba(unsigned long cluster)
{
	unsigned long lba_addr;
	
	// calculate sector to read
	lba_addr = FAT16::FAT16_cluster_begin_lba + ((cluster-2) * FAT16::FAT16_sectors_per_cluster);
	//UART_Printfu32(lba_addr);EOL();
	
	return lba_addr;
}

/**
 *  FAT16::FAT_lba2cluster
 *
 *	Calculate the cluster address of a sector.
 *
 *	@param	lba_addr uint32 pointing to lba_addr in cluster
 *	@return	cluster containing the lba_addr
 **/
unsigned long FAT16::FAT_lba2cluster(unsigned long lba_addr)
{
	unsigned long cluster;
	
	cluster = 2+((lba_addr-FAT16_cluster_begin_lba)/FAT16_sectors_per_cluster);

	return cluster;
}


/**
 *  FAT16::FAT_scanDir_lba
 *
 *	scan a directory for songs, set max, min and currentdir variables
 *
 *	@param	lba_addr uint32 pointing to directory starting sector
 *	@return	number of files found (up to 255)
 **/
unsigned char FAT16::FAT_scanDir_lba(unsigned long lba_addr)
{	
	unsigned char files=0;
	unsigned long file=-1,nextfile;
	unsigned long saveMin,saveMax;
	
	// ignore last dirs file max
	saveMax = FAT16::FAT16_entryMAX;
  FAT16::FAT16_entryMAX = 0xffff; 
	
	// get first good file
	saveMin = FAT16::FAT16_entryMIN;
  FAT16::FAT16_entryMIN = FAT16::FAT_getNextSong(0,lba_addr);
	
	//scan directory and get max File Entry.
	nextfile = FAT16::FAT16_entryMIN;
	while ( nextfile ){
		file = nextfile;
		files++;
		nextfile = FAT16::FAT_getNextSong(file,lba_addr);
	}
	
	if (files){ // then set working directory
	  FAT16::FAT16_entryMAX =file; // set to last valid MP3
	  FAT16::FAT16_parent_dir_first_sector = FAT16::FAT16_dir_first_sector;
	  FAT16::FAT16_dir_first_sector = lba_addr;
	}else{
	  FAT16::FAT16_entryMIN = saveMin;
	  FAT16::FAT16_entryMAX = saveMax;
	}
	
	return files;
}


/**
 *  FAT16::FAT_getParentDir.
 *		
 *	@param	lba_addr uint32 pointing to current directory starting sector
 *	@return	first sector of the parent directory or 0 for error
 *
 *	return the first sector of the .. entry. assuming it is the 
 *	second entry in the directory file. otherwise return 0 to do nothing.
 **/
unsigned long FAT16::FAT_getParentDir(unsigned long lba_addr)
{
	unsigned char result,record,fat_entry;
	//uint8 *p;
	
	unsigned long lba;
	
	// no parent from the root directory
	if (lba_addr == FAT16::FAT16_root_dir_first_sector) return 0;
	

	// read the dir
	result = FAT16::rawDev->read(lba_addr);
	if (result) return result; // abort on non-zero reply 
	
	//dump_buffer(6,FAT_buffer);
	//UART_Printfu08(FAT16::FAT_buffer[fat_entry]);
	//UART_Printfu08(FAT16::FAT_buffer[fat_entry+1]);
	
	// get second entry in directory
	record = 0x01;
	fat_entry = 0x20;
	
	if (FAT16::FAT_buffer[fat_entry]!='.' &&FAT_buffer[fat_entry+1]!='.') return 0;
	
	// get Cluster 
	/*
	UART_Printfu08(FAT16::FAT_scratch[13] = FAT16::FAT_buffer[(fat_entry)+0x15]);
	UART_Printfu08(FAT16::FAT_scratch[14] = FAT16::FAT_buffer[(fat_entry)+0x14]);
	UART_Printfu08(FAT16::FAT_scratch[15] = FAT16::FAT_buffer[(fat_entry)+0x1B]);
	UART_Printfu08(FAT16::FAT_scratch[16] = FAT16::FAT_buffer[(fat_entry)+0x1A]);
	EOL();
	*/
	
	// this should be quicker but...
//	lba = 0;
//	*((char *)&lba + 3) = FAT16::FAT_buffer[(fat_entry)+0x15];
//	*((char *)&lba + 2) = FAT16::FAT_buffer[(fat_entry)+0x14];
//	*((char *)&lba + 1) = FAT16::FAT_buffer[(fat_entry)+0x1B];
//	*((char *)&lba + 0) = FAT16::FAT_buffer[(fat_entry)+0x1A];
	
	// this method comnpiles smaller ?!??
	lba = FAT16::FAT_buffer[(fat_entry)+0x15];
	lba <<= 8;
	lba += FAT16::FAT_buffer[(fat_entry)+0x14];
	lba <<= 8;
	lba += FAT16::FAT_buffer[(fat_entry)+0x1B];
	lba <<= 8;
	lba += FAT16::FAT_buffer[(fat_entry)+0x1A];
	
	
	// if its zero then set to root directory
	if (lba == 0) {
		lba = FAT16::FAT16_root_dir_first_sector;
	}else{
		lba = FAT16::FAT_cluster2lba(lba);
	}
	
	return lba;
}


/**
 *  FAT16::FAT_Scratch2Cluster
 *
 *	Extracts the first cluster from the scratchpad buffer
 *	left after the most recent call of FAT16::FAT_readFile() 
 **/
void FAT16::FAT_Scratch2Cluster(){
	
	// copy the bytes from the scratch pad into the varible
	*((char *)&gCluster + 0) = FAT16::FAT_scratch[16];
	*((char *)&gCluster + 1) = FAT16::FAT_scratch[15];
	*((char *)&gCluster + 2) = FAT16::FAT_scratch[14];
	*((char *)&gCluster + 3) = FAT16::FAT_scratch[13];

	
}
#endif