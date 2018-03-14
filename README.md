# SirSound Mockumentation
Work-in-progress development code.

REVISION
========
* 2018-02-20 allenh - Project started.
* 2018-03-14 allenh - Documentation started. Mmmm, Pi.

FILES
=====
* README.md - this file
* xxx

CONFIGURATION
=============
TBA

RUNNING
=======
SirSound always powers up defaulting to 1200 baud. The computer must be set to match the baud rate:
Color Computer Printer Baud
---------------------------
POKE 150,X - where 'x' is one of the following:
* 180 = 300 baud
* 87 = 600
* 41 = 1200 
* 18 = 2400 
* 7 = 4800 
* 1 = 9600
```
POKE 150,41
```

MC-10 Printer Baud
------------------
POKE 16932,X - where "X" is one of the following:
* 241 = 300 
* 118 = 600 
* 57 = 1200 
* 26 = 2400 
* 10 = 4800 
* 9 = 9600
```
POKE 16932,57
```

NOTE: SirSound supports up to three play strings to be played at the same time. SirSound Jr supports only one.
```
PRINT #-2,"voice 1 play string{,voice 2 play string,voice 3 play string}"
```
To play a single-voice tune:
```
PRINT #-2,"CEGFEFDE"
```
To play a chord of C, E and G:
```
PRINT #-2,"C,E,G"
```

Standard PLAY commands:

NOTE
----
N (optional) followed by a letter from "A" to "G" or a number from 1 to 12.
When using letters, they can be optionally followed by "#" or "+" to make it
as sharp, or "-" to make it flat. When using numbers, they must be separated
by a ";" (semicolon).
```
C  C# D  D# E  F  F# G  G# A  A# B  (sharps)
1  2  3  4  5  6  7  8  9  10 11 12
C  D- D  E- E  F  G- G  A- A  B- B  (flats)
```
Due to how the original PLAY command was coded by Microsoft, it also allows
sharps and flats that would normally not be allowed. For instance, E# is the
same as F, and F- is the same a E. Since notes are numbered 1-12, the code
did not allow C- or B#. This quirk is replicated in this implementation.
```
PRINT #-2,"CDEFGAB"
````

OCTAVE
------
"O" followed by a number from 1 to 5. Default is octave 2, which includes
middle C. (Supports modifiers.)
```
PRINT #-2,"O1 C O2 C O3 C O4 C O5 C"
```

LENGTH
------
"L" followed by a number from 1 to 255, with an optional "." after it to
add an additional 1/2 of the specified length. i.e., L4 is a quarter note,
L4. is like L4+L8 (dotted quarter note). Default is 2. (Supports modifiers.)
* L1 - whole note
* L2 - 1/2 half node
* L3 - dotted quarter note (L4.)
* L4 - 1/4 quarter note
* L8 - 1/8 eighth note
* L16 - 1/16 note
* L32 - 1/32 note
* L64 - 1/64 note
```
PRINT #-2,"L2 C D E L8 F D L4 E D C"
```

TEMPO
-----
"T" followed by a number from 1-255. Default is 2. (Supports modifiers.)
```
PRINT #-2,"T2 C D E T4 C D E T 8 C D E"
```

VOLUME
------
"V" followed by a number from 1-31. Default is 15. (Supports modifiers.)
(Does nothing on the Arduino.)
```
PRINT #-2,"V31"
```

PAUSE
-----
"P" followed by a number from 1-255.
```
PRINT #-2,"P4"
```

SUBSTRINGS
----------
"X" followed by a 1-2 character string name (previously loaded using "+") and the dollar sign.

 
Non-Standard Extensions
-----------------------
"Z" to reset back to default settings:
* Octave (1-5, default 2)
* Volume (1-31, default 15)
* Note Length (1-255, default 4) - quarter note
* Tempo (1-255, default 2)

"*" to stop the sequencer and discard any queued playback.

"TBD" to pause the specified track.
```
REM PAUSE VOICE 1
PRINT #-2,"TBA"
REM PAUSE VOICE 3
PRINT #-2,",,TBA"
```

"TBD" to resume the specified track.
```
REM RESUME VOICE 1
PRINT #-2,"TBA"
REM RESUME VOICE 3
PRINT #-2,",,TBA"
```

"@xxx" repeat the sequence 'xxx' times from this marker up to the next repeat marker or end of sequence.
```
REM REPEAT FIVE TIMES
PRINT #-2,"@5 O1 L8 C C E E G G A A"
REM REPEAT FOREVER
PRINT #-2,"@0 O1 L2 C P2 C+ P2"
```

"+xx=yyyy" add named substring (up to two characters, with the first being A-Z and the second being A-Z or 0-9). The substring will be available for use with the "X" command.

"-xx" delete named substring.

"#xxxx" change the baud rate of SirSound to xxxx. SirSound always powers up using 1200 baud.
````
REM SET COCO TO 1200 BAUD
POKE 150,41
REM SET SIRSOUND TO 9600 BAUD
PRINT #-2,"#9600"
REM SET COCO TO 9600 BAUD
POKE 150,1
```

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

Sample Music
============
Here is "Oh When the Saints Go Marching In" from the Extended Color BASIC manual:

```
  play("T5;C;E;F;L1;G;P4;L4;C;E;F;L1;G");
  play("P4;L4;C;E;F;L2;G;E;C;E;L1;D");
  play("P8;L4;E;E;D;L2.;C;L4;C;L2;E");
  play("L4;G;G;G;L1;F;L4;E;F");
  play("L2;G;E;L4;C;L8;D;D+;D;E;G;L4;A;L1;O3;C");
```

TODO
====
* Set variable (2-digit name, numeric or string).
* Use variables to support Xvar$; and =var; 

