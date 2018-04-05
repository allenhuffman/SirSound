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
2018-04-03     allenh - Updated with standard-C defines for testing.

TODO:
* ...

TOFIX:
* ...

*/
/*---------------------------------------------------------------------------*/
#if defined(C)
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <ctype.h>
typedef uint8_t byte;
#define F(s) s

void playHandler();
void setVolume(uint8_t track, uint8_t value);
void playNote(uint8_t track, uint8_t note);
unsigned int millis();
#endif // defined

/*---------------------------------------------------------------------------*/
// STRUCTURES
/*---------------------------------------------------------------------------*/

typedef struct
{
  unsigned int  start;
  unsigned int  nextIn;
  unsigned int  nextOut;
  unsigned int  end;
  unsigned int  ready;
} SequencerTrackStruct;

typedef struct
{
  unsigned long playNextTime;
  unsigned int  repeatStart;
  byte          repeatCount;
  byte          trackStatus;
} SequencerStruct;

typedef struct
{
  unsigned int  addStart;
  unsigned int  start;
  unsigned int  length; // to speed things up.
} SequencerSubstringStruct;


/*---------------------------------------------------------------------------*/
// DEFINES / ENUMS
/*---------------------------------------------------------------------------*/

#if defined(SIRSOUNDJR)
#define MAX_TRACKS 1
#else
#define MAX_TRACKS 3
#endif

//#define BUFFER_SIZE 300
#define BUFFER_SIZE (38*MAX_TRACKS)

#define MAX_SUBSTRINGS      16  // 0-15

#define NOTE_MASK           0b01111111 // 0-127 only for notes.

#define CMD_BIT             0b10000000 // bit(7)
#define CMD_MASK            0b11110000
#define CMD_VALUE_MASK      0b00001111

#define CMD_VOLUME          0b10000000 // 0
#define CMD_REPEAT          0b10010000 // 1
#define CMD_INTERRUPT       0b10100000 // 2
#define CMD_ADD_SUBSTRING   0b10110000 // 3
#define CMD_DEL_SUBSTRING   0b11000000 // 4
#define CMD_PLAY_SUBSTRING  0b11010000 // 5
#define CMD_6               0b11100000 // 6
#define CMD_END_SEQUENCE    0b11110000 // 7

#define NOTE_REST           0b01111111 // 127

enum {
  TRACK_IDLE,
  TRACK_PLAYING,
  TRACK_COMPLETE,
  TRACK_INTERRUPTED
};

#define NOT_PLAYING 0xffff // max unsigned int

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

// These are set to the lowest and highest note the chip can play.
//#define LOWEST_NOTE   NB2

//#define HIGHEST_NOTE  NC8 // It is really NC9, beyond piano.

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
// 88 piano notes (flats)
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

bool sequencerInit(unsigned int bufferSize);
bool sequencerStart();
bool sequencerStop();
bool sequencerInterrupt();
bool sequencerIsPlaying();
bool sequencerIsReady();

bool sequencerPutByte(byte track, byte value);
bool sequencerPutNote(byte track, byte note, unsigned int noteLength);
bool sequencerGetByte(byte track, byte *value, bool cmdByteCheck);

bool sequencerHandler();

unsigned int sequencerGetLargestFreeBufferAvailable();
unsigned int sequencerGetSmallestFreeBufferAvailable();

void sequencerShowBufferInfo();
void sequencerShowTrackInfo(byte track);
bool sequencerOptimizeBuffer();

bool sequencerAddSubstringBuffer(unsigned int bytesToAdd);
bool sequencerOptimizeSubstringBuffer();

#if MAX_SUBSTRINGS > 16
#error Sequencer can only support up to 16 substrings.
#endif

#endif // SEQUENCER_H

/*---------------------------------------------------------------------------*/
// End of Sequencer.h

