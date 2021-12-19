#include <Arduino.h>
#include "Text.h"
#include "Ship.h"
#include "Elite.h"
#include "Credits.h"
#include "Dials.h"
#include "Loader.h"
#include "Config.h"

namespace loader {
// forward decls
extern const word owlIcon[];
void DrawBackground(int x, int y, int w, int h);
void DrawMonoBitImage(const byte* pData, int y, int height, int insetBlocks);

void Init()
{
  // Animate the credits then prepare the screen
#ifdef ENABLE_CREDITS
  credits::Draw();
  delay(4000);
  dials::Draw(false);
  delay(1000);
#ifdef ENABLE_RAM_OVERFLOW  
  if (config::data.m_bAcornElectron)
  {
    // overflow into screen RAM with some arbitrary data
    DrawMonoBitImage((const byte*)ship::NameMultiStr_PGM, 0, 8, 0);
    delay(500);
  }
#endif  
#else
  dials::Draw(false);
#endif
  DrawBackground();
  DrawSpaceArea();
}

void DrawMonoBitImage(const byte* pData, int y, int height, int insetBlocks)
{
  // Load 1-bit-per-pixel screen-memory data
  // insetBlocks skips that many cols of 8 pixels from the left and right sides
  int widthBlocks = SCREEN_WIDTH / 8 - 2 * insetBlocks;
  LCD_BEGIN_FILL(SCREEN_OFFSET_X + 8 * insetBlocks, SCREEN_OFFSET_Y + y, widthBlocks * 8, height);
  for (int strip = 0; strip < height / 8; strip++)
  {
    for (int row = 0; row < 8; row++)
    {
      const byte* pByte = pData + row + insetBlocks * 8;
      for (int col = 0; col < widthBlocks; col++)
      {
        byte Byte = pgm_read_byte_near(pByte);
        if (Byte)
          for (int bit = 0; bit < 8; bit++)
            LCD_FILL_BYTE(1, (Byte & (0x80 >> bit)) ? 0xFF : 0x00);
        else
          LCD_FILL_BYTE(8, 0x00);
        pByte += 8;
      }
    }
    pData += 8 * 32;
  }
}

#ifdef ENABLE_GREEN_PALETTE
const word palette[4] = {RGB(0x00, 0x00, 0x00), RGB(0xFF, 0x00, 0x00), RGB(0xFF, 0xFF, 0x00), RGB(0x00, 0xFF, 0x00)}; // black, red, yellow, green
#else
const word palette[4] = {RGB(0x00, 0x00, 0x00), RGB(0xFF, 0x00, 0x00), RGB(0xFF, 0xFF, 0xFF), RGB(0x00, 0xFF, 0xFF)}; // black, red, white, cyan (escape capsule)
#endif

void DrawColourBitImage(const byte* pData, int y, int height, int insetBlocks)
{
  // Load 2-bits-per-pixel screen-memory data
  // insetBlocks skips that many cols of 8 pixels from the left AND right sides (used to re-draw the centre strip of the dials)
  int widthBlocks = SCREEN_WIDTH / 8 - 2 * insetBlocks;
  LCD_BEGIN_FILL(SCREEN_OFFSET_X + 8 * insetBlocks, SCREEN_OFFSET_Y + y, widthBlocks * 8, height);

  for (int strip = 0; strip < height / 8; strip++)
  {
    for (int row = 0; row < 8; row++)
    {
      const byte* pByte = pData + row + insetBlocks * 8;
      for (int col = 0; col < widthBlocks; col++)
      {
        byte Byte = pgm_read_byte_near(pByte);
        if (Byte)
          for (int pix = 0; pix < 4; pix++)
          {
            int idx = ((Byte >> (3 - pix)) & 0b00000001) | (((Byte >> (4 + (3 - pix))) & 0b00000001) << 1);
            LCD_FILL_COLOUR(2, palette[idx]);
          }
        else
          LCD_FILL_BYTE(8, 0x00);
        pByte += 8;
      }
    }
    pData += 8 * 32;
  }
}

void DrawBackground()
{
  // Background panels, left & right
  DrawBackground(0, 0, SCREEN_OFFSET_X, LCD_HEIGHT);
  DrawBackground(LCD_WIDTH - SCREEN_OFFSET_X, 0, SCREEN_OFFSET_X, LCD_HEIGHT);
  if (SCREEN_OFFSET_Y)
  {
    // top & bottom
    DrawBackground(SCREEN_OFFSET_X, 0, SCREEN_WIDTH, SCREEN_OFFSET_Y);
    DrawBackground(SCREEN_OFFSET_X, LCD_HEIGHT - SCREEN_OFFSET_Y, SCREEN_WIDTH, SCREEN_OFFSET_Y);
  }
}

void DrawSpaceArea()
{
  // Fills and draws the border
  LCD_FILL_BYTE(LCD_BEGIN_FILL(SCREEN_OFFSET_X, SCREEN_OFFSET_Y, SCREEN_WIDTH, SCREEN_HEIGHT - DIALS_HEIGHT), 0);
  LCD_FILL_BYTE(LCD_BEGIN_FILL(SCREEN_OFFSET_X, SCREEN_OFFSET_Y, SCREEN_WIDTH, 1), 0xFF); // 1 pixel line along top
  LCD_FILL_BYTE(LCD_BEGIN_FILL(SCREEN_OFFSET_X, SCREEN_OFFSET_Y, 2, SCREEN_HEIGHT - DIALS_HEIGHT), 0xFF);  // 2 pixel lines either side
  LCD_FILL_BYTE(LCD_BEGIN_FILL(LCD_WIDTH - SCREEN_OFFSET_X - 2, SCREEN_OFFSET_Y, 2, SCREEN_HEIGHT - DIALS_HEIGHT), 0xFF);  // 2 pixel lines either side
}

void DrawBackground(int x, int y, int w, int h)
{
  // Fill-in a panel around the Elite screen
  unsigned long count = LCD_BEGIN_FILL(x, y, w, h);
  byte scale;
  if (config::data.m_Background == loader::BeigeBackground) // Beige monitor
    LCD_FILL_COLOUR(count, RGB(0xF5, 0xF5, 0xDC));
  else if (config::data.m_Background == loader::ElkBackground)  // Electron-style pattern of squares
  {
    // best guess at the colours
    word fore = RGB(187, 184, 172);
    word back = RGB(0x20, 0x2B, 0x0B);
    scale = 8;
    for (int row = 0; row < h; row++)
    {
      if ((y+row) % scale)
        for (byte block = 0; block < w / scale; block++)
        {
          LCD_FILL_COLOUR(1, fore);
          LCD_FILL_COLOUR(scale - 1, back);
        }
      else
        LCD_FILL_COLOUR(w, fore);
    }
  }
  else if (config::data.m_Background == loader::BeebBackground) // the BBC Micro owl icon
  {
    // Draws 4x4 tiles with two Owls:
    //  O.
    //  .O
    // Owls have three extra blank rows and overlap by two columns to make the tile 32x48 pixels    
    // A little complicated by the need to draw the tiles in separate regions but have the pattern
    // be seamless across the LCD
    // The left (17th) column, omitted from owlIcon table to save space
    const word colData = 0b1010101010100000;
    for (int lcdY = y; lcdY < (y + h); lcdY++)
    {
      int row = lcdY % 24;
      word rowData = pgm_read_word_near(owlIcon + row);  // reading beyond array is OK
      for (int lcdX = x; lcdX < (x + w); lcdX++)
      {
        int col = lcdX % 32;
        bool on = false;
        if (row < 21)
        {
          if ((lcdY / 24) % 2)
            col -= 15;
          if (col)
            on = rowData & (0x10000UL >> col);
          else
            on = colData & (0x8000 >> row); 
        }
        if (on)
          LCD_ONE_WHITE();
        else
          LCD_ONE_BLACK();
      }
    }
  }
  else  // black
    LCD_FILL_BYTE(count, 0x00);

  if (config::data.m_Background == loader::StarsBackground) // stars
  {
    for (long star = 0; star < (w/8)*(h/10); star++)
    {
      int X = random(w);
      int Y = random(h);
      LCD_BEGIN_FILL(x + X, y + Y, 1, 1);
      LCD_ONE_WHITE();
    }
  }
}

// Square root of integer, https://en.wikipedia.org/wiki/Integer_square_root
uint32_t Sqrt(uint32_t s)
{
  uint32_t x0 = s >> 1;
  if (x0)
  {
    uint32_t x1 = ( x0 + s / x0 ) >> 1;
    while (x1 < x0)
    {
      x0 = x1;
      x1 = ( x0 + s / x0 ) >> 1;
    }
    return x0;
  }
  else
  {
    return s;
  }
}

// owl tiled as background
const word owlIcon[] PROGMEM =
{
  // 17 bits wide, so move the 17th out so we can use words
  // 0B's where there were 1's, 0b for 0
  0B0101010101010101,
  0b1000001010000010,
  0B0001000100010001,
  0b0010100000101000,
  0B0001000000010001,
  0b1000001010000010,
  0B0100000100000101,
  0b1010000000001000,
  0B0101010101010001,
  0b1010101000000000,
  0B0101010100000001,
  0b1010101000000000,
  0b0101010100000001,
  0b0010101010000000,
  0b0001010101000001,
  0b0000101010100000,
  0b0000010101010001,
  0b0000001000101000,
  0b0000010001000101,
  0b1010101010100010,
  0b0000000000000001,
};
}
