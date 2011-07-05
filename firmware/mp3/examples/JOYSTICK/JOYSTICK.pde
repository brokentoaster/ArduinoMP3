//
// 
// Arduino MP3 Shield Joystick test file
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

#define BAUD 115200
#define DEBOUNCE_VALUE 100

#define joystick_A  8   // up
#define joystick_B  7   // right 
#define joystick_C  6   // left
#define joystick_D  5   // down
#define joystick_E  4   // center


//////////////////////////////////// SETUP
void setup()
{ 
        // Set up the Joystick pins as input
        pinMode(joystick_A,INPUT);
        pinMode(joystick_B,INPUT);
        pinMode(joystick_C,INPUT);
        pinMode(joystick_D,INPUT);
        pinMode(joystick_E,INPUT);

        // enable pullups on the pins
        digitalWrite(joystick_A,HIGH);
        digitalWrite(joystick_B,HIGH);
        digitalWrite(joystick_C,HIGH);
        digitalWrite(joystick_D,HIGH);
        digitalWrite(joystick_E,HIGH);

        // initialise the joystick
        Serial.begin(BAUD);
        Serial.println("Arduino MP3 Shield Joystick Test");
}



/***************************************************************************
*	Name:	        check_joystick
*	Description:    read and debouce the joystick 
*	Parameters:	none
*	Returns:	none
***************************************************************************/
void check_joystick(void)
{  
        static char last;
        static char printed;
        static char count;
        char now;
        char i;

        now = 0; // set the current position to center

        // check the inputs of the 5 way joystick 
        for (i=joystick_A;i>=joystick_E;i--){
                if (digitalRead(i)==0){
                        now = i; // remeber the last reading to be held low.
                }
        }

        // check for change
        if (now == last){
                count++;
        }else{
                count = 0 ;  // reset signal timer
                printed = 0; // reset printed flag
        }

        // remeber joystick position for next time
        last = now;

        // only act if signal has been consistant for specified time
        if (count > DEBOUNCE_VALUE){
                count= DEBOUNCE_VALUE; // set to stop rollover at 256

                // Have we got an actual button push and not printed it yet 
                if ((now>0)&&(printed==0)) {
                        printed = 1; // only print the button detect once

                        Serial.print("Joystick : ");
                        switch ( now){
                                case joystick_E: 
                                          Serial.println("PUSH");
                                          break;
                                  
                                case joystick_D: 
                                          Serial.println("DOWN");
                                          break;
                                  
                                case joystick_C: 
                                          Serial.println("LEFT");
                                          break;
                                  
                                case joystick_B: 
                                          Serial.println("RIGHT");
                                          break;

                                case joystick_A: 
                                          Serial.println("UP");
                                          break;
                        }
                }
        }
}



//////////////////////////////////// LOOP
void loop() 
{
   check_joystick();
}


