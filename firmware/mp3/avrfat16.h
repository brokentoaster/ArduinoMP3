/**
	@file	avrfat16.h
	@brief	Fat16 Functions
	@author	Nick Lott 
	@date	September 2004
 
	$Id: avrfat16.h,v 1.20 2009/01/10 23:16:08 brokentoaster Exp $

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

#ifndef __AVRFAT16_H_
#define __AVRFAT16_H_
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#include "WProgram.h"
#include "types.h"
#include "utils.h"
#include "mmc.h"


#define PARTITION_START		446	///< Start address of partition 1 in MBR

//-----------------------------------------------------------------------------
// FAT32 File Attributes and Types
//-----------------------------------------------------------------------------
#define FILE_ATTR_READ_ONLY   	0x01
#define FILE_ATTR_HIDDEN 		0x02
#define FILE_ATTR_SYSTEM 		0x04
#define FILE_ATTR_SYSHID		0x06
#define FILE_ATTR_VOLUME_ID 	0x08
#define FILE_ATTR_DIRECTORY		0x10
#define FILE_ATTR_ARCHIVE  		0x20
#define FILE_ATTR_LFN_TEXT		0x0F
#define FILE_ATTR_LFN_MASK		0x3F
#define FILE_HEADER_BLANK		0x00
#define FILE_HEADER_DELETED		0xE5
#define FILE_TYPE_DIR			0x10
#define FILE_TYPE_FILE			0x20
#define FILE_TYPE_MP3			0x55
#define FAT16_LDIR_Ord			0
#define FAT16_LDIR_Name1		1
#define FAT16_LDIR_Attr			11
#define FAT16_LDIR_Type			12
#define FAT16_LDIR_Chksum		13
#define FAT16_LDIR_Name2		14
#define FAT16_LDIR_FstClusLO	26
#define FAT16_LDIR_Name3		28

// For ID3 tags
#define LFN_TYPE_INDEX        208
#define LFN_TYPE_FILENAME     0x00        ///< No bits set
#define LFN_TYPE_ID3_TITLE    0x01        ///< Bit 0
#define LFN_TYPE_ID3_ARTIST   0x02        ///< Bit 1
#define LFN_BUFFER_LENGTH     208
#define LFN_TITLE_INDEX       0
#define LFN_TITLE_LENGTH      100      ///< Title length including '\0'
#define LFN_ARTIST_INDEX      100
#define LFN_ARTIST_LENGTH     100      ///< Artist length including '\0'
#define LFN_FREE_INDEX        200
#define LFN_FREE_LEN          8        
///< Currently 8 free bytes in buffer if using ID3 tags not filenames
class FAT16{
public:

    unsigned long 	gFAT_entry ; 	///< global current entry number in FAT table. (aka file number)
    unsigned long 	gCluster; 		///< current  cluster in song
    unsigned long 	gFileSectorSize; 	///< size of current file in complete sectors.
    unsigned long	gFileSectorsPlayed;	///< number of full sectors played. (aka position)
    unsigned char   	gFile_good; 		///< File has been read from Fat and ready to go.
    
unsigned long 	FAT16_fat_begin_lba;
unsigned long 	FAT16_cluster_begin_lba;
 unsigned char	FAT16_sectors_per_cluster;
unsigned long	FAT16_dir_first_sector;
unsigned long	FAT16_root_dir_first_sector;
unsigned long	FAT16_parent_dir_first_sector;

unsigned char	FAT16_filetype;
unsigned char 	FAT16_longfilename[LFN_BUFFER_LENGTH+1]; ///< buffer for long filenames / ID3 tags
unsigned char	FAT16_LFNTitleLen;
unsigned char	FAT16_LFNArtistLen;
unsigned short 	FAT16_entryMAX; 	///< Maximum entry number in FAT table. (aka file number)
unsigned short 	FAT16_entryMIN; 	///< Minimum entry number in FAT table. (aka file number)

unsigned char *FAT_buffer; ///< 512 byte buffer for sector reads/writes
unsigned char *FAT_scratch; ///< 32 byte buffer for filenames and data
    MMC *rawDev; ///< Actual hw to do reading and writing
 //   unsigned char  MMC::(*FAT_read)(unsigned long lba); ///< pointer to read block function 
 //   unsigned char  MMC::(*FAT_write)(unsigned long lba);///< pointer to write block function 

unsigned char FAT_initFat16(void); ///< read MBR and bootsector and set fat variables
unsigned char FAT_readCluster(unsigned long cluster, unsigned char sector_offset); ///< read a sector from a cluster
unsigned char FAT_get_label(unsigned char label[]); 
unsigned long FAT_NextCluster(unsigned long cluster); //
//unsigned char FAT_NextnCluster(unsigned short cluster, unsigned short fatBuf[], unsigned char buffer_size);
//unsigned char FAT_getLongFilename(unsigned long filenumber);
unsigned long FAT_getNextSong(unsigned long filenumber,unsigned long dir_lba);
unsigned long FAT_getPrevSong(unsigned long filenumber,unsigned long dir_lba);

/**
 *	GetNumberedSong
 *
 *  Get the file number of a song given a 2 digit BCD of the first two digits 
 *  of the filename. eg given CHAR2BCD(8) as an argument the function will find 
 *  the first file in the current directory whos name begins with "08" and ends 
 *  with ".mp3"
 *	
 *  @param songNumber   char    a BCD number 00 - 99 of song/sample
 *	@param dir_lba      unsigned long  lba of directory to search
 *
 *	@return new filenumber or 0=err
 *
 **/
unsigned long FAT_getNumberedSong(char songNumber,unsigned long dir_lba);


/** 
*	FAT_ChkSum() 
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
unsigned char FAT_ChkSum(unsigned char *pFcbName); 

/**
 *   FAT_readRoot
 *
 *	Read the root directory of the disk and obtain details 
 *	about FAT entry number filenumber
 *
 *	@param		filenumber 32bit uInt pointing to file of interst.
 *	@return		error code
 **/
//unsigned char	FAT_readRoot(unsigned long filenumber);

/**
 *   FAT_read
 *
 *	Read any directory of the disk and obtain details 
 *	about FAT entry number filenumber
 *
 *	@param		filenumber 32bit uInt pointing to file of interst.
 *	@param		dir_first_entry 32bit uInt pointing to first cluster of directory
 *	@return		error code
 **/
unsigned char	FAT_readFile(unsigned long filenumber, unsigned long dir_first_sector);

/**
 *	FAT_scanDir
 *
 *	scan a directory for songs, set max, min and currentdir variables
 *
 *	@param	directory_cluster unsigned long pointing to director start cluster
 *	@return	number of files found (up to 255)
 **/
unsigned char FAT_scanDir(unsigned long directory_cluster);

/**
 *	FAT_scanDir_lba
 *
 *	scan a directory for songs, set max, min and currentdir variables
 *
 *	@param	lba_addr unsigned long pointing to director starting sector
 *	@return	number of files found (up to 255)
 **/
unsigned char FAT_scanDir_lba(unsigned long lba_addr);

/**
*	FAT_cluster2lba
 *
 *	Calculate the sector address of a cluster.
 *
 *	@param	unsigned long pointing to cluster
 *	@return	lba of first sector in cluster
 **/
unsigned long FAT_cluster2lba(unsigned long cluster);

/**
*	FAT_lba2cluster
 *
 *	Calculate the cluster address of a sector.
 *
 *	@param	lba_addr unsigned long pointing to lba in cluster
 *	@return	cluster containing lba
 **/
unsigned long FAT_lba2cluster(unsigned long lba_addr);

/**
*	FAT_getParentDir
 *		
 *	@param	lba_addr unsigned long pointing to current directory starting sector
 *	@return	first sector of the parent directory or 0 for error
 *
 *	return the first sector of the .. entry. assuming it is the 
 *	second entry in the directory file. otherwise return 0 to do nothing.
 **/
unsigned long FAT_getParentDir(unsigned long lba_addr);

/**
 *	FAT_Scratch2Cluster
 *
 *	Extracts the first cluster from the scratchpad buffer
 *	left after the most recent FAT_readFile() call
 **/
void FAT_Scratch2Cluster(void);

};
#endif