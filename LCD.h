#pragma once

// If defined, use later revision of Jaycar module, anti-static bag, otherwise blister pack
#define XC4630_HX8347i

// If defined, LCD oriented with USB left, otherwise right
//#define ROTATION_USB_LEFT

// If defined initialisation runs touch calibration routine, DEBUG needs to be defined too, see LCD::TouchCalib()
//#define XC4630_TOUCH_CALIB

// If defined initialisation runs touch check routine, cross-hairs drawn at stylus, see LCD::TouchCheck()
//#define XC4630_TOUCH_CHECK

// make an RGB word
#define RGB(_r, _g, _b) (word)((_b & 0x00F8) >> 3) | ((_g & 0x00FC) << 3) | ((_r & 0x00F8) << 8)

// optionally dump graphics cmds to serial:
//#define SERIALIZE
#ifdef SERIALIZE
#define SERIALISE_ON(_on) lcd._serialise=_on;
#else
#define SERIALISE_ON(_on)
#endif

class LCD
{
  public:
     void init();
     void ChipSelect(bool select);

     unsigned long beginFill(int x, int y,int w,int h);
     void fillColour(unsigned long size, word colour);
     void fillByte(unsigned long size, byte colour);
     void OneWhite();
     void OneBlack();

     bool isTouch(int x, int y, int w,int h);
     bool getTouch(int& x, int& y);
     void touchCalib();
     void touchCheck();
     
#ifdef SERIALIZE
     bool _serialise = false;
#endif
};

extern LCD lcd;
