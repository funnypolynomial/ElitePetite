#include <Arduino.h>
#include "XC4630d.h"
#include "LCD.h"

LCD lcd;

#ifdef SERIALIZE
#define SERIALISE_COMMENT(_c) if (_serialise) { Serial.print("; ");Serial.println(_c);}
#define SERIALISE_INIT(_w,_h,_s) if (_serialise) { Serial.print(_w);Serial.print(',');Serial.print(_h);Serial.print(',');Serial.println(_s);}
#define SERIALISE_BEGINFILL(_x,_y,_w,_h) if (_serialise) { Serial.print(_x);Serial.print(',');Serial.print(_y);Serial.print(',');Serial.print(_w);Serial.print(',');Serial.println(_h);}
#define SERIALISE_FILLCOLOUR(_len,_colour) if (_serialise) { Serial.print(_len);Serial.print(',');Serial.println(_colour);}
#define SERIALISE_FILLBYTE(_len,_colour) if (_serialise) { Serial.print(_len);Serial.print(',');Serial.println(_colour?0xFFFF:0x0000);}
#else
#define SERIALISE_COMMENT(_c)
#define SERIALISE_INIT(_w,_h,_s)
#define SERIALISE_BEGINFILL(_x,_y,_w,_h)
#define SERIALISE_FILLCOLOUR(_len,_colour)
#define SERIALISE_FILLBYTE(_len,_colour)
#endif

#define LCD_RD   B00000001
#define LCD_WR   B00000010
#define LCD_RS   B00000100
#define LCD_CS   B00001000
#define LCD_RST  B00010000

// optimised code, specific to Uno
#define ToggleDataWR PORTC = B00010101; PORTC = B00010111; // keeps RST, RS & RD HIGH 
#define FastData(d) FastData2((d) & B11111100, (d) & B00000011);
// Preserves 10,11,12 & 13 on B
#define FastData2(h, l) PORTD = (h); PORTB = (PORTB & B11111100) | (l); ToggleDataWR;


// Preserves 10,11,12 & 13 on B
#define FastCmdByte(c) PORTC = LCD_RST | LCD_RD; PORTD = c & B11111100; PORTB = (PORTB & B11111100) | (c & B00000011);PORTC |= LCD_WR; PORTC |= LCD_RS;
#define FastCmd(c) FastCmdByte(0); FastCmdByte(c);

void LCD::init()
{
  // reset
  delay(250);
  pinMode(A4, OUTPUT);
  digitalWrite(A4, LOW);
  delay(100);
  digitalWrite(A4, HIGH);
  delay(500);


  XC4630_init();
  ChipSelect(true);

#ifdef ROTATION_USB_LEFT
  XC4630_rotate(2);
#else
  XC4630_rotate(4);
#endif

  _rotation = XC4630_orientation;
  _width = XC4630_width;
  _height = XC4630_height;
  SERIALISE_COMMENT("*** START");
  SERIALISE_INIT(_width, _height, 1);
}

void LCD::ChipSelect(bool select)
{
  if (select)
    PORTC &= ~LCD_CS;
  else
    PORTC |= LCD_CS;
}


unsigned long LCD::beginFill(int x, int y, int w, int h)
{
  SERIALISE_BEGINFILL(x, y, w, h);
#ifdef ROTATION_USB_LEFT
  // rotation = 2 case
  int x2 = x + w - 1;
  int y2 = y + h - 1;

  int tmp = x;
  int tmp2 = x2;
  x = y;
  x2 = y2;
  y = _width - tmp2 - 1;
  y2 = _width - tmp - 1;

  FastCmd(0x50);               //set x bounds
  FastData(x >> 8);
  FastData(x);
  FastCmd(0x51);               //set x bounds
  FastData(x2 >> 8);
  FastData(x2);
  FastCmd(0x52);               //set y bounds
  FastData(y >> 8);
  FastData(y);
  FastCmd(0x53);               //set y bounds
  FastData(y2 >> 8);
  FastData(y2);

  FastCmd(0x20);               //set x pos
  FastData(x >> 8);
  FastData(x);
  FastCmd(0x21);               //set y pos
  FastData(y2 >> 8);
  FastData(y2);

  FastCmd(0x22);  // Write Data to GRAM
#else
  // rotation = 4 case
  int x2 = x + w - 1;
  int y2 = y + h - 1;

  int tmp = x;
  int tmp2 = x2;
  x = _height - y2 - 1;
  x2 = _height - y - 1;
  y = tmp;
  y2 = tmp2;

  FastCmd(0x50);               //set x bounds
  FastData(x >> 8);
  FastData(x);
  FastCmd(0x51);               //set x bounds
  FastData(x2 >> 8);
  FastData(x2);
  FastCmd(0x52);               //set y bounds
  FastData(y >> 8);
  FastData(y);
  FastCmd(0x53);               //set y bounds
  FastData(y2 >> 8);
  FastData(y2);

  FastCmd(0x20);               //set x pos
  FastData(x2 >> 8);
  FastData(x2);
  FastCmd(0x21);               //set y pos
  FastData(y >> 8);
  FastData(y);

  FastCmd(0x22);  // Write Data to GRAM
#endif
  unsigned long count = w;
  count *= h;
  return count;
}

void LCD::fillColour(unsigned long count, word colour)
{
  SERIALISE_FILLCOLOUR(count, colour);

  // fill with full 16-bit colour
  byte h1 = (colour >> 8) & B11111100;
  byte l1 = (colour >> 8) & B00000011;
  byte h2 =  colour       & B11111100;
  byte l2 =  colour       & B00000011;
  while (count--)
  {
    FastData2(h1, l1);
    FastData2(h2, l2);
  }
}

void LCD::fillByte(unsigned long count, byte colour)
{
  SERIALISE_FILLBYTE(count, colour);

  // fill with just one byte, i.e. 0/black or 255/white, or other, for pastels
  PORTD = colour & B11111100;
  PORTB = (PORTB & B11111100) | (colour & B00000011);
  while (count--)
  {
    ToggleDataWR;
    ToggleDataWR;
  }
}

void LCD::OneWhite()
{
  SERIALISE_FILLBYTE(1, 0xFF);

  PORTD = B11111100;
  PORTB = (PORTB & B11111100) | B00000011;
  ToggleDataWR;
  ToggleDataWR;
}

void LCD::OneBlack()
{
  SERIALISE_FILLBYTE(1, 0x00);

  // fill with just one byte, i.e. 0/black or 255/white, or other, for pastels
  PORTD = 0x00;
  PORTB = PORTB & B11111100;
  ToggleDataWR;
  ToggleDataWR;
}

bool LCD::isTouch(int x, int y, int w, int h)
{
  return XC4630_istouch(x, y, x + w, y + h);
}

bool LCD::getTouch(int& x, int& y)
{
  x = XC4630_touchx();
  y = XC4630_touchy();
  return x >= 0 && x >= 0;
}
