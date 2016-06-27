# Rhonda
Rhondha is just another personal home assistant working with speech recognition (a Javis like). Developped for Raspberry. All the configuration will be done with one xml file https://github.com/Smanar/Rhonda/blob/master/rhonda/config.xml and you can use specials shells scripts for personals actions.

So firstly, I know the code is ugly, but this project was written in shell then in C and finaly in C++, with lot of modifications (unicode, wide char, string), it must be entirely rewritten. And this application isn't multilanguage yet, lot of parts are hard-coded in french.

It uses:

- For STT : Google translate.
- For STT : picoTTS.
- For hotword detection, software solution with https://github.com/Kitt-AI/snowboy, or hardware with http://www.mikroe.com/click/speakup/

To use it, you need:

- A raspberry, (I m using a B version)
- A 8*8 matrix, 3 colors.
- A microphone with an usb sound card.
- A 433Mhz transmitter.


The hardware part:
- How to set the matrix  https://learn.adafruit.com/adafruit-led-backpack/bi-color-8x8-matrix.
- How to set the transmitter, you need to connect the data to pin 13/GPIO2 (Pin 2 for wiringpi).
 
The software part:

A this moment you have 2 solutions, take the pre-compiled version, or compile it yourself.
-The precompiled version, just download and extract files.
-If you want to compile it you need first to install.

    command

