# Rhonda (under construction)
Rhondha is just another personal home assistant working with speech recognition (a Javis like). Developped for Raspberry, it was made to be the lightest possible with the minimal access to the SD card as possible.  
All the configuration will be done with one xml file https://github.com/Smanar/Rhonda/blob/master/rhonda/config.xml .  
Some actions are hard-coded but you can use specials shells scripts for personals actions.  
It can too command some 433 Mhz devices and use a 8*8 matrix to display some icons.  

So firstly, I know the code is ugly, but this project was written in shell then in C and finaly in C++, with lot of modifications (unicode, wide char, string), it must be entirely rewritten. And this application isn't multilanguage yet, lot of parts are hard-coded in french.

You can see somes pictures here https://github.com/Smanar/Rhonda/wiki


----------


**It uses:**

- For STT : Google Speech recognition API.
- For STT : picoTTS.
- For hotword detection, software solution with https://github.com/Kitt-AI/snowboy, or hardware with http://www.mikroe.com/click/speakup/


----------


**You need:**

- A raspberry, (I m using a B version) with a working microphone and audio. I have made my test with a dongle that make output and input in same time like this one https://www.adafruit.com/product/1475 it doesn't work yet with Jack.
- A 8*8 matrix, 3 colors.
- A 433Mhz transmitter.


----------


**The hardware part:**
- How to set the matrix  https://learn.adafruit.com/adafruit-led-backpack/bi-color-8x8-matrix.
- How to set the transmitter http://blog.idleman.fr/raspberry-pi-08-jouer-avec-les-ondes-radio/  you need to connect the data to pin 13/GPIO2 (Pin 2 for wiringpi).
 


----------


**The software part:**

- PicotTTS

>  sudo apt-get install libttspico-utils


- Atlas matrix computing library

> sudo apt-get install libatlas-base-dev

----------


A this moment you have 2 solutions, take the pre-compiled version, or compile it yourself.

- The precompiled version, just download the archive here https://github.com/Smanar/Rhonda/releases/download/v1.0.0/Rhonda_release.zip , and extract files.
- If you want to compile it you need first to install somes libraries, the codec FLAC, CURL and various audio libs.

> sudo apt-get install flac  
> sudo apt-get install libflac-dev  
> sudo apt-get install curl  
> sudo apt-get install libcurl4-openssl-dev  
> sudo apt-get install libjack-jackd2-dev libsndfile1-dev libasound2-dev  

You need too,  buid the portaudio library, but if you don't need special configuration, you will have all the files inside the archive.

There is a code::block project in the source, so you can install it, open the *cdb file and build the project. Be patient, it will take lot of time (the file pugi.xml itself need more than 4/5 mn). But for the next time, you will rebuild only the modified files.

> sudo apt-get install codeblocks

It will be realy slow, but it can compile the code.  
Or you can use the makefile (TODO)  

> make

Take care your file will be in bin/Release and you need to copy the "resources" folder and the xml file in the same folder than the created executable.

----------


**The configuration**

Open the xml file, and change the &lt;api> tag key to set your own key.  
To get the api key http://www.chromium.org/developers/how-tos/api-keys  
And then just run the application.

>sudo chmod -R 755 shell
>sudo chmod 755 rhonda  
>./rhonda


----------
**Remarques speciales**

Note pour les utilisateurs francais, le hotword "snowboy" est assez dur a reproduire, du moins pour moi avec mon accent pourri.
Donc allez plutot sur le site https://snowboy.kitt.ai/ fabriquez vous 2 hotwords, du style "Rhonda" et "tu m'ecoutes ?"
Dans le xml mettez

    <sound_engine>
        <model>resources/rhonda.pmdl,resources/tumecoutes.pmdl</model>
        <sensibility>0.5,0.5</sensibility>

Ceci pour eviter les faux positifs, en fait au declenchement du premier mot, vous avec 5/6s seconde pour declencher le second, sinon il repart a zero.

Plus d'information pour les francais ici https://github.com/Smanar/Rhonda/wiki

