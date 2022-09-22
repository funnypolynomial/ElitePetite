# ElitePetite
A small tribute to the 80's game Elite. An Arduino+LCD shows the iconic tumbling Cobra Mk 3, plus some extras (other ships, clock mode).

![320x240 Touch LCD shield](https://live.staticflickr.com/65535/51751679186_e0ebcc29b4_4k.jpg)
![480x320 LCD shield](https://live.staticflickr.com/65535/51752324209_3cc05c5bf5_4k.jpg)

[YouTube video](https://youtu.be/IbI2hjIkLns)

## *No this is **NOT** Elite ported to the Arduino!*

ElitePetite presents an approximation of the loading screens of Acornsoft's Elite, as seen on many BBC Microcomputers and Acorn Electrons in the 80's (including mine!).
Basically just the iconic "Load New Commander (Y/N)" screen with the tumbling Cobra Mk 3. But there was some feature creep, so with some extras including:
- 12 ship choices
- A clock mode and a clock "ship" (to give the project a reason to sit on the shelf, a purpose)
- The Status/COMMANDER screen
- The "Saturn" credits/splash screen
- Touch screen interaction including drag to manually pitch/roll ship
- Touch screen or button-press interaction with a menu system to configure ship, machine, background etc

Written from scratch but using original data from [github](https://github.com/markmoxon/electron-elite-beebasm) and informed by the amazing [BBCElite](https://www.bbcelite.com/).  

The normals of the ship's 3D faces are rotated by the current pitch & roll (there is no yaw).  If they are facing us (have a +ve Z) then all the vertices associated with them are also rotated, then all the associated edges (lines) are drawn, between the transformed vertices.  The associations are baked into the ship data structures. I found that I needed to compute my own face normals, the original values didn't work well with the 3D calculations I was doing.

But by far the bigest job was getting those ship edges to the LCD, fast.  The original game running on a BBC Micro or Acorn Electron had ~10k of screen RAM to play with and it directly XOR'd lines in (and out) of the display.  I only had 2k RAM minus variables and some stack.  So, a modified Bresenham line algorithm sets "pixels" in the Sparse data-structure.  This is basically row-based run-length-encoding, done on-the-fly.  At the end, when all the lines are done, the entire ship rectangle is drawn as a raster to the screen.  But it *does* XOR the ship raster data with the top and bottom text (ELITE & Load New), for that extra verisimilitude.  Even with all that, I get a frame rate of ~10Hz.

A fair bit of all this is documented in the code...

See also [Hackaday](https://hackaday.io/project/183107-elitepetite) and [flickr](https://flic.kr/s/aHBqjzvodW)

**NOTE:** The Small/Touch version now works with **both** revisions of the Jaycar XC-4630:
- the original which comes in a *blister pack*, with *"2.8" TFT LCD Shield V3"* printed on the back of the board. 
- AND the newer one which comes in an *anti-static bag* and has "HX8347" on the back.
- See the XC4630_HX8347i define in LCD.h
