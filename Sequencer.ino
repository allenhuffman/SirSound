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
2018-03-16 0.7 allenh - Adding VOLUME command for sound chip.
2018-03-17 0.8 allenh - Rewrite buffer system to support substrings.

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

#define SEQUENCER_VERSION "0.7"

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
//#define BUFFER_SIZE 900
#define BUFFER_SIZE 100

#define MAX_SUBSTRINGS      16  // 0-15

/*---------------------------------------------------------------------------*/
// STATIC GLOBALS
/*---------------------------------------------------------------------------*/

static unsigned long  S_playNextTime[MAX_TRACKS];
static byte           S_trackStatus[MAX_TRACKS]; // playing or not?
static byte           S_tracksPlaying = 0;
static byte           S_sequencesToPlay = 0;

static byte           S_buffer[BUFFER_SIZE];
static unsigned int   S_bufferStart[MAX_TRACKS];
static unsigned int   S_bufferEnd[MAX_TRACKS];

static unsigned int   S_nextIn[MAX_TRACKS];
static unsigned int   S_nextOut[MAX_TRACKS];
static unsigned int   S_ready[MAX_TRACKS];
static unsigned int   S_repeatStart[MAX_TRACKS];
static byte           S_repeatCount[MAX_TRACKS];

// TODO:
//static unsigned int   S_interruptStart[MAX_TRACKS] = {0};
//static unsigned int   S_interruptNextOut[MAX_TRACKS] = {0};

// TODO: Sub-string support
//static unsigned int   S_substringStart[MAX_SUBSTRINGS];

/*---------------------------------------------------------------------------*/
// FUNCTIONS
/*---------------------------------------------------------------------------*/

bool sequencerInit()
{
  byte          track;
  unsigned int  trackBufferSize;
  unsigned int  bufferStart;

  SEQUENCER_PRINT(F("Sequencer Buffer Size: "));
  SEQUENCER_PRINTLN(BUFFER_SIZE);

  trackBufferSize = (BUFFER_SIZE / MAX_TRACKS);

  bufferStart = 0;
  for (track=0; track < MAX_TRACKS; track++)
  {
    S_bufferStart[track] = bufferStart;
    S_bufferEnd[track] = (bufferStart + trackBufferSize - 1);

    SEQUENCER_PRINT(F("T"));
    SEQUENCER_PRINT(track);
    SEQUENCER_PRINT(F(" "));
    SEQUENCER_PRINT(S_bufferStart[track]);
    SEQUENCER_PRINT(F(" - "));
    SEQUENCER_PRINTLN(S_bufferEnd[track]);

    bufferStart = S_bufferEnd[track]+1;

    S_nextIn[track] = S_bufferStart[track];
    S_nextOut[track] = S_bufferStart[track];
    S_ready[track] = 0;
    S_repeatStart[track] = 0; // TBA
    S_repeatCount[track] = 0; // TBA    
  }
  // TODO: Make last track extend to use any leftover bytes.
  S_bufferEnd[MAX_TRACKS-1] = (BUFFER_SIZE - 1);

  return true;
} // end of seqencerInit()

/*---------------------------------------------------------------------------*/

/*
 * To make room for substrings, the track buffers will need to have their
 * sizes reduced. To allow this, we need a routine to compress the used
 * data down and ajust all the pointers to it.
 */

bool sequencerOptimizeBuffer()
{
  byte          track;

  SEQUENCER_PRINTLN(F("Optimizing Buffer."));

  for (track=0; track < MAX_TRACKS; track++)
  {
    unsigned int  i;
    unsigned int  buffersize;
    int           shift;
    unsigned int  pos;
    int           newpos;
    byte          temp, tomove;
  
    buffersize = (S_bufferEnd[track] - S_bufferStart[track])+1;
    shift = (S_bufferStart[track] - S_nextOut[track]);
    pos = S_nextOut[track];
  
    // Character to move is at pos.
    tomove = S_buffer[pos];
/*
    SEQUENCER_PRINT(F("Track "));
    SEQUENCER_PRINTLN(track);
    Serial.print("size  = "); Serial.println(buffersize);
    Serial.print("pos   = "); Serial.println(pos);
    Serial.print("shift = "); Serial.println(shift);
    Serial.print("in    = "); Serial.println(S_nextIn[track]);
    Serial.print("out   = "); Serial.println(S_nextOut[track]);

    for (i=S_bufferStart[track]; i<S_bufferEnd[track]; i++)
    {
      SEQUENCER_PRINT(S_buffer[i], DEC);
      SEQUENCER_PRINT(F(" "));
    }
    SEQUENCER_PRINTLN();
*/

    for (i=0; i < buffersize; i++)
    {
      // Location to move it to is newpos.
      newpos = sequencerAddShiftWithRollover(pos, shift,
        S_bufferStart[track], S_bufferEnd[track]);
      // Save what is there.
      temp = S_buffer[newpos];
      // Move character.
      S_buffer[newpos] = tomove;
      tomove = temp;
      pos = newpos;
    }
    // Adjust positions.
    S_nextIn[track] = sequencerAddShiftWithRollover(S_nextIn[track], shift,
        S_bufferStart[track], S_bufferEnd[track]);
    
    S_nextOut[track] = sequencerAddShiftWithRollover(S_nextOut[track], shift,
        S_bufferStart[track], S_bufferEnd[track]);

    S_repeatStart[track] = sequencerAddShiftWithRollover(S_repeatStart[track], shift,
        S_bufferStart[track], S_bufferEnd[track]);
/*
    Serial.print("in    = "); Serial.println(S_nextIn[track]);
    Serial.print("out   = "); Serial.println(S_nextOut[track]);
    for (i=S_bufferStart[track]; i<S_bufferEnd[track]; i++)
    {
      SEQUENCER_PRINT(S_buffer[i], DEC);
      SEQUENCER_PRINT(F(" "));
    }
    SEQUENCER_PRINTLN();
*/
  } // end of for()

  return true;
} // end of seqencerInit()

unsigned int sequencerAddShiftWithRollover(unsigned int pos, int shift,
  unsigned int start, unsigned int end)
{
  unsigned int  bufferSize;
  int           newPos;

  bufferSize = (end - start)+1;
  
  newPos = (pos + shift);

  // Handle rollover.
  if (newPos < (int)start)
  {
    newPos = (newPos + bufferSize);
  }
  else if (newPos > (int)end)
  {
    newPos = (newPos - bufferSize);
  }

  return newPos;
}

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
        S_trackStatus[track] = TRACK_PLAYING;
        sequencerShowTrackStatus(track);
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
    S_trackStatus[track] = TRACK_IDLE;
    sequencerShowTrackStatus(track);
    S_nextIn[track] = S_bufferStart[track];
    S_nextOut[track] = S_bufferStart[track];
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
  return (sequencerBufferAvailable() > (BUFFER_SIZE/MAX_TRACKS/2));
}

/*---------------------------------------------------------------------------*/

unsigned int sequencerTrackBufferAvailable(byte track)
{
  unsigned int bufferAvailable;
  
  if (track < MAX_TRACKS)
  {
    bufferAvailable = (S_bufferEnd[track] - S_bufferStart[track]) + 1 - S_ready[track];
  }
  else
  {
    bufferAvailable = 0; // Bad!
  }

  return bufferAvailable;
}

/*
 * Return the largest amount of buffer available. Eventually, we'll use one
 * global buffer and this will be better.
 */
unsigned int sequencerBufferAvailable()
{
  unsigned int track;
  unsigned int largestBufferAvailable;
  unsigned int bufferAvailable;

  sequencerHandler();

  largestBufferAvailable = 0;
  for (track = 0; track < MAX_TRACKS; track++)
  {
    bufferAvailable = sequencerTrackBufferAvailable(track);
    if (bufferAvailable > largestBufferAvailable)
    {
      largestBufferAvailable = bufferAvailable;
    }
  }

  return largestBufferAvailable;
}

/*---------------------------------------------------------------------------*/
// BUFFER FUNCTIONS
/*---------------------------------------------------------------------------*/

bool sequencerPutByte(byte track, byte value)
{
  bool status;

  if ((track < MAX_TRACKS) && (sequencerTrackBufferAvailable(track) > 1))
  {
    S_buffer[S_nextIn[track]] = value;
    
    S_nextIn[track]++;
    if (S_nextIn[track] > S_bufferEnd[track])
    {
      S_nextIn[track] = S_bufferStart[track];
      SEQUENCER_PRINT(F("T"));
      SEQUENCER_PRINT(track);
      SEQUENCER_PRINT(F(": wrap to "));
      SEQUENCER_PRINTLN(S_nextIn[track]);
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

/*---------------------------------------------------------------------------*/

/*
 * Add something to sequencer buffer.
 * 
 * note = 0-127
 * length 1-256, but saved as 0-255
 */
bool sequencerPutNote(byte track, byte note, unsigned int noteLength)
{
  bool status;

  if ((track < MAX_TRACKS) && (sequencerTrackBufferAvailable(track) > 2))
  {
    SEQUENCER_PRINT(F("T"));
    SEQUENCER_PRINT(track);
    SEQUENCER_PRINT(F(" put("));
    SEQUENCER_PRINT(S_nextIn[track]);
    SEQUENCER_PRINT(F(") - "));

    status = sequencerPutByte(track, note);
    if (status == true)
    {
      SEQUENCER_PRINT(note);
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
        SEQUENCER_PRINT(F(", "));
        SEQUENCER_PRINT(noteLength);
        SEQUENCER_PRINT(F(" -> "));

        SEQUENCER_PRINT(F("rdy:"));
        SEQUENCER_PRINT(S_ready[track]);
        SEQUENCER_PRINTLN(F(" "));
      }
    }
  }
  else
  {
    SEQUENCER_PRINT(F("T"));
    SEQUENCER_PRINT(track);
    SEQUENCER_PRINTLN(F(": Can't put."));

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
    *value = S_buffer[S_nextOut[track]];

    SEQUENCER_PRINT(F("T"));
    SEQUENCER_PRINT(track);
    SEQUENCER_PRINT(F(" get("));
    SEQUENCER_PRINT(S_nextOut[track]);
    SEQUENCER_PRINT(F(") = "));
    SEQUENCER_PRINTLN(*value);
    //sequencerShowByte(*value);

    S_nextOut[track]++;
    if (S_nextOut[track] > S_bufferEnd[track])
    {
      S_nextOut[track] = S_bufferStart[track];
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
// SEQUENCER FUNCTIONS
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
    if ((S_trackStatus[track] == TRACK_IDLE) || 
        (S_trackStatus[track] == TRACK_COMPLETE))
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
      if (sequencerGetByte(track, &value) == true)
      {
        if (value & CMD_BIT) // Is it a command byte?
        {
          byte cmd;
          byte cmdValue;

          cmd = (value & CMD_MASK);
          cmdValue = (value & CMD_VALUE_MASK);

          if (cmd == CMD_END_SEQUENCE)
          {
            // If we read END for both, that track is done.
            SEQUENCER_PRINT(F("T"));
            SEQUENCER_PRINT(track);
            SEQUENCER_PRINTLN(F(" Out of data."));

            S_trackStatus[track] = TRACK_COMPLETE;
            sequencerShowTrackStatus(track);

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
                  S_trackStatus[i] = TRACK_PLAYING;
                  sequencerShowTrackStatus(track);
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
#if !defined(SIRSOUNDJR) // No volume on Arduino tone()      
          else if (cmd==CMD_VOLUME)
          {
            SEQUENCER_PRINT(F("setVolume: "));
            SEQUENCER_PRINT(track);
            SEQUENCER_PRINT(F(", "));
            SEQUENCER_PRINTLN(15-cmdValue);
            // Volume is 0-15, but sound chip is 15 (off) to 0 (max).
            setVolume(track, (15-cmdValue) );
          }
#endif
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
            
            SEQUENCER_PRINT(F("T"));
            SEQUENCER_PRINT(track);
            SEQUENCER_PRINT(F(" "));
            SEQUENCER_PRINT(S_playNextTime[track]);
            SEQUENCER_PRINT(F(": "));
            SEQUENCER_PRINT(note);
            SEQUENCER_PRINT(F(", "));
            SEQUENCER_PRINT(noteLength);
            SEQUENCER_PRINT(F(" ("));
            //unsigned long ms = (noteLength*1000L)/60L;
            // 1024 works better for the math and prevents drift.
            unsigned long ms = (noteLength*1024L)/60L;
            SEQUENCER_PRINT(ms);
            SEQUENCER_PRINT(F("ms)"));
  
            if (note != REST) // Don't call with invalid note.
            {
  #if defined(SIRSOUNDJR)
              tonePlayNote(note, ms);
  #else
              playNote(track, note);
  #endif
            }
            S_playNextTime[track] = S_playNextTime[track] + ms;
      
            SEQUENCER_PRINT(F(" next:"));
            SEQUENCER_PRINTLN(S_playNextTime[track]);
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
// DEBUG FUNCTIONS
/*---------------------------------------------------------------------------*/

static void sequencerShowByte(byte value)
{
  SEQUENCER_PRINT(value);
  SEQUENCER_PRINT(F(" "));
  
  if (value & CMD_BIT)
  {
    switch (value & CMD_MASK)
    {
      case CMD_VOLUME:
        SEQUENCER_PRINT(F("CMD_VOLUME | "));
        break;
      case CMD_REPEAT:
        SEQUENCER_PRINT(F("CMD_REPEAT | "));
        break;
      case CMD_INTERRUPT:
        SEQUENCER_PRINT(F("CMD_INTERRUPT | "));
        break;
      case CMD_ADD_SUBSTRING:
        SEQUENCER_PRINT(F("CMD_ADD_SUBSTRING | "));
        break;
      case CMD_DEL_SUBSTRING:
        SEQUENCER_PRINT(F("CMD_DEL_SUBSTRING | "));
        break;
      case CMD_PLAY_SUBSTRING:
        SEQUENCER_PRINT(F("CMD_PLAY_SUBSTRING | "));
        break;
      case CMD_6:
        SEQUENCER_PRINT(F("CMD_6 | "));
        break;
      case CMD_END_SEQUENCE:
        SEQUENCER_PRINT(F("CMD_END_SEQUENCE | "));
        break;
      default:
        SEQUENCER_PRINT(F("CMD_unknown"));
        break;
    }
    SEQUENCER_PRINT(value & CMD_VALUE_MASK);    
  }
  
  SEQUENCER_PRINTLN();
}

/*---------------------------------------------------------------------------*/

static void sequencerShowTrackStatus(byte track)
{
  SEQUENCER_PRINT(F("T"));
  SEQUENCER_PRINT(track);
  SEQUENCER_PRINT(F(" "));
  switch (S_trackStatus[track])
  {
    case TRACK_IDLE:
      SEQUENCER_PRINTLN(F("TRACK_IDLE"));
      break;

    case TRACK_PLAYING:
      SEQUENCER_PRINTLN(F("TRACK_PLAYING"));
      break;    

    case TRACK_COMPLETE:
      SEQUENCER_PRINTLN(F("TRACK_COMPLETE"));
      break;    

    case TRACK_INTERRUPTED:
      SEQUENCER_PRINTLN(F("TRACK_INTERRUPTED"));
      break;    

    default:
      SEQUENCER_PRINTLN(F("TRACK_???"));
      break;    
  }
}

/*---------------------------------------------------------------------------*/
void sequencerShowBufferInfo(byte track)
{
  Serial.print(F("T"));
  Serial.print(track);
  Serial.println(F(": Info"));

  Serial.print(F("Size    : "));
  Serial.println(S_bufferEnd[track] - S_bufferStart[track] + 1);

  Serial.print(F("Next In : "));
  Serial.println(S_nextIn[track]);

  Serial.print(F("Next Out: "));
  Serial.println(S_nextOut[track]);
}

/*---------------------------------------------------------------------------*/
// End of Sequencer.ino

