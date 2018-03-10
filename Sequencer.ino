//#define DEBUG_SEQUENCER
/*---------------------------------------------------------------------------*/
/*
Sequencer
By Allen C. Huffman
www.subethasoftware.com

Simple music sequencer. As long as the player routine is called more often
than the shortest notes, it should do a pretty good job at playing music.

REFERENCES:


CONFIGURATION:
1. ...

VERSION HISTORY:
2017-03-01 0.0 allenh - In the beginning...
2017-03-06 0.1 allenh - Using renamed playHandler() function. Comments.
2018-03-07 0.2 allenh - Making note length match CoCo PLAY command.

TODO:
* Use one large buffer, with the restriction that sequences have to be
  added full track at a time. This would work with the PLAY command parser,
  but maybe is not the best generic solution. More thought needed.
* Super-secret features.  

TOFIX:
* ...

*/
/*---------------------------------------------------------------------------*/

#define SEQUENCER_VERSION "0.1"

#include "Sequencer.h"

/*---------------------------------------------------------------------------*/
// DEFINES / ENUMS
/*---------------------------------------------------------------------------*/
#if defined(DEBUG_SEQUENCER)
#define SEQUENCER_PRINT(...)   Serial.print(__VA_ARGS__)
#define SEQUENCER_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
#define SEQUENCER_PRINT(...)
#define SEQUENCER_PRINTLN(...)
#endif

#define MAX_TRACKS  1
#define MAX_STEPS   255 // 255 max

/*---------------------------------------------------------------------------*/
// STATIC GLOBALS
/*---------------------------------------------------------------------------*/

static unsigned long  S_playNextTime[MAX_TRACKS];
static bool           S_trackPlaying[MAX_TRACKS]; // playing or not?
static byte           S_tracksPlaying = 0;
static byte           S_sequencesToPlay = 0;

static MusicStruct    S_sequence[MAX_TRACKS][MAX_STEPS];
static byte           S_nextIn[MAX_TRACKS] = {0};
static byte           S_nextOut[MAX_TRACKS] = {0};
static byte           S_ready[MAX_TRACKS] = {0};

/*---------------------------------------------------------------------------*/
// FUNCTIONS
/*---------------------------------------------------------------------------*/

/*
 * Start squencer.
 */
bool sequencerStart()
{
  bool    status;
  uint8_t track;

  // TODO: We need some kind of error checking here to make sure we aren't
  // being told to play without adding data. We could check the current
  // next in position and see if it was already an END_OF_SEQUENCE, and then
  // ignore.

  if (S_sequencesToPlay < 255)
  {
    SEQUENCER_PRINTLN(F("START SEQUENCE"));
  
    // Initialize
    for (track = 0; track < MAX_TRACKS; track++)
    {
      // Mark end of previous sequence.
      sequencerPut(track, END_OF_SEQUENCE, END_OF_SEQUENCE);
      
      if (S_sequencesToPlay == 0)
      {
        SEQUENCER_PRINTLN(F("Start new sequence."));
        S_playNextTime[track] = 0; //millis();
        S_trackPlaying[track] = true;
        // TODO: fix this, somewhere else.
        S_tracksPlaying = MAX_TRACKS;
      }
    }

    S_sequencesToPlay++;

    SEQUENCER_PRINT(F("Sequences to play: "));
    SEQUENCER_PRINTLN(S_sequencesToPlay);

    status = true;
  }
  else
  {
    status = false;
  }
  
  return status;
} // end of sequencerStart()

/*---------------------------------------------------------------------------*/

bool sequencerStop()
{
  //S_playing = false;

  return true;
}

/*---------------------------------------------------------------------------*/

bool sequencerIsPlaying()
{
  return sequencerHandler();
}

/*---------------------------------------------------------------------------*/

bool sequencerIsReady()
{
  return (sequencerBufferAvailable() > (MAX_STEPS/2));
}

/*---------------------------------------------------------------------------*/
/*
 * Return the largest amount of buffer available. Eventually, we'll use one
 * global buffer and this will be better.
 */
unsigned int sequencerBufferAvailable()
{
  unsigned int track;
  byte largestBufferAvailable;

  sequencerHandler();

  largestBufferAvailable = 0;
  for (track = 0; track < MAX_TRACKS; track++)
  {
    if (S_ready[track] > largestBufferAvailable)
    {
      largestBufferAvailable = (MAX_STEPS - S_ready[track]);
    }
  }

  return largestBufferAvailable;
}

/*---------------------------------------------------------------------------*/
/*
 * Add something to sequencer buffer.
 */
bool sequencerPut(byte track, byte note, byte noteLength)
{
  bool status;
  
  if (S_ready[track] < MAX_STEPS)
  {
    SEQUENCER_PRINT(track);
    SEQUENCER_PRINT(F(":"));
    SEQUENCER_PRINT(S_nextIn[track]);
    SEQUENCER_PRINT(F(" add: "));
    SEQUENCER_PRINT(note);
    SEQUENCER_PRINT(F(", "));
    SEQUENCER_PRINT(noteLength);
    SEQUENCER_PRINT(F(" -> "));

    S_sequence[track][S_nextIn[track]].note = note;
    S_sequence[track][S_nextIn[track]].noteLength = noteLength;
    
    S_nextIn[track]++;
    
    if (S_nextIn[track] >= MAX_STEPS)
    {
      S_nextIn[track] = 0;
    }
    
    S_ready[track]++;
    SEQUENCER_PRINT(F("rdy:"));
    SEQUENCER_PRINT(S_ready[track]);
    SEQUENCER_PRINTLN(F(" "));

    status = true;
  }
  else
  {
    status = false;
  }
  
  return status;
} // end of sequencerAdd()

/*---------------------------------------------------------------------------*/

bool sequencerGet(byte track, byte *note, byte *noteLength)
{
  bool status;

  if (S_ready[track] != 0)
  {
    *note = S_sequence[track][S_nextOut[track]].note;
    *noteLength = S_sequence[track][S_nextOut[track]].noteLength;

    SEQUENCER_PRINT(S_nextOut[track]);
    SEQUENCER_PRINT(F(" get: "));
    SEQUENCER_PRINT(*note);
    SEQUENCER_PRINT(F(", "));
    SEQUENCER_PRINT(*noteLength);
    SEQUENCER_PRINT(F(" -> "));

    // 255 means end of sequence.
    if (*note == END_OF_SEQUENCE)
    {
      status = false;
    }
    else
    {
      status = true;
    }

    S_nextOut[track]++;
    if (S_nextOut[track] >= MAX_STEPS)
    {
      S_nextOut[track] = 0;
    }
    
    S_ready[track]--;
    SEQUENCER_PRINT(F("rdy:"));
    SEQUENCER_PRINT(S_ready[track]);
    SEQUENCER_PRINT(F(" "));
  }
  else
  {
    status = false;
  }

  return status; 
} // end of sequencerGet()

/*---------------------------------------------------------------------------*/

/*
 * Sequencer handler routine. This must be called repeatedly so it can do
 * timing and switch to the next note.
 */
bool sequencerHandler()
{
  unsigned int  track;
  uint8_t       note, noteLength;
  unsigned long timeNow;

  if (S_sequencesToPlay == 0)
  {
    return false;
  }

  timeNow = millis();

  // Loop through note data looking for something to play.
  for (track = 0; track < MAX_TRACKS; track++)
  {
    // Skip if already done.
    if (S_trackPlaying[track] == false)
    {
      continue;
    }

    if (S_playNextTime[track] == 0)
    {
      S_playNextTime[track] = timeNow;
    }

    //if ( (long)(millis()-S_playNextTime[i] >=0 ) )
    if ( (long)(timeNow > S_playNextTime[track]) )
    {
      //note = S_sequence[track][currentNote].note;
      //noteLength = S_sequence[track][currentNote].noteLength;
      if (sequencerGet(track, &note, &noteLength) == false)
      {
        // If we read END for both, that track is done.
        SEQUENCER_PRINT(F("Track "));
        SEQUENCER_PRINT(track);
        SEQUENCER_PRINTLN(F(" out of data."));

        S_trackPlaying[track] = false;
        S_tracksPlaying--; // One less track is playing.
        if (S_tracksPlaying == 0)
        {
          // No tracks playing. Sequence done.
          SEQUENCER_PRINTLN(F("End of sequence."));
          S_sequencesToPlay--;

          SEQUENCER_PRINT(F("Sequences to play: "));
          SEQUENCER_PRINTLN(S_sequencesToPlay);

          // Start next sequence?
          if (S_sequencesToPlay > 0)
          {
            uint8_t i;
            for (i=0; i < MAX_TRACKS; i++)
            {
              S_playNextTime[i] = timeNow;
              S_trackPlaying[i] = true;
            }
            S_tracksPlaying = MAX_TRACKS;
          }
          else
          {
            SEQUENCER_PRINTLN(F("Nothing else to play."));
          }
        }
        break;
      } // end of if (sequencerGet
  
      SEQUENCER_PRINT(F("V"));
      SEQUENCER_PRINT(track, DEC);
      SEQUENCER_PRINT(F(":"));
      SEQUENCER_PRINT(note);
      SEQUENCER_PRINT(F(","));
      SEQUENCER_PRINT(noteLength);
      SEQUENCER_PRINT(F(" "));
  
      unsigned long ms = (noteLength*1000L)/60L;

      // if (note != REST) ?
      tonePlayNote(note, ms);

      S_playNextTime[track] = S_playNextTime[track] + ms;

      SEQUENCER_PRINT(F(" (time: "));
      SEQUENCER_PRINT(S_playNextTime[track]);
      SEQUENCER_PRINTLN(F(") "));
    } // end of if ( (long)(millis() > S_playNextTime[track]) )
  } // end of for (track = 0; track < MAX_TRACKS; track++)

  // Handle note play stuff (ADSR, volume decay, etc.).
  //playHandler();

  // true means tracks are playing, false means they are not.
  return (S_tracksPlaying==0 ? false : true);
  
} // end of sequencerHandler()

/*---------------------------------------------------------------------------*/
// End of Sequencer.ino

