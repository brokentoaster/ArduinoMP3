//
// 
// Arduino MP3 Shield VS1011 Test file
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


// This is a Test for just the MP3 player chip and NOT MMC/SD card routines
// Should start up the board and then playsome sine wave tones.

#define BAUD 115200
#define FAT_NumberedSong_EN 1

#include <utils.h>
#include <mmc.h>
#include <vs1001.h>
#include <types.h>

MMC card;              // not used but doesn't work with out it 
vs1001 player;

char mp3_cs = 3;
char mp3_bsync = 2;
char mp3_dreq  = 15;
char mp3_reset = 14 ;
char debugpin = 8;

//////////////////////////////////// SETUP
void setup()
{
        char r,i;
        unsigned long lr;

        pinMode(debugpin,OUTPUT);           // setup debug trigger
        digitalWrite(debugpin, HIGH);

        digitalWrite(mp3_dreq, HIGH);       // turn on pullup resistors
        digitalWrite(mp3_bsync, LOW);       // turn off BSYNC
        digitalWrite(mp3_reset, LOW);       // turn off VS1003 
        digitalWrite(mp3_cs, HIGH);         // turn off VS10xx SPI

        pinMode(mp3_dreq, INPUT);           // set pin to input
        pinMode(mp3_bsync,OUTPUT);          // set pin to output
        pinMode(mp3_reset,OUTPUT);          // set pin to output
        pinMode(mp3_cs,OUTPUT);             // set pin to output

        // Init Serial comms and send a test message
        Serial.begin(BAUD);
        Serial.println("Arduino MP3 Shield VS1011 Test");       

        // Run some tests on the VS1011 chip
        Serial.print("Init IO ... ");
        player.init_io();
        Serial.println("OK");
        Serial.print("Init_chip ... ");
        player.init_chip();
        Serial.println("OK");     
        Serial.print("Sine_test ... "); 
        player.sine_test();
        Serial.println("OK");

        // Read out all the regesters
        for (i=0;i<16;i++){
                delay(10);
                Serial.print("Read Reg ");
                Serial.print(i,DEC);
                Serial.print(": ");
                Serial.println(readReg(i),HEX);  
        }
        
        // Done.
        Serial.println("DONE");
}

//////////////////////////////////// LOOP
void loop() 
{
        // Do nothing
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
*       Name:	      readReg
*	Description:  read a register from the VS1011 and return 16 Bit response
*	Parameters:   <reg> byte address of register to read
*	Returns:      16 Bit data returned from the register
***************************************************************************/
uint16 readReg(uint8 reg)
{
        uint16 data;
        player.read(reg, 2,&data);
        return data;
}


