#include <Arduino.h>
#include "Elite.h"
#include "Text.h"

namespace text {
extern const byte Font[];  // forward

// read a char from a pointer to PROGMEM or RAM
char ProgMemCharReader(const char* a) { return pgm_read_byte(a); }
char StdCharReader    (const char* a) { return *a; }

void Draw(int x, int y, const char* str, CharReader charReader, word colour)
{
  // Draw the string using the BBC font at {x, y} (which are LCD coordinates), using the reader to access the string
  // here colour=0 means white
  // A ctrl-O turns on "italic"!
  byte len = 0;
  while (charReader(str + len))
    len++;
  bool italic = charReader(str) == '\xF'; // ctrl-O (Shift In) = italics
  if (italic)
  {
    len--;
    LCD_BEGIN_FILL(x, y, 8*len + 1, 8);
    str++;
  }
  else
  {
    LCD_BEGIN_FILL(x, y, 8*len, 8);
  }
  raster::Start(x, y, str, charReader);
  for (byte row = 0; row < 8; row++)
  {
    raster::Row(x, y+row);
    int cols = 8*len;
    if (italic && row < 4)
      LCD_ONE_BLACK();
      
    while (cols--)
      if (raster::Next())
        if (colour)
          LCD_FILL_COLOUR(1, colour);
        else
          LCD_ONE_WHITE();
      else
        LCD_ONE_BLACK();
        
    if (italic && row >= 4)
      LCD_ONE_BLACK();
  }
}

const char* StrN(const char* multiStr, byte n, CharReader charReader)
{
  // return the Nth string in the given mult-string, or NULL
  // the first is 0
  while (n)
  {
    while (charReader(multiStr))
      multiStr++;
    multiStr++;
    n--;
  }
  return charReader(multiStr)?multiStr:NULL;
}

size_t StrLen(const char* str, CharReader charReader)
{
  // return the length of the string
  size_t len = 0;
  while (charReader(str++))
    len++;
  return len;
}

} // namespace text


namespace raster {
// Rasterize text so it can be combined with sparse drawing to create the XOR'd effect
int _TextX = 0;
int _TextY = 0;
int _TextRow = 0;
const char* _TextStr = NULL;
byte _StrIdx = 0;
text::CharReader _CharReader = NULL;
int _LeadIn = 0;
byte _CurrentChar = 0;
byte _CurrentDefnByte = 0;
byte _CurrentDefnMask = 0;

void Start(int x, int y, const char* str, text::CharReader charReader)
{
  // defines where the text is to be drawn
  _TextX = x;
  _TextY = y;
  _TextStr = str;
  _StrIdx = 0;
  _CharReader = charReader;
}

void Row(int x, int y)
{
  // define the start of a row at x, y
  _TextRow = y - _TextY;
  if (_TextRow < 0 || _TextRow > 7)
  {
    _LeadIn = 0xFFFF;  // all blank
    return;
  }
  _StrIdx = 0;
  _CurrentDefnMask = 0x80;
  int dX = x - _TextX;
  if (dX < 0)
  {
    _LeadIn = -dX;
  }
  else
  {
    _LeadIn = 0;
    _StrIdx = dX / 8;
    _CurrentDefnMask >>= dX % 8;
  }
  _CurrentChar = _CharReader(_TextStr + _StrIdx++);
  _CurrentDefnByte = pgm_read_byte_near(text::Font + 8*(_CurrentChar - ' ') + _TextRow);
}

bool Next()
{
  // return the next bit in the text row
  bool result = false;
  if (_LeadIn)  // before we get to the first char
    _LeadIn--;
  else if (_CurrentChar)
  {
    if (!_CurrentDefnMask)
    {
      _CurrentChar = _CharReader(_TextStr + _StrIdx++);
      _CurrentDefnMask = 0x80;
      _CurrentDefnByte = _CurrentChar?pgm_read_byte_near(text::Font + 8*(_CurrentChar - ' ') + _TextRow):0x00;
    }
    result = _CurrentDefnByte & _CurrentDefnMask;
    _CurrentDefnMask >>= 1;
  }
  return result;
}
} // namespace raster

//--------------------------------------------------------
// The BBC font
#define X (1)
#define _ (0)

#define LINE(_a, _b, _c, _d, _e, _f, _g, _h) ((_a << 7) | \
                                              (_b << 6) | \
                                              (_c << 5) | \
                                              (_d << 4) | \
                                              (_e << 3) | \
                                              (_f << 2) | \
                                              (_g << 1) | \
                                              (_h << 0))

const byte text::Font[] PROGMEM =
{
  LINE(_,_,_,_,_,_,_,_), // <sp>
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,_,X,X,_,_,_), // !
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,X,X,_,X,X,_,_), // "
  LINE(_,X,X,_,X,X,_,_),
  LINE(_,X,X,_,X,X,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,X,X,_,X,X,_), // #
  LINE(_,_,X,X,_,X,X,_),
  LINE(_,X,X,X,X,X,X,X),
  LINE(_,_,X,X,_,X,X,_),
  LINE(_,X,X,X,X,X,X,X),
  LINE(_,_,X,X,_,X,X,_),
  LINE(_,_,X,X,_,X,X,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,_,_,X,X,_,_), // $
  LINE(_,_,X,X,X,X,X,X),
  LINE(_,X,X,_,X,_,_,_),
  LINE(_,_,X,X,X,X,X,_),
  LINE(_,_,_,_,X,_,X,X),
  LINE(_,X,X,X,X,X,X,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,X,X,_,_,_,_,_), // %
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,_,_,_,X,X,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,X,X,_,_,_,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,_,_,_,_,X,X,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,X,X,X,_,_,_), // &
  LINE(_,X,X,_,X,X,_,_),
  LINE(_,X,X,_,X,X,_,_),
  LINE(_,_,X,X,X,_,_,_),
  LINE(_,X,X,_,_,X,_,X),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,_,X,X,X,_,X,X),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,_,_,X,X,_,_), // '
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,X,X,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,_,_,X,X,_,_), // (
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,X,X,_,_,_,_),
  LINE(_,_,X,X,_,_,_,_),
  LINE(_,_,X,X,_,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,_,X,X,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,X,X,_,_,_,_), // )
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,_,X,X,_,_),
  LINE(_,_,_,_,X,X,_,_),
  LINE(_,_,_,_,X,X,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,X,X,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,_,_,_,_,_,_), // *
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,X,X,X,X,X,X,_),
  LINE(_,_,X,X,X,X,_,_),
  LINE(_,X,X,X,X,X,X,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,_,_,_,_,_,_), // +
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,X,X,X,X,X,X,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,_,_,_,_,_,_), // ,
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,X,X,_,_,_,_),

  LINE(_,_,_,_,_,_,_,_), // -
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,X,X,X,X,X,X,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,_,_,_,_,_,_), // .
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),

  LINE(_,_,_,_,_,_,_,_), // /
  LINE(_,_,_,_,_,X,X,_),
  LINE(_,_,_,_,X,X,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,X,X,_,_,_,_),
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,X,X,X,X,_,_), // 0
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,X,X,X,_),
  LINE(_,X,X,X,X,X,X,_),
  LINE(_,X,X,X,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,_,X,X,X,X,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,_,X,X,_,_,_), // 1
  LINE(_,_,X,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,X,X,X,X,X,X,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,X,X,X,X,_,_), // 2
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,_,_,_,_,X,X,_),
  LINE(_,_,_,_,X,X,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,X,X,_,_,_,_),
  LINE(_,X,X,X,X,X,X,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,X,X,X,X,_,_), // 3
  LINE(_,X,_,_,_,X,X,_),
  LINE(_,_,_,_,_,X,X,_),
  LINE(_,_,_,X,X,X,_,_),
  LINE(_,_,_,_,_,X,X,_),
  LINE(_,X,_,_,_,X,X,_),
  LINE(_,_,X,X,X,X,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,_,_,X,X,_,_), // 4
  LINE(_,_,_,X,X,X,_,_),
  LINE(_,_,X,X,X,X,_,_),
  LINE(_,X,X,_,X,X,_,_),
  LINE(_,X,X,X,X,X,X,_),
  LINE(_,_,_,_,X,X,_,_),
  LINE(_,_,_,_,X,X,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,X,X,X,X,X,X,_), // 5
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,X,X,X,X,X,_,_),
  LINE(_,_,_,_,_,X,X,_),
  LINE(_,_,_,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,_,X,X,X,X,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,_,X,X,X,_,_), // 6
  LINE(_,_,X,X,_,_,_,_),
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,X,X,X,X,X,_,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,_,X,X,X,X,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,X,X,X,X,X,X,_), // 7
  LINE(_,_,_,_,_,X,X,_),
  LINE(_,_,_,_,X,X,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,X,X,_,_,_,_),
  LINE(_,_,X,X,_,_,_,_),
  LINE(_,_,X,X,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,X,X,X,X,_,_), // 8
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,_,X,X,X,X,_,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,_,X,X,X,X,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,X,X,X,X,_,_), // 9
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,_,X,X,X,X,X,_),
  LINE(_,_,_,_,_,X,X,_),
  LINE(_,_,_,_,X,X,_,_),
  LINE(_,_,X,X,X,_,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,_,_,_,_,_,_), // :
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,_,_,_,_,_,_), // ;
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,X,X,_,_,_,_),

  LINE(_,_,_,_,X,X,_,_), // <
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,X,X,_,_,_,_),
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,_,X,X,_,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,_,X,X,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,_,_,_,_,_,_), // =
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,X,X,X,X,X,X,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,X,X,X,X,X,X,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,X,X,_,_,_,_), // >
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,_,X,X,_,_),
  LINE(_,_,_,_,_,X,X,_),
  LINE(_,_,_,_,X,X,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,X,X,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,X,X,X,X,_,_), // ?
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,_,_,_,X,X,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,X,X,X,X,_,_), // @
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,X,X,X,_),
  LINE(_,X,X,_,X,_,X,_),
  LINE(_,X,X,_,X,X,X,_),
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,_,X,X,X,X,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,X,X,X,X,_,_), // A
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,X,X,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,X,X,X,X,X,_,_), // B
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,X,X,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,X,X,X,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,X,X,X,X,_,_), // C
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,_,X,X,X,X,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,X,X,X,X,_,_,_), // D
  LINE(_,X,X,_,X,X,_,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,X,X,_,_),
  LINE(_,X,X,X,X,_,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,X,X,X,X,X,X,_), // E
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,X,X,X,X,X,_,_),
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,X,X,X,X,X,X,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,X,X,X,X,X,X,_), // F
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,X,X,X,X,X,_,_),
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,X,X,X,X,_,_), // G
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,X,X,_,X,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,_,X,X,X,X,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,X,X,_,_,X,X,_), // H
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,X,X,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,X,X,X,X,X,X,_), // I
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,X,X,X,X,X,X,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,X,X,X,X,X,_), // J
  LINE(_,_,_,_,X,X,_,_),
  LINE(_,_,_,_,X,X,_,_),
  LINE(_,_,_,_,X,X,_,_),
  LINE(_,_,_,_,X,X,_,_),
  LINE(_,X,X,_,X,X,_,_),
  LINE(_,_,X,X,X,_,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,X,X,_,_,X,X,_), // K
  LINE(_,X,X,_,X,X,_,_),
  LINE(_,X,X,X,X,_,_,_),
  LINE(_,X,X,X,_,_,_,_),
  LINE(_,X,X,X,X,_,_,_),
  LINE(_,X,X,_,X,X,_,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,X,X,_,_,_,_,_), // L
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,X,X,X,X,X,X,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,X,X,_,_,_,X,X), // M
  LINE(_,X,X,X,_,X,X,X),
  LINE(_,X,X,X,X,X,X,X),
  LINE(_,X,X,_,X,_,X,X),
  LINE(_,X,X,_,X,_,X,X),
  LINE(_,X,X,_,_,_,X,X),
  LINE(_,X,X,_,_,_,X,X),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,X,X,_,_,X,X,_), // N
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,X,_,X,X,_),
  LINE(_,X,X,X,X,X,X,_),
  LINE(_,X,X,_,X,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,X,X,X,X,_,_), // O
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,_,X,X,X,X,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,X,X,X,X,X,_,_), // P
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,X,X,X,_,_),
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,X,X,X,X,_,_), // Q
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,X,_,X,_),
  LINE(_,X,X,_,X,X,_,_),
  LINE(_,_,X,X,_,X,X,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,X,X,X,X,X,_,_), // R
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,X,X,X,_,_),
  LINE(_,X,X,_,X,X,_,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,X,X,X,X,_,_), // S
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,_,X,X,X,X,_,_),
  LINE(_,_,_,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,_,X,X,X,X,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,X,X,X,X,X,X,_), // T
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,X,X,_,_,X,X,_), // U
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,_,X,X,X,X,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,X,X,_,_,X,X,_), // V
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,_,X,X,X,X,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,X,X,_,_,_,X,X), // W
  LINE(_,X,X,_,_,_,X,X),
  LINE(_,X,X,_,X,_,X,X),
  LINE(_,X,X,_,X,_,X,X),
  LINE(_,X,X,X,X,X,X,X),
  LINE(_,X,X,X,_,X,X,X),
  LINE(_,X,X,_,_,_,X,X),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,X,X,_,_,X,X,_), // X
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,_,X,X,X,X,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,X,X,X,X,_,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,X,X,_,_,X,X,_), // Y
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,_,X,X,X,X,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,X,X,X,X,X,X,_), // Z
  LINE(_,_,_,_,_,X,X,_),
  LINE(_,_,_,_,X,X,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,X,X,_,_,_,_),
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,X,X,X,X,X,X,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,X,X,X,X,X,_,_), // [
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,X,X,X,X,X,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,_,_,_,_,_,_), // <backslash>
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,_,X,X,_,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,_,X,X,_,_),
  LINE(_,_,_,_,_,X,X,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,X,X,X,X,X,_), // ]
  LINE(_,_,_,_,_,X,X,_),
  LINE(_,_,_,_,_,X,X,_),
  LINE(_,_,_,_,_,X,X,_),
  LINE(_,_,_,_,_,X,X,_),
  LINE(_,_,_,_,_,X,X,_),
  LINE(_,_,X,X,X,X,X,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,_,X,X,_,_,_), // ^
  LINE(_,_,X,X,X,X,_,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,_,_,_,_,X,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,_,_,_,_,_,_), // _
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(X,X,X,X,X,X,X,_),

  LINE(_,_,_,X,X,X,_,_), // <pound>
  LINE(_,_,X,X,_,X,X,_),
  LINE(_,_,X,X,_,_,_,_),
  LINE(_,X,X,X,X,X,_,_),
  LINE(_,_,X,X,_,_,_,_),
  LINE(_,_,X,X,_,_,_,_),
  LINE(_,X,X,X,X,X,X,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,_,_,_,_,_,_), // a
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,X,X,X,X,_,_),
  LINE(_,_,_,_,_,X,X,_),
  LINE(_,_,X,X,X,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,_,X,X,X,X,X,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,X,X,_,_,_,_,_), // b
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,X,X,X,X,X,_,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,X,X,X,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,_,_,_,_,_,_), // c
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,X,X,X,X,_,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,_,X,X,X,X,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,_,_,_,X,X,_), // d
  LINE(_,_,_,_,_,X,X,_),
  LINE(_,_,_,_,_,X,X,_),
  LINE(_,_,X,X,X,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,_,X,X,X,X,X,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,_,_,_,_,_,_), // e
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,X,X,X,X,_,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,X,X,X,X,_),
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,_,X,X,X,X,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,_,X,X,X,_,_), // f
  LINE(_,_,X,X,_,_,_,_),
  LINE(_,_,X,X,_,_,_,_),
  LINE(_,X,X,X,X,X,_,_),
  LINE(_,_,X,X,_,_,_,_),
  LINE(_,_,X,X,_,_,_,_),
  LINE(_,_,X,X,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,_,_,_,_,_,_), // g
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,X,X,X,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,_,X,X,X,X,X,_),
  LINE(_,_,_,_,_,X,X,_),
  LINE(_,_,X,X,X,X,_,_),

  LINE(_,X,X,_,_,_,_,_), // h
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,X,X,X,X,X,_,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,_,X,X,_,_,_), // i
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,X,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,X,X,X,X,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,_,X,X,_,_,_), // j
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,X,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,X,X,X,_,_,_,_),

  LINE(_,X,X,_,_,_,_,_), // k
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,X,X,_,_),
  LINE(_,X,X,X,X,_,_,_),
  LINE(_,X,X,_,X,X,_,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,X,X,X,_,_,_), // l
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,X,X,X,X,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,_,_,_,_,_,_), // m
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,X,X,_,X,X,_),
  LINE(_,X,X,X,X,X,X,X),
  LINE(_,X,X,_,X,_,X,X),
  LINE(_,X,X,_,X,_,X,X),
  LINE(_,X,X,_,_,_,X,X),
  LINE(_,_,_,_,_,_,_,_),


  // these come from the faxed page (the other was mailed):
  LINE(_,_,_,_,_,_,_,_), // n
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,X,X,X,X,X,_,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,_,_,_,_,_,_), // o
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,X,X,X,X,_,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,_,X,X,X,X,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,_,_,_,_,_,_), // p
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,X,X,X,X,X,_,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,X,X,X,_,_),
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,X,X,_,_,_,_,_),

  LINE(_,_,_,_,_,_,_,_), // q
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,X,X,X,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,_,X,X,X,X,X,_),
  LINE(_,_,_,_,_,X,X,_),
  LINE(_,_,_,_,_,X,X,X),

  LINE(_,_,_,_,_,_,_,_), // r
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,X,X,_,X,X,_,_),
  LINE(_,X,X,X,_,X,X,_),
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,_,_,_,_,_,_), // s
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,X,X,X,X,X,_),
  LINE(_,X,X,_,_,_,_,_),
  LINE(_,_,X,X,X,X,_,_),
  LINE(_,_,_,_,_,X,X,_),
  LINE(_,X,X,X,X,X,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,X,X,_,_,_,_), // t
  LINE(_,_,X,X,_,_,_,_),
  LINE(_,X,X,X,X,X,_,_),
  LINE(_,_,X,X,_,_,_,_),
  LINE(_,_,X,X,_,_,_,_),
  LINE(_,_,X,X,_,_,_,_),
  LINE(_,_,_,X,X,X,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,_,_,_,_,_,_), // u
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,_,X,X,X,X,X,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,_,_,_,_,_,_), // v
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,_,X,X,X,X,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,_,_,_,_,_,_), // w
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,X,X,_,_,_,X,X),
  LINE(_,X,X,_,X,_,X,X),
  LINE(_,X,X,_,X,_,X,X),
  LINE(_,X,X,X,X,X,X,X),
  LINE(_,_,X,X,_,X,X,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,_,_,_,_,_,_), // x
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,_,X,X,X,X,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,X,X,X,X,_,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,_,_,_,_,_,_), // y
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,X,X,_,_,X,X,_),
  LINE(_,_,X,X,X,X,X,_),
  LINE(_,_,_,_,_,X,X,_),
  LINE(_,_,X,X,X,X,_,_),

  LINE(_,_,_,_,_,_,_,_), // z
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,X,X,X,X,X,X,_),
  LINE(_,_,_,_,X,X,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,X,X,_,_,_,_),
  LINE(_,X,X,X,X,X,X,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,_,_,_,X,X,_), // {
  LINE(_,_,_,_,X,X,_,_),
  LINE(_,_,_,_,X,X,_,_),
  LINE(_,_,X,X,X,_,_,_),
  LINE(_,_,_,_,X,X,_,_),
  LINE(_,_,_,_,X,X,_,_),
  LINE(_,_,_,_,_,X,X,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,_,X,X,_,_,_), // |
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,X,X,_,_,_,_), // }
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,_,X,X,X,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,X,X,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,X,X,_,_,_,X), // ~
  LINE(_,X,X,_,X,_,X,X),
  LINE(_,X,_,_,_,X,X,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),

/////////////////////////////////
// NOT part of the original font:
  LINE(_,_,_,X,X,_,_,_), // Acorn
  LINE(_,_,X,X,X,X,_,_),
  LINE(_,_,X,X,X,X,_,_),
  LINE(_,X,X,X,X,X,X,_),
  LINE(_,X,X,X,X,X,X,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,X,X,X,X,X,X,_),
  LINE(_,_,X,X,X,X,_,_),

  LINE(_,_,_,_,_,_,_,_), // 0x80 top-left
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,X,X,X,X,X),
  LINE(_,_,_,X,X,X,X,X),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  
  LINE(_,_,_,_,_,_,_,_), // 0x81 horz
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(X,X,X,X,X,X,X,X),
  LINE(X,X,X,X,X,X,X,X),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,_,_,_,_,_,_), // 0x82 top-right
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(X,X,X,X,X,_,_,_),
  LINE(X,X,X,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),

  LINE(_,_,_,X,X,_,_,_), // 0x83 vert
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  
  LINE(_,_,_,X,X,_,_,_), // 0x84 bot-right
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(X,X,X,X,X,_,_,_),
  LINE(X,X,X,X,X,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  
  LINE(_,_,_,X,X,_,_,_), // 0x85 bot-left 
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,X,X,X),
  LINE(_,_,_,X,X,X,X,X),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),

  LINE(_,_,_,X,X,_,_,_), // 0x86 T-left 
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,X,X,X),
  LINE(_,_,_,X,X,X,X,X),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),

  LINE(_,_,_,X,X,_,_,_), // 0x87 T-right 
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(X,X,X,X,X,_,_,_),
  LINE(X,X,X,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
  LINE(_,_,_,X,X,_,_,_),
/*
  LINE(_,_,_,_,_,_,_,_), // 
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
  LINE(_,_,_,_,_,_,_,_),
*/
};
