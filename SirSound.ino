//#define SIRSOUNDJR
#define USE_SEQUENCER
//#define USE_SOFTSERIAL
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
* Pass serial bytes directly to PlayParser, rather than using the 255 byte
  buffer. This will make use of the 64-byte Serial buffer. (Must test to see
  if the parser can keep up at 9600 baud to not overflow the 64 byte buffer.)

TOFIX:
* ...

*/
/*---------------------------------------------------------------------------*/

#define SIRSOUND_VERSION "0.4"

#if defined(USE_SOFTSERIAL)
//#include <SoftwareSerial.h>
#include "ReceiveOnlySoftwareSerial.h"
#endif
#include "Sequencer.h"
#include "SN76489.h"

#define TX_PIN_COCO     A4
#define RX_PIN_COCO     A5
#define LED_PIN         13

#define BAUD_COCO       9600
#define BAUD_CONSOLE    115200

#if defined(USE_SOFTSERIAL)
//SoftwareSerial CoCoSerial(RX_PIN_COCO, TX_PIN_COCO); // RX, TX
ReceiveOnlySoftwareSerial CoCoSerial(RX_PIN_COCO); // RX, TX
#else
#define CoCoSerial Serial
#endif

/*---------------------------------------------------------------------------*/

void setup()
{
  Serial.begin(BAUD_CONSOLE);
  Serial.println(F("SirSound " SIRSOUND_VERSION " (" __DATE__ " " __TIME__ ")"));

#if defined(USE_SOFTSERIAL)
  CoCoSerial.begin(BAUD_COCO);
  //CoCoSerial.println(F("SirSound " SIRSOUND_VERSION" (" __DATE__ " " __TIME__ ")"));
#endif
  
#if !defined(SIRSOUNDJR)
  initSN76489();
#endif

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  pinMode(TX_PIN_COCO, OUTPUT);
  digitalWrite(TX_PIN_COCO, LOW); // Ready!

#if !defined(SIRSOUNDJR)
  setVolumeAll(0); // 0=high, 15=silent
  muteAll(); // Just in case...
#endif
  // TODO: Need a PlayParserInit() routine of some kind.
  sequencerInit(BUFFER_SIZE, 0);
  //sequencerInit(BUFFER_SIZE-64, 64);
  clearSubstrings();

  play(F("T8P4O2L4EEP4EP4CEP4GZ"));

#if 0
  // Input and Play routine.
  char buffer[80];
  while(1)
  {
    Serial.print(F("PLAY string (or 'bye')>"));
    lineInput(buffer, sizeof(buffer));
    if (strncmp_P(buffer, PSTR("BYE"), 3)==0) break;
    play(buffer);
    while (sequencerIsReady()==false);
  }
#endif
#if 0
  Serial.println(F("Volume (not implemented):"));
  play(F("Z"));
  play(F("Z V1 C V5 C V10 C V15 C V20 C V25 C V31 C"));
  //while (sequencerIsReady()==false);
  while (sequencerIsPlaying()==true);
  delay(2000);
  
  Serial.println(F("Length test:"));
  play(F("Z"));
  play(F("L1 CDEF L2 CDEF L4 CDEF L8 CDEF L16 CDEF"));
  //while (sequencerIsReady()==false);
  while (sequencerIsPlaying()==true);
  delay(2000);

  Serial.println(F("Octave parsing:"));
  play(F("Z"));
  play(F("O1 C O2 C O3 C O4 C O5 C O4 C O3 C O2 C O1 C"));
  //while (sequencerIsReady()==false);
  while (sequencerIsPlaying()==true);
  delay(2000);
  
  Serial.println(F("Numeric notes."));
  play(F("Z"));
  play(F("1;2;3;4;5;6;7;8;9;10;11;12"));
  //while (sequencerIsReady()==false);
  while (sequencerIsPlaying()==true);
  delay(2000);

  Serial.println(F("Normal notes."));
  play(F("Z"));
  play(F("CDEFGAB"));
  //while (sequencerIsReady()==false);
  while (sequencerIsPlaying()==true);
  delay(2000);

  Serial.println(F("Sharps using #."));
  play(F("Z"));
  play(F("CC#DD#EFF#GG#AA#B"));
  //while (sequencerIsReady()==false);
  while (sequencerIsPlaying()==true);
  delay(2000);

  Serial.println(F("Sharps using +."));
  play(F("Z"));
  play(F("CC+DD+EFF+GG+AA+B"));
  //while (sequencerIsReady()==false);
  while (sequencerIsPlaying()==true);
  delay(2000);

  Serial.println(F("Flats."));
  play(F("Z"));
  play(F("CD-DE-EFG-GA-AB-B"));
  //while (sequencerIsReady()==false);
  while (sequencerIsPlaying()==true);
  delay(2000);
#endif
#if 0
  Serial.println(F("Super Mario Brothers"));
  play(F("Z T6"));
  // From http://www.mariopiano.com/Mario-Sheet-Music-Overworld-Main-Theme.pdf
  // Page 1:
  play(F("O3 L4EEP4EP4CEP4GP4P2O-GP4P2"));
  // Begin repeat:
  play(F("O3 CP2O-G P2EP4 P4AP4B P4B-AP4"
    "O2 L3GO+E L4GO-L4O+A P4FG P4EP4C DO-BP2"
    "O3 CO-P2G P2EP4 P4AP4B P4B-AP4"
    "O3 L3O-GO+E L4GO-L4O+A P4FG P4EP4C DO-BP2"));
  while (sequencerIsReady()==false);

  // Page 2:
  play(F("O3 O-CO+P4GG- FD#O-CO+E O-FO+G#AO+C O-O-CO+AO+CD"
    "O3 O-CO+P4GG- FD#O-CO+E P4O+CP4C CP4O-O-GO+O+P4"
    "O3 O-CO+P4GG- FD#O-CO+E O-FO+G#AO+C O-O-CO+AO+CD"
    "O3 O-CP4O+E-P4 P4DP2 CP2O-O-G GP4CP4 O+O+"));
  while (sequencerIsReady()==false);

  // Page 3 - repeat of 2:
  play(F("O2 CO+P4GG- FD#O-CO+E O-FO+G#AO+C O-O-CO+AO+CD"
    "O2 CO+P4GG- FD#O-CO+E P4O+CP4C CP4O-O-GO+O+P4"
    "O2 CO+P4GG- FD#O-CO+E O-FO+G#AO+C O-O-CO+AO+CD"
    "O2 CP4O+E-P4 P4DP2 CP2O-O-G GP4CP4 O+O+"));
  while (sequencerIsReady()==false);

  // Page 4:
  play(F("O3 CCP4C P4CDP4 ECP4O-A GP4O-GP4"
    "O3 CCP4C P4CDE O-GP2C P2O-GP4 O+"
    "O3 CCP4C P4CDP4 ECP4O-A GP4O-GP4"
    "O3 EEP4E P4CEP4 GP4P2 O-GP4P2"));
  while (sequencerIsReady()==false);

  // Page 5 - repeat of 1 starting 2nd line:
  play(F("O3 CP2O-G P2EP4 P4AP4B P4B-AP4"
    "O3 L3O-GO+E L4GO-L4O+A P4FG P4EP4C DO-BP2"
    "O3 CP2O-G P2EP4 P4AP4B P4B-AP4"
    "O3 L3O-GO+E L4GO-L4O+A P4FG P4EP4C DO-BP2"));
  while (sequencerIsReady()==false);

  // Page 6:
  play(F("O3 ECP4O-G O-GP4O+G#P4 AO+FO-AO+F O-ACO-FP4"
    "O3 L3O-BO+AL4A L3AGL4F ECO-O-GO+A GCO-GP4"
    "O3 ECP4O-G O-GP4O+G#P4 AO+FO-AO+F O-ACO-FP4"
    "O2 BO+FP4F L3FEL4D CO-EGE CP4P2"));
  while (sequencerIsReady()==false);

  // Page 7 - repeat of 6:
  play(F("O3 ECP4O-G O-GP4O+G#P4 AO+FO-AO+F O-ACO-FP4"
    "O3 L3O-BO+AL4A L3AGL4F ECO-O-GO+A GCO-GP4"
    "O3 ECP4O-G O-GP4O+G#P4 AO+FO-AO+F O-ACO-FP4"
    "O2 BO+FP4F L3FEL4D CO-EGE CP4P2"));
  while (sequencerIsReady()==false);

  // Page 8 - repeat of page 4:
  play(F("O3 CCP4C P4CDP4 ECP4O-A GP4O-GP4"
    "O3 CCP4C P4CDE O-GP2C P2O-GP4 O+"
    "O3 CCP4C P4CDP4 ECP4O-A GP4O-GP4"
    "O3 EEP4E P4CEP4 GP4P2 O-GP4P2"));
  while (sequencerIsReady()==false);

  // Page 9 - repeat of page 6:
  play(F("O3 ECP4O-G O-GP4O+G#P4 AO+FO-AO+F O-ACO-FP4"
    "O3 L3O-BO+AL4A L3AGL4F ECO-O-GO+A GCO-GP4"
    "O3 ECP4O-G O-GP4O+G#P4 AO+FO-AO+F O-ACO-FP4"
    "O2 BO+FP4F L3FEL4D CO-EGEC"));
  while (sequencerIsReady()==false);
  
  while (sequencerIsPlaying()==true);
  delay(2000);

  // Repeat End

  Serial.println(F("Example from the Extended Color BASIC manual."));
  play(F("Z"));
  play(F("T5;C;E;F;L1;G;P4;L4;C;E;F;L1;G"));
  while (sequencerIsReady()==false);
  play(F("P4;L4;C;E;F;L2;G;E;C;E;L1;D"));
  while (sequencerIsReady()==false);
  play(F("P8;L4;E;E;D;L2.;C;L4;C;L2;E"));
  while (sequencerIsReady()==false);
  play(F("L4;G;G;G;L1;F;L4;E;F"));
  while (sequencerIsReady()==false);
  play(F("L2;G;E;L4;C;L8;D;D+;D;E;G;L4;A;L1;O3;C"));
  while (sequencerIsReady()==false);

  while (sequencerIsPlaying()==true);
  delay(2000);

/*
Relative octave jumps:
Peter Gunn: L8 CCDCL>DD+L<CFE
Popeye:     L16 EGGGL<FL>EL<GL>

Frogger (2-voice):
T2O2 L8 A+F+F+F+ A+F+F+F+ BBA+A+L4G+P4 L8BBA+A+L4G+ O+L8C+D+C+O-BA+G+L2F+ ,
T2O1 L8 F+A+C+A+F+A+C+A+ G+BC+BG+BC+B G+BC+BG+BC+B G+BC+BL2F+

^T2O4L32CGO+GO-

Donkey Kong:
T3O1L8CP4L8EP8L8GAG

Bach - 2-Part Invention 13
T2 O2 P16 L16 E A O+ C O- B E B O+ D L8 C E O- G# O+ E,
O1 L8 A L4 O+ A L8 G# L16 A E A O+ C O- B E B O+ D

*/

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
  while (sequencerIsReady()==false);
  play(F("CO5CO4BAGCACGCFC EGDGEGFGEGDGCO5CO4BAGCACGCFC EO3CO4CO3CBCO4CO3"));
  while (sequencerIsReady()==false);
  play(F("CO4DO3CBC O4CO3CO4EO3CO4DO3CO4EO3CO4FO3CO4DO3C O4ECO3CO4CO2BO4CO3CO4CO3DO4CO2BO4C O3CO4CO3EO4CO3DO4CO3EO4CO3F"));
  while (sequencerIsReady()==false);
  play(F("O4CO3DO4C O3CDCDEGCDEGCD EFEFGO4CO3EFGO4CO3EF GAGAB-O4EO3GAB-O4EO3GA B-O4GECO3B-GECO2B-AB-G ABABO3C#EO2ABO3C#EO2AB"));
  while (sequencerIsReady()==false);
  play(F("O3C#DC#DEAC#DEAC#D EFEFGO4C#O3EFGO4C#O3EF GO4EC#O3AO4GEFDC#EO3AG FAFDO4DO3BO4CO3AG#BED CECO2AO3CEAEO4CO3AO4EO3A"));
  while (sequencerIsReady()==false);
  play(F("G#BG#EO4ED#ED#EO3BO4CO3A G#BG#EO4DC#DC#DO3BO4CO3A G#BG#EAEG#EL8AF L4EO4DP8L8O3B L4O4CO3AP8O4L8D# L4EO5DP8L8O4B L4O5CO4AP8L8O3A"));
  while (sequencerIsReady()==false);
  play(F("L16 CD#AD#O4CO3D#AD#O4CO3D#AD# CD#AD#O4CO3D#AD#O4CO3D#AD# O2BO3EAEBEAEBEAE DEG#EBEG#EBEG#E AO4AGFEAEDCECO3B AO4AEDCECO3BAO4CO3AG L8F#DF#AL4O4C L16O3BO4GF#EDGDCO3BO4DO3BA GO4GFEDFDCO3BO4DO3BA"));
  while (sequencerIsReady()==false);
  play(F("L2O4FL16FEFD ECEGO5CO4GECP16O4GO5CO4B"));
  while (sequencerIsReady()==false);
  play(F("L4O5CO4L2G L8GL16FEL2F L8FL16EDL4EL16ECO3GO4C O3FO4CDCO3BGBO4CDO3GO4DE L4.FL8GL4E L16EFEL32FDL32DEDEDEDEDEDEDEDEL4.DL8C L16CO5CO4BAGO5CO4FO5CO4EO5CO4DO5C O4CO5CO4BAGCFCECDC P16O3EGBO4CEGBL4O5C"));
  while (sequencerIsPlaying()==true);
#endif
#if 0
  Serial.println(F("Frogger"));
  play(F("T2O1 L8 F+A+C+A+F+A+C+A+ G+BC+BG+BC+B G+BC+BG+BC+B G+BC+BL2F+,"
         "T2O2 L8 A+F+F+F+ A+F+F+F+ BBA+A+L4G+P4 L8BBA+A+L4G+ O+L8C+D+C+O-BA+G+L2F+"));
  while (sequencerIsPlaying()==true);
  //delay(2000);
#endif
} // end of setup()

void loop()
{
  int incomingByte;
  char buffer[255];
  byte pos = 0;
  bool lastSequencerStatus;
  bool sequencerStatus;

  digitalWrite(LED_PIN, HIGH);

  lastSequencerStatus = sequencerIsReady();
  Serial.print(F("Sequencer Status: "));
  Serial.println(lastSequencerStatus);

  Serial.println(F("loop"));
  while(1)
  {
    sequencerStatus = sequencerIsReady();
    if (sequencerStatus != lastSequencerStatus)
    {
      Serial.print(F("Sequencer Status: "));
      Serial.println(lastSequencerStatus);

      if (sequencerStatus == true)
      {
        digitalWrite(TX_PIN_COCO, LOW);
        digitalWrite(LED_PIN, HIGH);
      }
      else
      {
        digitalWrite(TX_PIN_COCO, HIGH);
        digitalWrite(LED_PIN, LOW);
      }
      lastSequencerStatus = sequencerStatus;
    }
    
    // send data only when you receive data:
    if (CoCoSerial.available() > 0)
    {
      //digitalWrite(LED_PIN, LOW);
  
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
      }
    }

    sequencerHandler();
  }// end while
} // end of loop()

/*---------------------------------------------------------------------------*/
// End of SirSound

