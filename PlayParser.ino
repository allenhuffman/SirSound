#define DEBUG_PLAYPARSER
/*---------------------------------------------------------------------------*/
/* 
Sub-Etha Software's PLAY Parser
By Allen C. Huffman
www.subethasoftware.com

This is an implementation of the Microsoft BASIC "PLAY" command, based on the
6809 assembly version in the Tandy/Radio Shack TRS-80 Color Computer's
Extended Color BASIC.

2018-02-20 0.00 allenh - Project began.
2018-02-28 0.00 allenh - Initial framework.
2018-03-02 0.00 allenh - More work on PLAY and its options.
2018-03-03 0.00 allenh - Most things seem to work now.
2018-03-04 0.00 allenh - Supports Flash-strings. Tweaking debug output.
2018-03-05 0.00 allenh - Fixed issue with dotted notes.
2018-03-11 1.00 allenh - Merging standalone player with SirSound player.
2018-03-14 1.00 allenh - Adding support for REPEAT.
2018-03-17 1.00 allenh - Adding SUBSTRING support.

TODO:
* DONE: Data needs to be moved to PROGMEM.
* Pause may be slightly off (or the demo song is just wrong).
* DONE: Need a "reset to defaults" command.
* DONE: Add support for PROGMEM strings.
* Set variable (2-digit name, numeric or string).
* Use variables to support Xvar$; and =var; 

TOFIX:
* Dotted notes do not work like the real PLAY command, so really long notes
  do not work. They will max out at a note length of 255 (60/sec) which is
  4 seconds. Since only one byte is being used for the note length, anything
  longer currently is not supported. But it could be, if it needs to be.

NOTE
----
N (optional) followed by a letter from "A" to "G" or a number from 1 to 12.
When using letters, they can be optionally followed by "#" or "+" to make it
as sharp, or "-" to make it flat. When using numbers, they must be separated
by a ";" (semicolon).

      C  C# D  D# E  F  F# G  G# A  A# B  (sharps)
      1  2  3  4  5  6  7  8  9  10 11 12
      C  D- D  E- E  F  G- G  A- A  B- B  (flats)

Due to how the original PLAY command was coded by Microsoft, it also allows
sharps and flats that would normally not be allowed. For instance, E# is the
same as F, and F- is the same a E. Since notes are numbered 1-12, the code
did not allow C- or B#. This quirk is replicated in this implementation.

OCTAVE
------
"O" followed by a number from 1 to 5. Default is octave 2, which includes
middle C. (Supports modifiers.)

LENGTH
------
"L" followed by a number from 1 to 255, with an optional "." after it to
add an additional 1/2 of the specified length. i.e., L4 is a quarter note,
L4. is like L4+L8 (dotted quarter note). Default is 2. (Supports modifiers.)

L1 - whole note
L2 - 1/2 half node
L3 - dotted quarter note (L4.)
L4 - 1/4 quarter note
L8 - 1/8 eighth note
L16 - 1/16 note
L32 - 1/32 note
L64 - 1/64 note

TEMPO
-----
"T" followed by a number from 1-255. Default is 2. (Supports modifiers.)

VOLUME
------
"V" followed by a number from 1-31. Default is 15. (Supports modifiers.)

PAUSE
-----
"P" followed by a number from 1-255.

SUBSTRINGS
----------
To be documented, since we will need a special method to load them before
we can use them.

Non-Standard Extensions
-----------------------
"Z" to reset back to default settings.

MODIFIERS
---------
Many items that accept numbers can also use a modifier instead. The
modifier will apply to whatever the last value was. Modifiers are:

* "+" increase value by 1.
* "-" decreate value by 1.
* ">" double value.
* "<" halve value.

For instance, if the octave is currently 1 (O1), using "O+" will make it
octave 2. If Tempo was currenlty 2 (T2), using "T>" would make it 4. If a
modifier causes the value to go out of allowed range, the command will fail
the same as if the out-of-range value was used.
*/
/*---------------------------------------------------------------------------*/

#define PLAYPARSER_VERSION "0.00"

/*---------------------------------------------------------------------------*/
// PROTOTYPES
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
// DEFINES
/*---------------------------------------------------------------------------*/
#if defined(DEBUG_PLAYPARSER)
#define PLAYPARSER_PRINT(...)   Serial.print(__VA_ARGS__)
#define PLAYPARSER_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
#define PLAYPARSER_PRINT(...)
#define PLAYPARSER_PRINTLN(...)
#endif

// 9C5B - Table of numerical note values for letter notes.
static const byte S_NoteJumpTable[7] PROGMEM =
{
  /*   A,  B, C, D, E, F, G */
  /**/10, 12, 1, 3, 5, 6, 8 
};

#define STRINGTYPE_RAM   0
#define STRINGTYPE_FLASH 1

#define MAX_SUBSTRINGS      16  // 0-15
#define SUBSTRING_NAME_LEN  2   // xx$

/*---------------------------------------------------------------------------*/
// GLOBALS
/*---------------------------------------------------------------------------*/
// Used for resetting defaults later, too.
#define DEFAULT_OCTAVE 2
#define DEFAULT_VOLUME 15
#define DEFAULT_NOTELN 4
#define DEFAULT_TEMPO  2

static byte S_Octave = DEFAULT_OCTAVE; // Octave (1-5, default 2)
static byte S_Volume = DEFAULT_VOLUME; // Volume (1-31, default 15)
static byte S_NoteLn = DEFAULT_NOTELN; // Note Length (1-255, default 4) - quarter note
static byte S_Tempo  = DEFAULT_TEMPO;  // Tempo (1-255, default 2)

// Substring index.
static char S_substringName[MAX_SUBSTRINGS][SUBSTRING_NAME_LEN];

/*---------------------------------------------------------------------------*/

/*
 * play()
 */
void play(const __FlashStringHelper *playString)
{
  unsigned int address = (unsigned int)playString;

  PLAYPARSER_PRINT(F("play(F(\""));
  PLAYPARSER_PRINT(playString);
  PLAYPARSER_PRINTLN(F("\"))"));

  playWorker(address, STRINGTYPE_FLASH);
}

void play(const char *playString)
{
  unsigned int address = (unsigned int)playString;

  PLAYPARSER_PRINT(F("play(\""));
  PLAYPARSER_PRINT(playString);
  PLAYPARSER_PRINTLN(F("\")"));

  playWorker(address, STRINGTYPE_RAM);
}

void playWorker(unsigned int commandPtr, byte stringType)
{
  char    commandChar;
  bool    done;
  byte    value;
  byte    note;
  byte    dotVal;
  unsigned int    noteDuration; // 0-256
  unsigned int    dotDuration;  // 0-256
  byte    currentTrack;
  char    substringName[SUBSTRING_NAME_LEN];

  if (commandPtr == 0) return;

  done = false;
  value = 0; // force ?FC ERROR
  dotVal = 0; // Start out with no dotted value.
  currentTrack = 0; // Start loading first track.
  do
  {
    #if defined(USE_SEQUENCER)
    // Handle sequencer during parsing of long strings.
    sequencerHandler();
    #endif
    
    // L9A43
    // L9B98
    // * GET NEXT COMMAND - RETURN VALUE IN ACCA
    commandChar = getNextCommand(&commandPtr, stringType);

    switch( commandChar )
    {
      case '\"': // ignore quotes for testing.
        break;
        
      case '\0':
        value = 1; // no error
        done = true;
        break;

      // 9A4A - ;
      // SUB COMMAND TERMINATED
      case ';':
        // IGNORE SEMICOLONS
        PLAYPARSER_PRINT(F(" ;"));
        break;
        
      // 9A4E - '
      // CHECK FOR APOSTROPHE
      case '\'':
        // IGNORE THEM TOO
        PLAYPARSER_PRINT(F(" '"));
        break;

      // 9A52
      // CHECK FOR AN EXECUTABLE SUBSTRING
      case 'X':
        PLAYPARSER_PRINT(F(" X")); // not supported
        // L9C0A
        // L9C1B
        // Get 1-2 characters, up to $;
        value = checkForVariableName(&commandPtr, stringType, substringName);
        if (value != 0)
        {
          // Translate to number 0-X
          if (getSubstring(substringName, &value)==true)
          {
            value = (value & 0x00001111);
            PLAYPARSER_PRINT(F(" substring "));
            PLAYPARSER_PRINTLN(value);
            showSubstrings();
            sequencerPutByte(currentTrack, (CMD_PLAY_SUBSTRING | value));
            value = 1; // No error.
          }
          else
          {
            value = 0; // ?FC ERROR
          }
        }
        break;

      // CHECK FOR OTHER COMMANDS
      
      // 9A5C
      case 'O':
        // // ADJUST OCTAVE?
        PLAYPARSER_PRINT(F(" O"));
        // O - octave (1-5, default 2)
        //    Modifiers
        value = checkModifier(&commandPtr, stringType, S_Octave);
        if (value >=1 && value <= 5)
        {
          PLAYPARSER_PRINT(value);
          S_Octave = value;
        }
        else
        {
          value = 0; // ?FC ERROR
          done = true;
        }
        break;

      // 9a6D
      case 'V':
        //  V - volume (1-31, default 15)
        PLAYPARSER_PRINT(F(" V"));
        //    Mofifiers
        value = checkModifier(&commandPtr, stringType, S_Volume);
        if (value >=1 && value <= 31)
        {
          PLAYPARSER_PRINT(value);
          S_Volume = value;
#if defined(USE_SEQUENCER)
          // Sequencer allows volume 0-15, so divide by 2.
          // TODO: PLAY does not allow volume 0, so we really need to scale
          // this from 1-31 (31) to 1-15 (15).
          value = (value >> 1);
          value = (value & CMD_VALUE_MASK);
          sequencerPutByte(currentTrack, (CMD_VOLUME | value));
#endif
        }
        else
        {
          value = 0; // ?FC ERROR
          done = true;
        }
        break;

      // 9a8b
      case 'L':
        //  L - note length
        PLAYPARSER_PRINT(F(" L"));
        //    Modifiers
        value = checkModifier(&commandPtr, stringType, S_NoteLn);
        if (value > 0 )
        {
          PLAYPARSER_PRINT(value);
          S_NoteLn = value;
        }
        else
        {
          value = 0; // ?FC ERROR
          done = true;
          break;
        }
        //    . - dotted note
        dotVal = 0;
        while(1)
        {
          commandChar = getNextCommand(&commandPtr, stringType);
          // Done if there is no more.
          if (commandChar == '\0')
          {
            value = 1; // no error
            done = true;
            break;
          }
          else if (commandChar == '.')
          {
            PLAYPARSER_PRINT(F("."));
            dotVal++;
          }
          else // Not a dot.
          {
            // Not a dot. Put it back.
            commandPtr--;
            break;
          }
        }
        break;
        
      // L9AB2
      case 'T':
        //  T - tempo (1-255, default 2)
        PLAYPARSER_PRINT(F(" T"));
        //    Modifiers
        value = checkModifier(&commandPtr, stringType, S_Tempo);
        if (value > 0)
        {
          PLAYPARSER_PRINT(value);
          S_Tempo = value;
        }
        else
        {
          value = 0; // ?FC ERROR
          done = true;
        }
        break;
        
      // L9AC3
      case 'P':
        //  P - pause (1-255)
        PLAYPARSER_PRINT(F(" P"));

        commandChar = getNextCommand(&commandPtr, stringType);
        // Done if there is no more.
        if (commandChar == '\0')
        {
          value = 0; // ?FC ERROR
          done = true;
          break;
        }

        // since =var; is not supported, we default to note length
        value = checkForVariableOrNumeric(&commandPtr, stringType, commandChar, S_NoteLn);
        if (value > 0)
        {
          PLAYPARSER_PRINT(value);

          // Create 60hz timing from Tempo and NoteLn (matching CoCo).
          noteDuration = (256/value/S_Tempo);
          
#if defined(USE_SEQUENCER)
          sequencerPutNote(currentTrack, REST, noteDuration);
#else
          // Convert to 60/second
          // tm/60 = ms/1000
          // ms=(tm/60)*1000
          // no floating point needed this way
          // (tm*1000)/60
          delay((noteDuration*1000L)/60L);
#endif
        }
        else
        {
          value = 0; // ?FC ERROR
          done = true;
        }
        break;

      /*-----------------------------------------------------*/
      // Non-standard PLAY extensions:
      case 'Z': // reset
        PLAYPARSER_PRINT(F(" Z [Defaults]"));
        S_Octave = DEFAULT_OCTAVE; // Octave (1-5, default 2)
        S_Volume = DEFAULT_VOLUME; // Volume (1-31, default 15)
        S_NoteLn = DEFAULT_NOTELN; // Note Length (1-255, default 4) - quarter note
        S_Tempo  = DEFAULT_TEMPO;  // Tempo (1-255, default 2)
#if !defined(SIRSOUNDJR)
        setVolumeAll(0);
#endif
        break;

      case ',': // comma, next track
        PLAYPARSER_PRINTLN(F(" + [Next Track]"));
        // Mark end of current track sequence.
        //sequencerPut(currentTrack, CMD_END_SEQUENCE);
        currentTrack++;
        // No error checking, since we don't know the capabilities
        // of the music player.
        break;

#if defined(USE_SEQUENCER)
      case '&':
        PLAYPARSER_PRINTLN(F(" + [OPTIMIZE]"));
        sequencerOptimizeBuffer();
        break;
      case '%':
        PLAYPARSER_PRINTLN(F(" + [BUFFER]"));
        sequencerShowBufferInfo(currentTrack);
        break;    

      case '*': // asterisk - stop!
        PLAYPARSER_PRINTLN(F(" * [Stop]"));
        sequencerStop();
        //return true;
        // continue parsing anything after it.
        // TODO: Maybe silence other channels?
        break;

      case '@': // at sign - repeat (1-15)
        PLAYPARSER_PRINT(F(" @"));

        commandChar = getNextCommand(&commandPtr, stringType);
        // Done if there is no more.
        if (commandChar == '\0')
        {
          value = 0; // ?FC ERROR
          done = true;
          break;
        }

        // since =var; is not supported, we default to note length
        value = checkForVariableOrNumeric(&commandPtr, stringType, commandChar, 1);
        if (value > 0)
        {
          PLAYPARSER_PRINT(value);

          if (value > 0x0f) // 0-15 allowed (4-bits)
          {
            value = 0x0f;
          }
          sequencerPutByte(currentTrack, (CMD_REPEAT | value));
        }
        else
        {
          value = 0; // ?FC ERROR
          done = true;
        }
        break;

      case '^': //  ^ - Interrupt
        PLAYPARSER_PRINT(F(" ^ [Interrupt]"));
        sequencerPutByte(currentTrack, CMD_INTERRUPT);
        break;

      case '+': // + - Add substring
        PLAYPARSER_PRINT(F(" + [Add]"));
        // Get 1-2 characters, up to $;
        value = checkForVariableName(&commandPtr, stringType, substringName);
        if (value != 0)
        {
          // Translate to number 0-X
          if (addSubstring(substringName, &value)==true)
          {
            value = (value & 0x00001111);
            PLAYPARSER_PRINT(F(" substring "));
            PLAYPARSER_PRINTLN(value);
            showSubstrings();
            sequencerPutByte(currentTrack, (CMD_ADD_SUBSTRING | value));
            value = 1; // No error.
          }
          else
          {
            value = 0; // ?FC ERROR
          }
        }
        break;

      case '-': // + - Delete substring
        PLAYPARSER_PRINT(F(" - [Delete]"));
        // * means delete all
        commandChar = getNextCommand(&commandPtr, stringType);
        
        // Done if there is no more.
        if (commandChar == '*')
        {
          clearSubstrings();
          PLAYPARSER_PRINTLN(F(" all substrings"));
          showSubstrings();
          value = 1; // no error
          break;
        }
        else
        {
          // Not a *. Put it back.
          commandPtr--;
        }
        
        // Get 1-2 characters, up to $;
        value = checkForVariableName(&commandPtr, stringType, substringName);
        if (value != 0)
        {
          // Translate to number 0-X
          if (removeSubstring(substringName, &value)==true)
          {
            value = (value & 0x00001111);
            PLAYPARSER_PRINT(F(" substring "));
            PLAYPARSER_PRINTLN(value);
            showSubstrings();
            sequencerPutByte(currentTrack, (CMD_DEL_SUBSTRING | value));
            value = 1; // No error.
          }
          else
          {
            value = 0; // ?FC ERROR
          }
        }
        break;
#endif // USE_SEQUENCER

      /*-----------------------------------------------------*/

      // L9AEB
      case 'N':
        //  N - note (optional)
        PLAYPARSER_PRINT(F(" N"));
        // Get next command character.
        commandChar = getNextCommand(&commandPtr, stringType);
        
        // Done if there is no more.
        if (commandChar == '\0')
        {
          value = 0; // ?FC ERROR
          done = true;
          break;
        }
        // Drop down to looking for note value.
        // no break

      // L9AF2
      default:
        // (A-G, 1-12)
        //    A-G
        note = 0;
        if (commandChar >= 'A' && commandChar <= 'G')
        {
          //PLAYPARSER_PRINT("A-G ");
          // Get numeric note value of letter note. (0-11)
          note = pgm_read_byte_near(&S_NoteJumpTable[commandChar - 'A']);
          // note is now 1-12

          PLAYPARSER_PRINT(F(" "));
          PLAYPARSER_PRINT(commandChar);
          
          // Check for sharp/flat character.
          commandChar = getNextCommand(&commandPtr, stringType);
          
          // Done if there is no more.
          if (commandChar == '\0')
          {
            // Nothing to see after this one is done.
            value = 1; // no error
            done = true;
          }

          //      # - sharp
          //      + - sharp
          if (commandChar == '#' || commandChar == '+') // Sharp
          {
            PLAYPARSER_PRINT(commandChar);
            note++; // Add one to note number (charp)
          }
          else if (commandChar == '-') // Flat
          {
            PLAYPARSER_PRINT(commandChar);
            note--;
          }
          else
          {
            // Not a #, + or -. Put it back.
            commandPtr--;
          }
        }
        else // NOT A-G, check for 1-12
        {
          // L9BBE - Evaluate decimal expression in command string.
          // Jump to cmp '=' thing in modifier!
          note = checkForVariableOrNumeric(&commandPtr, stringType, commandChar, 0);
          if (note == 0)
          {
            value = 0; // ?FC ERROR
            done = true;
            break;
          }
          PLAYPARSER_PRINT(F(" "));
          PLAYPARSER_PRINT(note);
        }
        
        // L9B22 - Process note value.
        note--; // Adjust note number, BASIC uses 1-2, internally 0-11

        // If not was C (1), and was flat (0), this would make it 255.
        if (note > 11)
        {
          value = 0; // ?FC ERROR
          done = true;
          break;
        }

        /*--------------------------------------------------------*/
        // PROCESS NOTE HERE!
        /*--------------------------------------------------------*/
        
        // Convert tempo and length into note duration.
        noteDuration = (256/S_NoteLn/S_Tempo);

        // Add on dotted notes.
        if (dotVal != 0)
        {
          dotDuration = (noteDuration / 2 );

          // Prevent note duration rollover since we currently do not
          // support a duration larger than 255. Would it be better to
          // just max out at 255 for the longest note possible?
          while((dotVal > 0) && (noteDuration < (256-dotDuration)))
          {
            noteDuration = noteDuration + dotDuration;
            dotVal--;
          }
        }
        
#if defined(USE_SEQUENCER)
        // Sequencer is based on 88-key piano keyboard. PLAY command
        // starts at the 27th note on a piano keyboard, so we add
        // that offset.
        sequencerPutNote(currentTrack, 27+note+(12*(S_Octave-1)), noteDuration);
#else
        // Convert from 60/second to ms
        // tm/60 = ms/1000
        // ms=(tm/60)*1000
        // no floating point needed this way
        // (tm*1000)/60
        unsigned long msDuration = ((noteDuration*1000L)/60L);
        
        // TonePlayer is based on 88-key piano keyboard. PLAY command
        // starts at the 27th note on a piano keyboard, so we add
        // that offset.
        tonePlayNote(27+note+(12*(S_Octave-1)), msDuration);
        delay(msDuration);
#endif
        break;
        
    } // end switch( commandChar );

  } while( done == false );

  if (value == 0)
  {
    PLAYPARSER_PRINTLN();
    PLAYPARSER_PRINT(F("?FC ERROR"));
  }
#if defined(USE_SEQUENCER)
  else
  {
    // End of PLAY string. Start playing.
    sequencerStart();
  }
#endif

  PLAYPARSER_PRINTLN();
  //PLAYPARSER_PRINTLN(F("End."));
} // end of play()

/*---------------------------------------------------------------------------*/

/*
 * Get Next Command
 */
// L9B98
char getNextCommand(unsigned int *ptr, byte stringType)
{
  char commandChar;

  //PLAYPARSER_PRINT("getNextCommand(");
  //PLAYPARSER_PRINT((unsigned int)*ptr);
  //PLAYPARSER_PRINT(") - ");
  
  if (ptr == 0)
  {
    // Return nil character, and leave pointer alone.
    commandChar = '\0'; // NIL character
  }
  else
  {
    // Return next character, skipping spaces.
    while(1)
    {
      // Get character at current position.

      if (stringType == STRINGTYPE_RAM)
      {
        char *p = (char*)*ptr;
        commandChar = *p;
      }
      else
      {
        commandChar = pgm_read_byte_near(*ptr);
      }

      if ( commandChar == '\0') break;

      // Increment pointer.
      (*ptr)++;

      if ( commandChar != ' ') break;
    }
  }

  //PLAYPARSER_PRINT(" ... ");
  //PLAYPARSER_PRINTLN((unsigned int)*ptr);

  return commandChar;
}

/*---------------------------------------------------------------------------*/

/*
 * Check Modifiers
 */
/*
  // + - add one
  // - - substract one
  // > - multiply to two
  // < - divited by two
  // = - variable value (not suppoted; just skip)
  // number - use that value (skip if >255)
*/

// L9AC0 - JMP L9BAC
byte checkModifier(unsigned int *ptr, byte stringType, byte value)
{
  char      commandChar;
  
  if (ptr != 0)
  {
    commandChar = getNextCommand(ptr, stringType);

    switch( commandChar )
    {
      case '\0':
        break;
        
      // ADD ONE?
      case '+':
        if (value < 255)
        { 
          value++;
        }
        else
        {
          value = 0; // ?FC ERROR
        }
        break;
        
      // SUBTRACT ONE?
      case '-':
        if (value > 0)
        {
          value--;
        }
        else
        {
          value = 0; // ?FC ERROR
        }
        break;
        
      // MULTIPLY BY TWO?
      case '>':
        if (value <= 127)
        {
          value = value * 2;
        }
        else
        {
          value = 0; // ?FC ERROR
        }
        break;

      // DIVIDE BY TWO?
      case '<':
        if (value > 1)
        {
          value = value / 2;
        }
        else
        {
          value = 0; // ?FC ERROR
        }
        break;

      // Could be = or number, so we call a separate function since we
      // need this in the note routine as well.
      default:
        value = checkForVariableOrNumeric(ptr, stringType, commandChar, value);
        break;
        
    } // end of switch( commandChar )

  } // end of NULL check

  return value;  
} // end of checkModifiers()

/*---------------------------------------------------------------------------*/

byte checkForVariableOrNumeric(unsigned int *ptr, byte stringType, char commandChar, byte value)
{
  //byte      value = 0;
  unsigned int  temp; // MUL A*B = D

  switch( commandChar )
  {
    // SirSound will not have a way to support this.
  
    // CHECK FOR VARIABLE EQUATE
    case '=':
      // "=XX;" - value = whatever XX is set to.
      PLAYPARSER_PRINT(F("(")); // not supported
      PLAYPARSER_PRINT(commandChar);
      // Skip until semicolon or end of string.
      do
      {
        commandChar = getNextCommand(ptr, stringType);
        
        if (commandChar == '\0') break;

        PLAYPARSER_PRINT(commandChar);
        
      } while( commandChar != ';' );
      PLAYPARSER_PRINT(F(")")); // not supported
      // Leave value unchanged, since we don't support it.
      
      //value = 0; // ?FC ERROR since we do not support this yet.
      break;
  
    // Else, check for numeric string.
    default:
      temp = 0;
      value = 0;
      do
      {
        // Stop at a non-numeric character.
        if ( !isdigit(commandChar) ) // not digit
        {
          // Rewind so we end up where we started.
          (*ptr)--;
          break;
        }

        // If here, it must be a digit, 0-9
  
        // Base 10. First time it will be 0 * 10.
        temp = temp * 10;
        
        // Convert ASCII number to value.
        temp = temp + (commandChar - '0');

        if (temp > 255)
        {
/*
          // Skip past numbers.
          do
          {
            commandChar = getNextCommand(ptr);
            if (commandChar == '\0') break;
          } while(isdigit(commandChar) != 0); // digit
  
          // Rewind
          (*ptr)--;
          temp = 0;
*/
          temp = 0; // ?FC ERROR
          break;
        }
        
        // Get another command byte.
        commandChar = getNextCommand(ptr, stringType);
        
      } while( commandChar != '\0' );

      value = temp;
      break;
  } // end of switch( commandChar )

  return value;
}

/*---------------------------------------------------------------------------*/
// INTERPRET THE PRESENT COMMAND STRING AS IF IT WERE A BASIC VARIABLE
// L9C1B
byte checkForVariableName(unsigned int *ptr, byte stringType, char *variableNamePtr)
{
  byte value;
  byte commandChar;

  value = 0; // ?FC ERROR
  if ((ptr != 0) && (variableNamePtr != NULL))
  {
    commandChar = getNextCommand(ptr, stringType);

    if ((commandChar >= 'A') && (commandChar <= 'Z'))
    {
      variableNamePtr[0] = commandChar;
      PLAYPARSER_PRINT((char)commandChar);
      
      commandChar = getNextCommand(ptr, stringType);

      if ( ((commandChar >= 'A') && (commandChar <= 'Z')) ||
           ((commandChar >= '0') && (commandChar <= '9')) )
      {
        variableNamePtr[1] = commandChar;
        PLAYPARSER_PRINT((char)commandChar);

        // Here we should look for $, then ;, with only alpha/numeric.
        // TODO: Only when doing the X do we care about the ;
        // +XX$;CDEFGAB --- does that make sense?
        do
        {
          commandChar = getNextCommand(ptr, stringType);
          if (commandChar == ';')
          {
            value = 1;
            break;
          }
        } while(commandChar != '\0');
      } // end of A-Z, 0-9
    } // end of A-Z
  }

  return value;
}

/*---------------------------------------------------------------------------*/
// SUBSTRINGS
/*---------------------------------------------------------------------------*/

static void clearSubstrings()
{
  byte index;

  for (index=0; index < MAX_SUBSTRINGS; index++)
  {
    S_substringName[index][0] = '\0';
  }
}

static bool addSubstring(char *name, byte *number)
{
  bool status;
  byte index;

  if ((name==NULL) || (number==NULL)) return false;

  if (getSubstring(name, number)==true)
  {
    // Already exists.
    status = true;
  }
  else // Does not exist. Find empty slot and add.
  {
    // Find empty slot.
    for (index=0; index < MAX_SUBSTRINGS; index++)
    {
      if (S_substringName[index][0] == '\0')
      {
        break;
      }
    }

    if (index < MAX_SUBSTRINGS)
    {
      strncpy(S_substringName[index], name, SUBSTRING_NAME_LEN);
      *number = index;
      status = true;
    }
    else
    {
      status = false;
    }
  }

  return status;
}

static bool getSubstring(char *name, byte *number)
{
  bool status;
  byte index;

  if ((name==NULL) || (number==NULL)) return false;

  status = false;
  for (index=0; index < MAX_SUBSTRINGS; index++)
  {
    if (strncmp(name, S_substringName[index], SUBSTRING_NAME_LEN)==0)
    {
      *number = index;
      status = true;
      break;
    }
  }

  return status;
}

static bool removeSubstring(char *name, byte *number)
{
  bool status;
  
  if ((name==NULL) || (number==NULL)) return false;

  if (getSubstring(name, number)==true)
  {
    S_substringName[*number][0] = '\0';
    status = true;
  }
  else
  {
    status = false;
  }

  return status;
}

void showSubstrings()
{
  byte index;
  byte charPos;

  Serial.println(F("Substrings:"));
  for (index=0; index < MAX_SUBSTRINGS; index++)
  {
    if (S_substringName[index][0] != '\0')
    {
      Serial.print(index);
      Serial.print(F(". "));
      for (charPos=0; charPos < SUBSTRING_NAME_LEN; charPos++)
      {
        Serial.print(S_substringName[index][charPos]);
      }
      Serial.println();
    }
  }
}

/*---------------------------------------------------------------------------*/
// End of PlayParser

