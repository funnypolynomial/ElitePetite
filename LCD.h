#pragma once

// If defined, LCD oriented with USB left, otherwise right
//#define ROTATION_USB_LEFT

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
     const int Width = 240;
     const int Height = 320;
     void init();
     void ChipSelect(bool select);

     unsigned long beginFill(int x, int y,int w,int h);
     void fillColour(unsigned long size, word colour);
     void fillByte(unsigned long size, byte colour);
     void OneWhite();
     void OneBlack();

     bool isTouch(int x, int y, int w,int h);
     bool getTouch(int& x, int& y);
     
     int  _width;
     int  _height;
     int  _rotation;

#ifdef SERIALIZE
     bool _serialise = false;
#endif
};

extern LCD lcd;
