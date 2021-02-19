# regex.c

A small program to find regular expressions within a file. It will convert a regular expression into an e-NFA, which it will use to search the text.

The first argument of the program is the expression itself in \" \" quotes in windows, dont forget the escape character. The second one is the file location. The third is optional - window wisth (100 by default).

Regular expressions are using the following syntax:
'*' - kleene star
'+' - OR
'()' - brackets

