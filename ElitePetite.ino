#include <Arduino.h>
// External libraries:
#include <SoftwareI2C.h>  // see RTC.h

#include "RTC.h"
#include "Pins.h"
#include "BTN.h"
#include "Text.h"
#include "Sparse.h"
#include "Ship.h"
#include "Loader.h"
#include "Config.h"
#include "Elite.h"


/*
 *    ---- E l i t e P e t i t e ----
 *    A tribute to the 1980's game Elite. Mark Wilson, September-December 2021.
 *    
 *    ElitePetite presents an approximation of the loading screens of AcornSoft's Elite, as seen on many Acorn Electrons and BBC Microcomputers in the 80's.
 *    
 *    >>>>>> It is *NOT* the game ported to the Arduino :-). <<<<<<
 *    
 *    Basically just the iconic "Load New Commander (Y/N)" screen with the tumbling Cobra Mk 3, with some extras including:
 *      * 12 ship choices
 *      * A clock mode and a clock "ship" (to give the project a reason to sit on the shelf, a purpose)
 *      * The Status/COMMANDER screen
 *      * The "Saturn" credits/splash screen
 *      * Touch screen interaction including drag to manually pitch/roll the ship
 * 
 *      
 *    This started as a lockdown project triggered while reading the "Acorn Computer and BBC Micro Enthusiasts" FB group. I had an Electron in the 80's and played a
 *    LOT of Elite, eventually reaching that status.  Having had some success (and fun) with my FlipClock project, I thought I'd try recreating something of Elite on
 *    an LCD with an Arduino.  Some time later I'm calling it done. Basicaly if I thought of something to add, I added it (approach animation, clock ship, etc, etc, etc).
 *    I have now run out of program space (less than 4% free).
 *    
 *    Elite on the original hardware had the luxury of XORing lines into ~10k of video RAM, 320x256 1-bit pixels.
 *    Only 2k RAM total on a Uno, so a considerable part of this project has been devising the Sparse algorithm that lets me "draw" lines to run-length-encoded data structures.
 *    I also made the effort to recreate the effect of the ship XORing though the text as it tumbles.
 *    Efficient rendering of the sparse lines to the LCD and using integer-based 3D calculations give me a pretty reasonable frame rate for the ship animation of ~10Hz.
 *    
 *    ElitePetite starts by displaying a screen simulating typing commands to load and run Elite.  
 *    It then recreates the "Saturn" loading screen before showing the "Load New Commander (Y/N)?" screen with a tumbling Cobra Mk 3.
 *    (For verisimilitude, when simulating the Electron, the loading screen will end by showing program data loading into the video RAM.)
 *    ElitePetite supports two imput methods -- a touch screen and push buttons (see Customization).
 *    Tapping on an element on the screen changes it.
 *      * tapping on the ship changes to the next ship in the list (see below)
 *      * tapping on the dial/scanner window toggles the machine between monochrome (Electron) and colour (BBC)
 *      * tapping on the background areas either side of the Elite screen step through various fill modes: black, the BBC Owl, the Electron squares pattern, beige or random stars
 *      * tapping on the title toggles between Elite and the time (if enabled, see Customization)
 *    Pressing and holding on the ship changes to manual mode. Tumbling stops and dragging changes the pitch and roll angles.
 *    Click to resume tumbling.  Manual mode and the menu both time out.
 *    Tapping on the Load or Acornsoft text brings up the Menu.  
 *    The menu is also accessed by pressing the Set button. Outside the menu, pressing Adj cycles the ship.
 *    The menu is operated by taps on the screen or with button presses: Set selects an item, Adj moves to the next item.
 *    The menu consists of:
 *      * Ship: A list of ships - Adder, Anaconda, Boa, Cobra Mk 3, Constrictor, Fer De Lance, Krait, Mamba, Moray, Python, Sidewinder & Viper.
 *              Along with RANDOM, which cycles through the ships at random and, if enabled, CLOCK which shows a tumbling "ship" with the time in 3D.
 *      * Screen: A choice of Load (the tumbling ship view) or Status (the COMMANDER screen).
 *      * Machine: A choice of BBC Micro or Acorn Electron.
 *      * Background: A choice of fill for the parts of the screen not taken up by the Elite screen; black, Owl etc.    
 *      * Title: A choice of Elite or Time (if enabled). The time also replaces the COMMANDER line on the Status page.
 *      * Time: Sets the time AND choice of display mode, 12- or 24-hour (if enabled).  See Config.cpp
 *      * Restart: Starts back at the command prompt step of the display.
 *      * About: Shows brief credits.
 *      * Exit: Exits the menu.
 *    But see Customization -- menu items may be omitted.
 *    
 *    On the Jaycar XC4630 240x320 LCD screen the "Ship Window" is slightly shorter.  It is also narrower than the screen. See Elite_Small.h
 *    On the 320x480 shield the window is full size (256x248). See Elite_Large.h
 *      
 *    Resources:
 *      Mostly https://www.bbcelite.com/, https://github.com/markmoxon/electron-elite-beebasm and https://github.com/markmoxon/cassette-elite-beebasm
 *      Also some YouTube videos, https://www.youtube.com/watch?v=3f2JhORxsCY, https://www.youtube.com/watch?v=PnhHyMxt1S0
 *      All the code was written from scratch, including the 3D view ship-rendering.  The vertices/edges/faces of the original ships are from the asm files on GitHub.
 *      The image data for the dials and the large text on the credit screen are also from files on GitHub.
 *      The "Saturn" credits screen is C code written to recreate (mostly) what the original asm did.
 *      Sadly I have not yet been able to resurrect my own Electron otherwise I might have tried loading "Elite" from tape.
 *      
 *    Customization:
 *      Include or exclude a Real Time Clock via RTC_I2C_ADDRESS in RTC.h
 *      Include or exclude Set & Adj pushbuttons via PIN_BTN_SET & PIN_BTN_ADJ in Pins.h
 *      Customize the LCD (and Touch) via the defines in Elite.h
 *         * See Elite_Small.h and Elite_Large.h which define the LCD/Touch interface for two different LCD shields.
 *      Several other things can be customised, see the ENABLE_ defines in Elite.h
 *      The text shown in the Status page (Commander name, Rating etc) can be edited (statusText in voew.cpp).
 *      
 *    Credits:
 *      I couldn't have done this without the extraordinary resource of bbcelite.com and the sources on GitHub.
 *      But it all started with the hard work of Bell & Braben:
 *        * BBC Micro Elite was written by Ian Bell and David Braben and is copyright (c) Acornsoft 1984.
 *        * Acorn Electron Elite was written by Ian Bell and David Braben and is copyright (c) Acornsoft 1984.
 *      Looking at all those lines of asm reinforces what a mammoth effort it was.
 */


void setup()
{
#ifdef DEBUG
  Serial.begin(38400);
  Serial.println("ElitePetite");
#endif  
  rtc.Setup();
#if defined(PIN_BTN_SET) && defined(PIN_BTN_ADJ)
  btn1Set.Init(PIN_BTN_SET);
  btn2Adj.Init(PIN_BTN_ADJ);
#endif  
  LCD_INIT();
  elite::Init();
}

#ifdef DEBUG
// frame-rate stats
unsigned long sumMS = 0;
extern unsigned long sumLinesMS;
extern unsigned long sumPaintMS;
int count = 0;
#endif

void loop()
{
  config::Loop();
  unsigned long nowMS = millis();
  elite::Loop();
  unsigned long durationMS = millis() - nowMS;
#ifdef DEBUG
  sumMS += durationMS;
  count++;
  if (count == 2000)
  {
    Serial.println(sumMS / count);
    Serial.println(sumLinesMS / count);
    Serial.println(sumPaintMS / count);
    // Cobra: ~85ms, 47ms lines, 38ms paint (without approach/radar) / 94,57,38 (FULL) 
    DBG(sparse::highWater); // Cobra:430 / 760 (FULL). Fur-de-lance ~ 460/780
  }
#endif  
  if (durationMS < config::data.minimumFramePeriodMS)
    delay(config::data.minimumFramePeriodMS - durationMS);
}
