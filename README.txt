
Bugs:
1. Rectangle indicating screenshot area sometimes glitches

Elias Nijs								    2023


                                   screenshot


Small c program for taking a screenshot on X11


Dependencies:
1. libx11-dev     Interacting with X11
2. xclip          Saving images to the clipboard

Build:
gcc -pipe -O3 -o screenshot screenshot.c -lX11

Usage:
1. Save the screenshot to the clipboard

   ./screenshot

2. Save the screenshot to a file

   ./screenshot {filename}

3. Save the screenshot to a file with no filename specified.

   ./screenshot -t

   This will, for example, generate the following file:
   screenshot-2023-05-18-16:30:58.png

