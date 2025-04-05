# ror (read, output, repeat) diagram

## todo

- ror coffee mug
- ror logo (lion head clipart split into 8x8 pixel chunks on a grid with a 2px margin)

a mix between a railroad diagram and a state diagram. ror diagrams are precise enough so one can manually process input by reading them and following edges and nodes.

ror diagrams are used to unambiguously program a working parser that covers edge cases properly.

written as a flowchart.

## how to read

from the starting point, follow the arrow to the node matching the next character, or * as a last resort if nothing else matches.

if you traverse circled nodes without arrows on the way, output those characters.

once you reach the node connected with an arrow, read the character it contains. if the node is circled, output it as-is, too.

repeat until eof is outputted.

## character node labels for casectl

symbol|character represented
a|uppercase letter
a|lowercase letter
_|e1 (escape 1), aka underscore
|EN (escape N), aka currency sign
EOF|End of file, ends processing when outputted. Must be the last "character" read.
^|Last read character. Cannot be used before at least one character has been read.
*|Literally anything else (default case). Should always be present if all other node labels aren't present.

## Character nodes

Node shape|Edge heads reaching the node|Behavior|
Empty (just the label)|Arrow|Read a character
Circle|Arrow|Read a character and output it as-is
Circle|None|Output a character

## Warning node

Node Shape: warning sign (triangle with exclamation point inside)

Behavior: outputs a warning to stderr when traversed

## Junction node

Node shape: empty circle (no label)

Behavior: none, act as an intersection between multiple edges.

When connected to, a junction always acts a last resort choice. This is useful to reuse large sections of the diagram.

## Start node

Node shape: filled circle

Behavior: indicates the starting point of processing. Usually connected to a junction.

## Comment node

Node shape: curly brace on the left

Behavior: none. Linked to another node with a dotted link.

Can be linked to a comment node, to comment a comment. Looping comment chains are discouraged.

## Basic diagram (outputs everything as-is)

```mermaid
flowchart LR
0@{shape: f-circ} --> 1((\*)) --> 1
```

## CASECTL diagrams

### Lowercase

```mermaid
---
config:
  theme: mc
  look: classic
  layout: elk
---
flowchart TB
0 --- 1(("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"))
1 --> 2["A"] & 5["_"] & 4(("\*")) & 7[""]
2 --- 3(("a"))
3 --- 1
4 --- 1
5 --- 6(("_"))
5 --> 4
6 --> 7
7 --- 9(("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"))
7 --> 8((""))
8 --- 1
9 --- 13["!"]
9 --> 10(("\*"))
9 --> 11[""]
10 --- 9
11 --- 1
11 --> 12((""))
12 --- 9
13 --> 14(("EOF"))
15["Unterminated literal span"] -.-> 13

0@{shape: f-circ}
2@{shape: text}
5@{shape: text}
7@{shape: text}
11@{shape: text}
13@{shape: tri}
15@{shape: comment}
```

### Uppercase

```mermaid
---
config:
  theme: mc
  look: classic
  layout: elk
---
flowchart TB
0 --- 1((&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;))
1 --> 2["a"]
1 --> 3.5((\*))
1 --> 4(())
1 --> 6["_"]
1 --> 7["a"]
2 --> 3((a))
3 --- 1
3.5 --- 1
4 --- 5(())
5 --- 1
6 --- 10(())
6 --- 8((_))
7 --- 10
7 --- 8
8 --- 9((^))
9 --- 1
10 --- 11((^))
11 --> 12((_))
11 --> 13(())
11 --> 14((A))
12 --- 16((&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;))
13 --- 15(())
14 --- 16
15 --- 16
16 --- 20(())
16 --> 17(())
16 --> 19((\*))
17 --- 18(())
18 --- 16
19 --- 16
20 --> 2
20 --> 21((EOF))

0@{shape: f-circ}
2@{shape: text}
6@{shape: text}
7@{shape: text}
```
