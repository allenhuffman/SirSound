#ifndef SEQUENCER_H
#define SEQUENCER_H
/*---------------------------------------------------------------------------*/
/*
SN76489 sound chip routines
By Allen C. Huffman
www.subethasoftware.com

Header file with Music Sequencer definitions.

VERSION HISTORY:
2017-03-01 0.0 allenh - In the beginning...
2017-03-06 0.1 allenh - Adding END and REST defines. Fixing lowest note.
2018-03-07 0.2 allenh - Making note length match CoCo PLAY command.
                        Renaming duration to noteLength.

TODO:
* ...

TOFIX:
* ...

*/
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// STRUCTURES
/*---------------------------------------------------------------------------*/

typedef struct {
  byte note;
  byte noteLength;
} MusicStruct;

/*---------------------------------------------------------------------------*/
// DEFINES / ENUMS
/*---------------------------------------------------------------------------*/

// These are set to the lowest and highest note the chip can play.
//#define LOWEST_NOTE   NB2

//#define HIGHEST_NOTE  NC8 // It is really NC9, beyond piano.

#define CMD_BIT             0b10000000 // bit(7)
#define CMD_MASK            0b01110000
#define CMD_REPEAT          0b10010000

#define END_OF_SEQUENCE     0b11111111 // 255

//
// These are defines for the 88 notes on a piano. Not all the notes are
// actually playable when using a 4Mhz crystal (?), but they are defined
// since this code could will be used for other sound chips.
//
/* ___________________________________________      _______________________
   # # | # # | # # # | # # | # # # | # # | # #......# # # | # # | # # # | #
   # # | # # | # # # | # # | # # # | # # | # #......# # # | # # | # # # | #
   |_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|......|_|_|_|_|_|_|_|_|_|_|_|
    A B C D E F G A B C D E F G A B C D E F G        G A B C D E F G A B C 
    0 0 1 1 1 1 1 1 1 2 2 2 2 2 2 2 3 3 3 3 3        6 6 6 7 7 7 7 7 7 7 8 
*/
// 88 piano notes (sharps)
enum sharps {
  NA0, NA0S, NB0, // 3
  NC1, NC1S, ND1, ND1S, NE1, NF1, NF1S, NG1, NG1S, NA1, NA1S, NB1, // 12
  NC2, NC2S, ND2, ND2S, NE2, NF2, NF2S, NG2, NG2S, NA2, NA2S, NB2, // 12
  NC3, NC3S, ND3, ND3S, NE3, NF3, NF3S, NG3, NG3S, NA3, NA3S, NB3, // 12
  NC4, NC4S, ND4, ND4S, NE4, NF4, NF4S, NG4, NG4S, NA4, NA4S, NB4, // 12
  NC5, NC5S, ND5, ND5S, NE5, NF5, NF5S, NG5, NG5S, NA5, NA5S, NB5, // 12
  NC6, NC6S, ND6, ND6S, NE6, NF6, NF6S, NG6, NG6S, NA6, NA6S, NB6, // 12
  NC7, NC7S, ND7, ND7S, NE7, NF7, NF7S, NG7, NG7S, NA7, NA7S, NB7, // 12
  NC8 // 1
};

#if 0
// 88 paino notes (flats)
enum flats {
  NA0, NB0F, NB0, // 3
  NC1, ND1F, ND1, NE1F, NE1, NF1, NG1F, NG1, NA1F, NA1, NB1F, NB1, // 12
  NC2, ND2F, ND2, NE2F, NE2, NF2, NG2F, NG2, NA2F, NA2, NB2F, NB2, // 12
  NC3, ND3F, ND3, NE3F, NE3, NF3, NG3F, NG3, NA3F, NA3, NB3F, NB3, // 12
  NC4, ND4F, ND4, NE4F, NE4, NF4, NG4F, NG4, NA4F, NA4, NB4F, NB4, // 12
  NC5, ND5F, ND5, NE5F, NE5, NF5, NG5F, NG5, NA5F, NA5, NB5F, NB5, // 12
  NC6, ND6F, ND6, NE6F, NE6, NF6, NG6F, NG6, NA6F, NA6, NB6F, NB6, // 12
  NC7, ND7F, ND7, NE7F, NE7, NF7, NG7F, NG7, NA7F, NA7, NB7F, NB7, // 12
  NC8 // 1
};
#endif

#define REST      0xfe      // 0 is a special case for no note
#define END       0xff      // end of note table flag

// Durations
#define L128      128       // 128th note

#define L64       64        // 64th note

#define L32DOTTED (L32+L64) // dotted 32nd note
#define L32       32        // 32nd note

#define L16DOTTED (L16+L32) // dotted 16 note
#define L16       16        // 16th note

#define L8DOTTED  (L8+L16)  // dotted 8th note
#define L8        8         // 8th note

#define L4DOTTED  (L4+L8)   // dotted quarter note
#define L4        4         // quareter note

#define L2DOTTED  (L2+L4)   // dotted half note
#define L2        2         // half note

#define L1DOTTED  (L1+L2)   // dotted whole note
#define L1        1         // whole note

/*---------------------------------------------------------------------------*/
// EXTERNAL PROTOTYPES
/*---------------------------------------------------------------------------*/

bool sequencerStart();
bool sequencerStop();
bool sequencerIsPlaying();
bool sequencerIsReady();
bool sequencerPut(byte track, byte value);
bool sequencerPutNote(byte track, byte note, unsigned int noteLength);
bool sequencerGet(byte track, byte *value);
//bool sequencerGet(byte track, byte *note, unsigned int *noteLength);
bool sequencerHandler();

unsigned int sequencerBufferAvailable();

#endif // SEQUENCER_H

/*---------------------------------------------------------------------------*/
// End of Sequencer.h

