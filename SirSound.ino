#define SIRSOUNDJR
/*---------------------------------------------------------------------------*/
/*
SirSound serial sound card driver
By Allen C. Huffman
www.subethasoftware.com

Takes serial data and sends it to the sound chip.

REFERENCES:

CONFIGURATION:

VERSION HISTORY:
2017-03-01 0.0 allenh - In the beginning...
2017-03-26 0.1 allenh - SirSound based on initial sequencer work.
2018-02-17 0.2 allenh - Corrected CoCo serial port bug.
2018-02-18 0.3 allenh - Updating for NANO and SoftwareSerial.

TODO:
* Support various serial modes.

TOFIX:
* ...

*/
/*---------------------------------------------------------------------------*/

#define SIRSOUND_VERSION "0.3"

#include <SoftwareSerial.h>
#include "Sequencer.h"

#include "SN76489.h"        // for NOTES, etc.

#define TX_PIN_COCO     A4
#define RX_PIN_COCO     A5
#define LED_PIN         13

#define BAUD_COCO       1200
#define BAUD_CONSOLE    9600

SoftwareSerial CoCoSerial(RX_PIN_COCO, TX_PIN_COCO); // RX, TX

#define CoCoSerial Serial

MusicStruct g_track0[] = // Bass
{
  { NB2, L8DOTTED },
  { NB3, L16 },
  { NB2, L8DOTTED },
  { NB3, L16 },

  { NC3, L8DOTTED },
  { NC4, L16 },
  { NC3, L8DOTTED },
  { NC4, L16 },

  { NB2, L8DOTTED },
  { NB3, L16 },
  { NB2, L8DOTTED },
  { NB3, L16 },

  { NF3S, L8 },
  { NG3S, L8 },
  { NA3S, L8 },
  { NB3, L8 },

  { END, END } // END
};

MusicStruct g_track1[] =
{
  { NB3, L16 },
  { NB4, L16 },
  { NF4S, L16 },
  { ND4S, L16 },
  { NB4, L32 },
  { NF4S, L16DOTTED },
  { ND4S, L8 },

  { NC4, L16 },
  { NC5, L16 },
  { NG4, L16 },
  { NE4, L16 },
  { NC5, L32 },
  { NG4, L16DOTTED },
  { NE4, L8 },

  { NB3, L16 },
  { NB4, L16 },
  { NF4S, L16 },
  { ND4S, L16 },
  { NB4, L32 },
  { NF4S, L16DOTTED },
  { ND4S, L8 },

  { ND4S, L32 },
  { NE4, L32 },
  { NF4S, L16 },

  { NF4S, L32 },
  { NG4, L32 },
  { NG4S, L16 },

  { NG4S, L32 },
  { NA4, L32 },
  { NA4S, L16 },
  { NB4, L8 },

  { END, END }
};

MusicStruct g_test[] =
{
  { NC1, L16 },
  { NC2, L16 },
  { NC3, L16 },
  { NC4, L16 },
  { END, END }
};

void setup()
{
  Serial.begin(BAUD_CONSOLE);
  Serial.println(F("SirSound " SIRSOUND_VERSION " (" __DATE__ " " __TIME__ ")"));

  // Hardware serial port. Maybe.
  //CoCoSerial.begin(BAUD_COCO);
  //CoCoSerial.println(F("SirSound " SIRSOUND_VERSION" (" __DATE__ " " __TIME__ ")"));

#if !defined(SIRSOUNDJR)
  initSN76489();
#endif

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  // Notes about using an Arduino pin to generate the 4Mhz pulse:

  // For the Teensy 2.0, this is how to make a pin act as a 4MHz pulse.
  // I am using this on my Teensy 2.0 hardware for testing. This can be
  // done on other Arduino models, too, but I have only been using my
  // Teensy for this so far. My original NANO prototype is using an
  // external crystal.

  // For the Teensy 2.0, this
  //pinMode(14, OUTPUT);
  // Turn on toggle pin mode.
  //TCCR1A |= ((1<<COM1A1));
  // Set CTC mode (mode 4), and set clock to be CPU clock
  //TCCR1B |= ((1<<WGM12) | (1<<CS10));
  // Count to one and then reset and count again.  Since CPU is 16MHz,
  // this will divide clock by 2 and action on the pin.  Since we will be
  // toggling the pin, that will divide by 2 again, giving /4 or 4MHz
  //OCR1A = 1;

#if !defined(SIRSOUNDJR)
#if defined(TEENSY20)
  // Make pin 14 be a 4MHz signal.
  pinMode(14, OUTPUT);
  TCCR1A = 0x43;  // 01000011 
  TCCR1B = 0x19;  // 00011001
  OCR1A = 1;
#elif defined(ARDUINO_AVR_NANO)
  // Make pin 3 be a 4MHz signal.
  pinMode(3, OUTPUT);
  TCCR2A = 0x23;  // 00100011
  TCCR2B = 0x09;  // 00001001
  OCR2A = 3;
  OCR2B = 1;  
#else
#error 4MHz stuff not defined.
#endif
#endif

#if !defined(SIRSOUNDJR)
  setMaxVolume( 0 ); // 0=high, 15=silent

  muteAll(); // Just in case...
#endif

  Serial.println(F("Sequencer Test."));

  // Sequencer test.
  int i=0;
  do
  {
    sequencerPut(0, g_test[i].note, g_test[i].noteLength);
    i++;
  } while(g_test[i].note != END);

  bool isPlaying;
  
  isPlaying = sequencerStart();

  while( isPlaying == true )
  {
    isPlaying = sequencerHandler();
  };

#if 0
  // Input and Play routine.
  char buffer[80];
  while(1)
  {
    Serial.print(F("PLAY string (or 'bye')>"));
    lineInput(buffer, sizeof(buffer));
    //if (strncmp(buffer, "BYE", 3)==0) break;
    if (strncmp_P(buffer, PSTR("BYE"), 3)==0) break;
    play(buffer);
  }
  
/*
  Serial.println(F("Volume (not implemented):"));
  play(F("Z"));
  play(F("V1 C V5 C V10 C V15 C V20 C V25 C V31 C"));
  delay(2000);
  
  Serial.println(F("Length test:"));
  play(F("Z"));
  play(F("L1 CDEF L2 CDEF L4 CDEF L8 CDEF L16 CDEF"));
  delay(2000);

  Serial.println(F("Octave parsing:"));
  play(F("Z"));
  play(F("O1 C O2 C O3 C O4 C O5 C O4 C O3 C O2 C O1 C"));
  delay(2000);
  
  Serial.println(F("Numeric notes."));
  play(F("Z"));
  play(F("1;2;3;4;5;6;7;8;9;10;11;12"));
  delay(2000);

  Serial.println(F("Normal notes."));
  play(F("Z"));
  play(F("CDEFGAB"));
  delay(2000);

  Serial.println(F("Sharps using #."));
  play(F("Z"));
  play(F("CC#DD#EFF#GG#AA#B"));
  delay(2000);

  Serial.println(F("Sharps using +."));
  play(F("Z"));
  play(F("CC+DD+EFF+GG+AA+B"));
  delay(2000);

  Serial.println(F("Flats."));
  play(F("Z"));
  play(F("CD-DE-EFG-GA-AB-B"));
  delay(2000);  
*/
  Serial.println(F("Example from the Extended Color BASIC manual."));
  play(F("Z"));
  play(F("T5;C;E;F;L1;G;P4;L4;C;E;F;L1;G"));
  play(F("P4;L4;C;E;F;L2;G;E;C;E;L1;D"));
  play(F("P8;L4;E;E;D;L2.;C;L4;C;L2;E"));
  play(F("L4;G;G;G;L1;F;L4;E;F"));
  play(F("L2;G;E;L4;C;L8;D;D+;D;E;G;L4;A;L1;O3;C"));
  delay(2000);

  // Relative octave jumps:
  // Peter Gunn: L8 CCDCL>DD+L<CFE
  // Popeye:     L16 EGGGL<FL>EL<GL>
/*
1000 CLS:PRINT@43,"SINFONIA"
1010 PRINT@73,"BY J.S. BACH"
1020 PRINT@139,"ARRANGED"
1030 PRINT@165,"FOR THE COLOR COMPUTER"
1040 PRINT@206,"BY"
1050 PRINT@233,"TOMMY POLLOCK"
1060 PRINT@269,"AND"
1070 PRINT@298,"GAIL POLLOCK"
1080 PRINT@357,"PRESS ANY KEY TO BEGIN"
*/
  Serial.println(F("Sinfonia (Bach) arranged for CoCo by Tommy & Gail Pollok"));
  play(F("Z"));
  play(F("O2L8CO5L16CO4BO5L8CO4GEGL16CDCO3BO4L8CO3GEGL16CGDGEGFGEGDGCO4CO3BAGO4CO3BAGFEDCGDGEGFGEGDGCO4CO3BAGO4CO3BAGFEDCDEFGABO4CDEFDEGCDEFGABO5CO4ABO5CO4GFGEGFGEGDG"));
  play(F("CO5CO4BAGCACGCFC EGDGEGFGEGDGCO5CO4BAGCACGCFC EO3CO4CO3CBCO4CO3"));
  play(F("CO4DO3CBC O4CO3CO4EO3CO4DO3CO4EO3CO4FO3CO4DO3C O4ECO3CO4CO2BO4CO3CO4CO3DO4CO2BO4C O3CO4CO3EO4CO3DO4CO3EO4CO3F"));
  play(F("O4CO3DO4C O3CDCDEGCDEGCD EFEFGO4CO3EFGO4CO3EF GAGAB-O4EO3GAB-O4EO3GA B-O4GECO3B-GECO2B-AB-G ABABO3C#EO2ABO3C#EO2AB"));
  play(F("O3C#DC#DEAC#DEAC#D EFEFGO4C#O3EFGO4C#O3EF GO4EC#O3AO4GEFDC#EO3AG FAFDO4DO3BO4CO3AG#BED CECO2AO3CEAEO4CO3AO4EO3A"));
  play(F("G#BG#EO4ED#ED#EO3BO4CO3A G#BG#EO4DC#DC#DO3BO4CO3A G#BG#EAEG#EL8AF L4EO4DP8L8O3B L4O4CO3AP8O4L8D# L4EO5DP8L8O4B L4O5CO4AP8L8O3A"));
  play(F("L16 CD#AD#O4CO3D#AD#O4CO3D#AD# CD#AD#O4CO3D#AD#O4CO3D#AD# O2BO3EAEBEAEBEAE DEG#EBEG#EBEG#E AO4AGFEAEDCECO3B AO4AEDCECO3BAO4CO3AG L8F#DF#AL4O4C L16O3BO4GF#EDGDCO3BO4DO3BA GO4GFEDFDCO3BO4DO3BA"));
  play(F("L2O4FL16FEFD ECEGO5CO4GECP16O4GO5CO4B"));
  play(F("L4O5CO4L2G L8GL16FEL2F L8FL16EDL4EL16ECO3GO4C O3FO4CDCO3BGBO4CDO3GO4DE L4.FL8GL4E L16EFEL32FDL32DEDEDEDEDEDEDEDEL4.DL8C L16CO5CO4BAGO5CO4FO5CO4EO5CO4DO5C O4CO5CO4BAGCFCECDC P16O3EGBO4CEGBL4O5C"));
  delay(2000);  
#endif
} // end of setup()

void loop()
{
  int incomingByte;
  char buffer[255];
  byte pos = 0;

  digitalWrite(LED_PIN, HIGH);

  while(1)
  {
    // send data only when you receive data:
    if (CoCoSerial.available() > 0)
    {
      digitalWrite(LED_PIN, LOW);
  
      // read the incoming byte:
      incomingByte = CoCoSerial.read();
  
      if (incomingByte >= 0)
      {
        if (incomingByte == 13)
        {
          buffer[pos] = '\0';
          play(buffer);
          pos = 0;
        }
        else
        {
          buffer[pos] = incomingByte;
          if (pos<255)
          {
            pos++;
          }
          else
          {
            play(buffer);
            pos = 0;
          }
        }
        //poke(incomingByte);
        //Serial.print(incomingByte, HEX);
      }
    }
#if !defined(SIRSOUNDJR)
    playHandler();
#endif
    sequencerHandler();
  }// end while
} // end of loop()

/*---------------------------------------------------------------------------*/
// End of SirSound

