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

OCTAVE
------
"O" followed by a number from 1 to 5. Default is octave 2, which includes
middle C. (Supports modifiers.)

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

TEMPO
-----
"T" followed by a number from 1-255. Default is 2. (Supports modifiers.)

VOLUME
------
"V" followed by a number from 1-31. Default is 15. (Supports modifiers.)
(Does nothing on the Arduino.)

PAUSE
-----
"P" followed by a number from 1-255.

SUBSTRINGS
----------
NOT IMPLEMENTED YET. To be documented, since we will need a special method to load them befoe we can use them.
 
Non-Standard Extensions
-----------------------
"Z" to reset back to default settings:
* Octave (1-5, default 2)
* Volume (1-31, default 15)
* Note Length (1-255, default 4) - quarter note
* Tempo (1-255, default 2)

"*" to stop the sequencer and abort playback.

"@xxx" repeat the sequence 'xxx' times from this marker up to the next repeat marker or end of sequence.

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

