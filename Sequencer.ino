#define DEBUG_SEQUENCER
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
2018-03-13 0.3 allenh - Improve time sync between tracks.
2018-03-13 0.4 allenh - Rewrite sequencer buffer system.
2018-03-13 0.5 allenh - Adding STOP.
2018-03-14 0.6 allenh - Adding REPEAT.

TODO:
* Use one large buffer, with the restriction that sequences have to be
  added full track at a time. This would work with the PLAY command parser,
  but maybe is not the best generic solution. More thought needed.
* Super-secret features.

TOFIX:
* It does NOT handle the buffer being full well at all. When this happens,
  maybe it should just halt everything to indicate the condition, rather
  than trying to play bad data.

*/
/*---------------------------------------------------------------------------*/

#define SEQUENCER_VERSION "0.5"

#include "Sequencer.h"

void tonePlayNote(byte note, unsigned long duration);

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

#if defined(SIRSOUNDJR)
#define MAX_TRACKS 1
#else
#define MAX_TRACKS  3
#endif
#define BUFFER_SIZE 300 // 255 max

/*---------------------------------------------------------------------------*/
// STATIC GLOBALS
/*---------------------------------------------------------------------------*/

static unsigned long  S_playNextTime[MAX_TRACKS];
static bool           S_trackPlaying[MAX_TRACKS]; // playing or not?
static byte           S_tracksPlaying = 0;
static byte           S_sequencesToPlay = 0;

static byte           S_sequence[MAX_TRACKS][BUFFER_SIZE];
static unsigned int   S_nextIn[MAX_TRACKS] = {0};
static unsigned int   S_nextOut[MAX_TRACKS] = {0};
static unsigned int   S_ready[MAX_TRACKS] = {0};
static unsigned int   S_repeatStart[MAX_TRACKS] = {0};
static byte           S_repeatCount[MAX_TRACKS] = {0};


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
    SEQUENCER_PRINTLN(F("* Start Sequence."));
  
    // Initialize
    for (track = 0; track < MAX_TRACKS; track++)
    {
      // Mark end of previous sequence.
      sequencerPutByte(track, CMD_END_SEQUENCE);
      
      if (S_sequencesToPlay == 0)
      {
        SEQUENCER_PRINT(F("T"));
        SEQUENCER_PRINT(track);
        SEQUENCER_PRINTLN(F(" Start new sequence."));
        
        S_playNextTime[track] = 0;
        S_trackPlaying[track] = true;
        S_repeatCount[track] = 0;
        S_repeatStart[track] = 0; // Not needed.
        // TODO: fix this, somewhere else.
        S_tracksPlaying = MAX_TRACKS;
      }
    }

    S_sequencesToPlay++;

    SEQUENCER_PRINT(F("Sequences left: "));
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
  byte track;

  SEQUENCER_PRINTLN(F("STOP!"));

  for (track = 0; track < MAX_TRACKS; track++)
  {
    S_trackPlaying[track] = false;
    S_nextIn[track] = 0;
    S_nextOut[track] = 0;
    S_ready[track] = 0;
    S_repeatCount[track] = 0;
  }
  S_sequencesToPlay = 0;
  S_tracksPlaying = 0;

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
  return (sequencerBufferAvailable() > (BUFFER_SIZE/2));
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
    if ((BUFFER_SIZE - S_ready[track]) > largestBufferAvailable)
    {
      largestBufferAvailable = (BUFFER_SIZE - S_ready[track]);
    }
  }
  return largestBufferAvailable;
}

/*---------------------------------------------------------------------------*/

static bool sequencerPutByte(byte track, byte value)
{
  bool status;

  if ((track < MAX_TRACKS) && (S_ready[track] < BUFFER_SIZE))
  {
    S_sequence[track][S_nextIn[track]] = value;
    
    S_nextIn[track]++;
    if (S_nextIn[track] >= BUFFER_SIZE)
    {
      S_nextIn[track] = 0;
    }

    S_ready[track]++;

    status = true;
  }
  else
  {
    status = false;
  }

  return status;
}

/*
 * Add something to sequencer buffer.
 * 
 * note = 0-127
 * length 1-256, but saved as 0-255
 */
bool sequencerPutNote(byte track, byte note, unsigned int noteLength)
{
  bool status;

  if ((track < MAX_TRACKS) && (BUFFER_SIZE - S_ready[track] > 2))
  {
    SEQUENCER_PRINT(F("T"));
    SEQUENCER_PRINT(track);
    SEQUENCER_PRINT(F(" "));
    SEQUENCER_PRINT(S_nextIn[track]);
    SEQUENCER_PRINT(F(" put: "));
    SEQUENCER_PRINT(note);
    SEQUENCER_PRINT(F(", "));
    SEQUENCER_PRINT(noteLength);
    SEQUENCER_PRINT(F(" -> "));

    status = sequencerPutByte(track, note);
    if (status == true)
    {
      // To make the timing math work, it uses 256. But we only store it as
      // a byte (0-255). Since 0 is an invalid time, we treat this as base-0
      // and add one.    
      if (noteLength > 255)
      {
        noteLength = 0; // Store as 0 (256).
      }
      // TODO: Error checking to reject anything too large.

      status = sequencerPutByte(track, noteLength);
      if (status == true)
      {
        SEQUENCER_PRINT(F("rdy:"));
        SEQUENCER_PRINT(S_ready[track]);
        SEQUENCER_PRINTLN(F(" "));
      }
    }
  }
  else
  {
    SEQUENCER_PRINT(F("Invalid track put: "));
    SEQUENCER_PRINTLN(track);

    status = false;
  }
  
  return status;
} // end of sequencerPutNote()

/*---------------------------------------------------------------------------*/

static bool sequencerGetByte(byte track, byte *value)
{
  bool status;

fix_this_later: // HAHA! A GOTO IN C!

  if ((track < MAX_TRACKS) && (S_ready[track] != 0))
  {
    *value = S_sequence[track][S_nextOut[track]];

    S_nextOut[track]++;
    if (S_nextOut[track] >= BUFFER_SIZE)
    {
      S_nextOut[track] = 0;
    }
    
    // Was it a command byte?
    if (*value & CMD_BIT)
    {
      byte cmd;
      byte cmdValue;

      cmd = (*value & CMD_MASK);

      // Look for end of repeat.
      if ((cmd==CMD_REPEAT) || (cmd==CMD_END_SEQUENCE))
      {
        // Are we repeating?
        if (S_repeatCount[track] > 0)
        {
          // Decrement repeat count
          S_repeatCount[track]--;
          // Go back to saved start of sequence
          S_nextOut[track] = S_repeatStart[track];
          // TODO: escape
          goto fix_this_later;
        }
        // Not repeating. Drop through.
      }
      
      // Intercept new repeat command.
      if (cmd==CMD_REPEAT)
      {
        cmdValue = (*value & CMD_VALUE_MASK);

        // Save our current position.
        S_repeatStart[track] = S_nextOut[track];
        // And the repeat count.
        S_repeatCount[track] = cmdValue;
        // Remove the CMD.
        S_ready[track]--;
        // TODO: escape
        goto fix_this_later;
      }
    } // CMD_BIT

    // If currently repeating...
    if (S_repeatCount[track] == 0)
    {
      S_ready[track]--;
    }

    status = true;
  } // end of if ((track < MAX_TRACKS) && (S_ready[track] != 0))
  else
  {
    status = false;
  }

  return status;
} // end of sequencerGetByte()

/*---------------------------------------------------------------------------*/

/*
 * Sequencer handler routine. This must be called repeatedly so it can do
 * timing and switch to the next note.
 */
bool sequencerHandler()
{
  byte          value;
  byte          track;
  byte          note;
  unsigned int  noteLength;
  unsigned long timeNow;

#if !defined(SIRSOUNDJR)
  playHandler();
#endif

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
      //if (sequencerGet(track, &note, &noteLength) == false)
      if (sequencerGetByte(track, &value) == true)
      {
        if (value & CMD_BIT) // Is it a command byte?
        {
          byte cmd;
          //byte cmdValue;

          cmd = (value & CMD_MASK);
          //cmdValue = (value & CMD_VALUE_MASK);

          if (cmd == CMD_END_SEQUENCE)
          {
            // If we read END for both, that track is done.
            SEQUENCER_PRINT(F("T"));
            SEQUENCER_PRINT(track);
            SEQUENCER_PRINTLN(F(" Out of data."));

            S_trackPlaying[track] = false;
            S_tracksPlaying--; // One less track is playing.
            if (S_tracksPlaying == 0)
            {
              // No tracks playing. Sequence done.
              SEQUENCER_PRINTLN(F("* End of Sequence."));
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
          } // end of if (value == END_OF_SEQUENCE)
        }
        else // !CMD_BIT - not a command.
        {
          // If here, must have been a note.
          note = value;

          // Get note length.
          if (sequencerGetByte(track, &value) == true)
          {
            noteLength = value;
            if (noteLength == 0)
            {
              noteLength = 256;
            }
            
            SEQUENCER_PRINT(S_playNextTime[track]);
            SEQUENCER_PRINT(F(" T"));
            SEQUENCER_PRINT(track, DEC);
            SEQUENCER_PRINT(F(" "));
            SEQUENCER_PRINT(note);
            SEQUENCER_PRINT(F(", "));
            SEQUENCER_PRINT(noteLength, DEC);
            SEQUENCER_PRINT(F(" ("));
            unsigned long ms = (noteLength*1000L)/60L;
            SEQUENCER_PRINT(ms);
            SEQUENCER_PRINT(F("ms) "));
  
            if (note != REST) // Don't call with invalid note.
            {
  #if defined(SIRSOUNDJR)
              tonePlayNote(note, ms);
  #else
              playNote(track, note);
  #endif
            }
            S_playNextTime[track] = S_playNextTime[track] + ms;
      
            SEQUENCER_PRINT(F("(next: "));
            SEQUENCER_PRINT(S_playNextTime[track]);
            SEQUENCER_PRINTLN(F(") "));
          }
        } // note
      }
    } // end of if ( (long)(millis() > S_playNextTime[track]) )
  } // end of for (track = 0; track < MAX_TRACKS; track++)

  // Handle note play stuff (ADSR, volume decay, etc.).
  //playHandler();

  // true means tracks are playing, false means they are not.
  return (S_tracksPlaying==0 ? false : true);
  
} // end of sequencerHandler()

/*---------------------------------------------------------------------------*/
// End of Sequencer.ino

