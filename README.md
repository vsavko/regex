# regex
Learning automata theory.

A small program to find regular expressions within a file. It will convert a regular expression into an e-NFA, and then to DFA, which it will use to search the text.

The first argument of the program is the expression itself, the second one is the file location.

Regular expressions are using the following syntax:
'*' - kleene star
'+' - OR
'()' - brackets
'\e' - for empty string
'\n' - for null string.

The program is not finished yet.


