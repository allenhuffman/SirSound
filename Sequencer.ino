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

#define SEQUENCER_VERSION "0.8"

#include "Sequencer.h"
#if defined(SIRSOUNDJR)
#include "TonePlayer.h"
#endif

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

/*---------------------------------------------------------------------------*/
// STATIC GLOBALS
/*---------------------------------------------------------------------------*/

static byte           S_tracksPlaying = 0;
static byte           S_sequencesToPlay = 0;

static byte           S_buffer[BUFFER_SIZE];

static SequencerTrackStruct     S_trackBuf[MAX_TRACKS];
static SequencerStruct          S_seq[MAX_TRACKS];
static SequencerSubstringStruct S_substring[MAX_SUBSTRINGS];

// TODO:
//static unsigned int   S_interruptStart[MAX_TRACKS] = {0};
//static unsigned int   S_interruptNextOut[MAX_TRACKS] = {0};

// TODO: Sub-string support
//static unsigned int   S_substringStart[MAX_SUBSTRINGS];

/*---------------------------------------------------------------------------*/
// FUNCTIONS
/*---------------------------------------------------------------------------*/
/*
 * bufferSize - size to use for track sequencer
 * substringSize - size to use for substrings
 */
bool sequencerInit(unsigned int bufferSize, unsigned int substringSize)
{
  byte          track;
  unsigned int  trackBufferSize;
  unsigned int  bufferStart;

  SEQUENCER_PRINT(F("Sequencer Buffer Size: "));
  SEQUENCER_PRINTLN(bufferSize);

  trackBufferSize = (bufferSize / MAX_TRACKS);

  bufferStart = 0;
  for (track=0; track < MAX_TRACKS; track++)
  {
    S_trackBuf[track].start = bufferStart;
    S_trackBuf[track].end = (bufferStart + trackBufferSize - 1);

    SEQUENCER_PRINT(F("T"));
    SEQUENCER_PRINT(track);
    SEQUENCER_PRINT(F(" "));
    SEQUENCER_PRINT(S_trackBuf[track].start);
    SEQUENCER_PRINT(F(" - "));
    SEQUENCER_PRINTLN(S_trackBuf[track].end);

    bufferStart = S_trackBuf[track].end+1;

    S_trackBuf[track].nextIn = S_trackBuf[track].start;
    S_trackBuf[track].nextOut = S_trackBuf[track].start;
    S_trackBuf[track].ready = 0;

    S_seq[track].repeatStart = 0; // TBA
    S_seq[track].repeatCount = 0; // TBA

    S_substring[track].addStart = BUFFER_SIZE;
  }
  // TODO: Make last track extend to use any leftover bytes.
  S_trackBuf[MAX_TRACKS-1].end = (bufferSize - 1);

  // Substrings will start at (S_bufferEnd[MAX-TRACKS-1]) and end at
  // (BUFFER_SIZE - 1). Yes?
  SEQUENCER_PRINT(F("Substring Buffer Size: "));
  SEQUENCER_PRINTLN(BUFFER_SIZE - S_trackBuf[MAX_TRACKS-1].end - 1);
  SEQUENCER_PRINT(F("Substring Start: "));
  SEQUENCER_PRINTLN(S_trackBuf[MAX_TRACKS-1].end+1);
  SEQUENCER_PRINT(F("Substring End  : "));
  SEQUENCER_PRINTLN(BUFFER_SIZE - 1);

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
  
    buffersize = (S_trackBuf[track].end - S_trackBuf[track].start)+1;
    shift = (S_trackBuf[track].start - S_trackBuf[track].nextOut);
    pos = S_trackBuf[track].nextOut;
  
    // Character to move is at pos.
    tomove = S_buffer[pos];
/*
    SEQUENCER_PRINT(F("Track "));
    SEQUENCER_PRINTLN(track);
    Serial.print("size  = "); Serial.println(buffersize);
    Serial.print("pos   = "); Serial.println(pos);
    Serial.print("shift = "); Serial.println(shift);
    Serial.print("in    = "); Serial.println(S_trackBuf[track].nextIn);
    Serial.print("out   = "); Serial.println(S_trackBuf[track].nextOut);

    for (i=S_trackBuf[track].start; i<S_trackBuf[track].end; i++)
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
        S_trackBuf[track].start, S_trackBuf[track].end);
      // Save what is there.
      temp = S_buffer[newpos];
      // Move character.
      S_buffer[newpos] = tomove;
      tomove = temp;
      pos = newpos;
    }
    // Adjust positions.
    S_trackBuf[track].nextIn = sequencerAddShiftWithRollover(S_trackBuf[track].nextIn, shift,
        S_trackBuf[track].start, S_trackBuf[track].end);
    
    S_trackBuf[track].nextOut = sequencerAddShiftWithRollover(S_trackBuf[track].nextOut, shift,
        S_trackBuf[track].start, S_trackBuf[track].end);

    S_seq[track].repeatStart = sequencerAddShiftWithRollover(S_seq[track].repeatStart, shift,
        S_trackBuf[track].start, S_trackBuf[track].end);

/*
    Serial.print("in    = "); Serial.println(S_trackBuf[track].nextIn);
    Serial.print("out   = "); Serial.println(S_trackBuf[track].nextOut);
    for (i=S_trackBuf[track].start; i<S_trackBuf[track].end; i++)
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
        
        S_seq[track].playNextTime = 0;
        S_seq[track].trackStatus = TRACK_PLAYING;
        sequencerShowTrackStatus(track);
        S_seq[track].repeatCount = 0;
        S_seq[track].repeatStart = 0; // Not needed.
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
  byte track;

  SEQUENCER_PRINTLN(F("STOP!"));

  for (track = 0; track < MAX_TRACKS; track++)
  {
    S_seq[track].trackStatus  = TRACK_IDLE;
    S_trackBuf[track].nextIn  = S_trackBuf[track].start;
    S_trackBuf[track].nextOut = S_trackBuf[track].start;
    S_trackBuf[track].ready   = 0;
    S_seq[track].repeatCount  = 0;

    sequencerShowTrackStatus(track);
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
    bufferAvailable = (S_trackBuf[track].end - S_trackBuf[track].start) + 1 - S_trackBuf[track].ready;
  }
  else
  {
    bufferAvailable = 0; // Bad!
  }

  return bufferAvailable;
}

/*---------------------------------------------------------------------------*/

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
    if (value & CMD_BIT) // Is it a command byte?
    {
      byte cmd;
      byte cmdValue;
  
      cmd = (value & CMD_MASK);
      cmdValue = (value & CMD_VALUE_MASK);

      if (value==CMD_ADD_SUBSTRING)
      {
        // If sequencer cmdValue exists, delete, optimize.
        // Remember start location.
        S_substring[track].addStart = S_trackBuf[track].nextIn;
        SEQUENCER_PRINT(F("substring data will start at "));
        SEQUENCER_PRINTLN(S_substring[track].addStart);
      }
      else if (cmd==CMD_DEL_SUBSTRING)
      {
        SEQUENCER_PRINTLN(F("Delete Substring Here"));
      }
      else if (cmd==CMD_END_SEQUENCE)
      {
        if (S_substring[track].addStart < BUFFER_SIZE)
        {
          SEQUENCER_PRINT(F("substring data will end at "));
          SEQUENCER_PRINT(S_trackBuf[track].nextIn);
          SEQUENCER_PRINT(F(" ("));
          SEQUENCER_PRINT(S_trackBuf[track].nextIn - S_substring[track].addStart);
          SEQUENCER_PRINTLN(F(" bytes)"));
          
          //
        }
      }
    }
    
    S_buffer[S_trackBuf[track].nextIn] = value;
    
    S_trackBuf[track].nextIn++;
    if (S_trackBuf[track].nextIn > S_trackBuf[track].end)
    {
      S_trackBuf[track].nextIn = S_trackBuf[track].start;
      SEQUENCER_PRINT(F("T"));
      SEQUENCER_PRINT(track);
      SEQUENCER_PRINT(F(": wrap to "));
      SEQUENCER_PRINTLN(S_trackBuf[track].nextIn);
    }

    S_trackBuf[track].ready++;

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
    SEQUENCER_PRINT(S_trackBuf[track].nextIn);
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
        SEQUENCER_PRINT(S_trackBuf[track].ready);
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

bool sequencerGetByte(byte track, byte *value, bool cmdByteCheck)
{
  bool status;

fix_this_later: // HAHA! A GOTO IN C!

  if ((track < MAX_TRACKS) && (S_trackBuf[track].ready != 0))
  {
    *value = S_buffer[S_trackBuf[track].nextOut];

    SEQUENCER_PRINT(F("T"));
    SEQUENCER_PRINT(track);
    SEQUENCER_PRINT(F(" get("));
    SEQUENCER_PRINT(S_trackBuf[track].nextOut);
    SEQUENCER_PRINT(F(") = "));
    if (cmdByteCheck == true)
    {
      sequencerShowByte(*value);
    }
    else
    {
      SEQUENCER_PRINTLN(*value);
    }
    
    /*
    if (S_playingSubstring[track] == ?)
    {
      return next byte from substring
      NEXT OUT for substring? Yeah, if we have to save
        the normal nextout and restore it, we are still
        using another variable, so might as well use it
        for this.
      if it is end of sequence,
        S_playingSubstring[track] = 255;
    }
    else
    */
    // Normal playing
    S_trackBuf[track].nextOut++;
    if (S_trackBuf[track].nextOut > S_trackBuf[track].end)
    {
      S_trackBuf[track].nextOut = S_trackBuf[track].start;
    }

    // Was it a command byte?
    if ((cmdByteCheck == true) && (*value & CMD_BIT))
    {
      byte cmd;
      byte cmdValue;

      cmd = (*value & CMD_MASK);
      // TODO: Why set this if not everyone needs it below?
      //cmdValue = (*value & CMD_VALUE_MASK);
      
      // Look for end of repeat.
      if ((cmd==CMD_REPEAT) || (cmd==CMD_END_SEQUENCE))
      {
        // Are we repeating?
        if (S_seq[track].repeatCount > 0)
        {
          // Decrement repeat count
          S_seq[track].repeatCount--;
          // Go back to saved start of sequence
          S_trackBuf[track].nextOut = S_seq[track].repeatStart;
          // TODO: escape
          goto fix_this_later;
        }
        // Not repeating. Drop through.

        // if playing a substring, we would stop here.
      }
      
      // Intercept new repeat command.
      if (cmd==CMD_REPEAT)
      {
        cmdValue = (*value & CMD_VALUE_MASK);

        // Save our current position.
        S_seq[track].repeatStart = S_trackBuf[track].nextOut;
        // And the repeat count.
        S_seq[track].repeatCount = cmdValue;
        // Remove the CMD.
        S_trackBuf[track].ready--;
        // TODO: escape
        goto fix_this_later;
      }
      else if (cmd==CMD_PLAY_SUBSTRING)
      {
        cmdValue = (*value & CMD_VALUE_MASK);
        // Fake it, and start returning data from this substring
        // instead of the normal position.
        /*
        if (sequencerGetSubstringStart(cmdValue)==true)
        {
          // Substring exists.
          // S_playingSubstring[track] = cmdValue;
          // TODO: make sure we can't play a substring from within a substring.
          // TODO: Oh, well BASIC allows that, so... how?
        }
        S_trackBuf[track].ready--;
        // TODO: escape
        goto fix_this_later;
        */
      }
      
    } // CMD_BIT

    // If currently repeating...
    if (S_seq[track].repeatCount == 0)
    {
      S_trackBuf[track].ready--;
    }

    status = true;
  } // end of if ((track < MAX_TRACKS) && (S_trackBuf[track].ready != 0))
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
    if ((S_seq[track].trackStatus == TRACK_IDLE) || 
        (S_seq[track].trackStatus == TRACK_COMPLETE))
    {
      continue;
    }

    if (S_seq[track].playNextTime == 0)
    {
      S_seq[track].playNextTime = timeNow;
    }

    //if ( (long)(millis()-S_seq[i].playNextTime >=0 ) )
    if ( (long)(timeNow > S_seq[track].playNextTime) )
    {
      if (sequencerGetByte(track, &value, true) == true)
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

            S_seq[track].trackStatus = TRACK_COMPLETE;
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
                  S_seq[i].playNextTime = timeNow;
                  S_seq[i].trackStatus = TRACK_PLAYING;
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
          if (sequencerGetByte(track, &value, false) == true)
          {
            noteLength = value;
            if (noteLength == 0)
            {
              noteLength = 256;
            }
            
            SEQUENCER_PRINT(F("T"));
            SEQUENCER_PRINT(track);
            SEQUENCER_PRINT(F(" ["));
            SEQUENCER_PRINT(S_seq[track].playNextTime);
            SEQUENCER_PRINT(F("] "));
            SEQUENCER_PRINT(note);
            SEQUENCER_PRINT(F(", "));
            SEQUENCER_PRINT(noteLength);
            SEQUENCER_PRINT(F(" ("));
            //unsigned long ms = (noteLength*1000L)/60L;
            // 1024 works better for the math and prevents drift.
            unsigned long ms = (noteLength*1024L)/60L;
            SEQUENCER_PRINT(ms);
            SEQUENCER_PRINT(F("ms)"));
  
            if (note != NOTE_REST) // Don't call with invalid note.
            {
  #if defined(SIRSOUNDJR)
              tonePlayNote(note, ms);
  #else
              playNote(track, note);
  #endif
            }
            S_seq[track].playNextTime = S_seq[track].playNextTime + ms;
      
            SEQUENCER_PRINT(F(" next:"));
            SEQUENCER_PRINTLN(S_seq[track].playNextTime);
          }
        } // note
      }
    } // end of if ( (long)(millis() > S_seq[track].playNextTime) )
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
  switch (S_seq[track].trackStatus)
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
  Serial.println(S_trackBuf[track].end - S_trackBuf[track].start + 1);

  Serial.print(F("Next In : "));
  Serial.println(S_trackBuf[track].nextIn);

  Serial.print(F("Next Out: "));
  Serial.println(S_trackBuf[track].nextOut);
}

/*---------------------------------------------------------------------------*/
// End of Sequencer.ino

