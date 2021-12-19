#include <Arduino.h>
#include "Text.h"
#include "Ship.h"
#include "Loader.h"
#include "Config.h"
#include "View.h"
#include "Elite.h"

namespace elite {

// multi-string with the lines we "type" at the command prompt
static const char commands[] PROGMEM =
 TEXT_MSTR("Acorn Electron \177")  TEXT_MSTR("BBC Computer")  // one or other line!
 TEXT_MSTR("\n")
 TEXT_MSTR("BASIC")
 TEXT_MSTR("\n")
 TEXT_MSTR(">CHAIN \"ELITE\"")
 TEXT_MSTR("Searching")
 TEXT_MSTR("\n")
 TEXT_MSTR("Loading")
 TEXT_MSTR("\n")
 TEXT_MSTR("ELITE")
;

void Commands()
{
  // animate "typing" at the command prompt
#ifdef ENABLE_COMMAND
  int lineN = 0;
  int textRow = 0;
  int strLen = 0;
  const char* pLine = text::StrN(commands, lineN);
  if (config::data.m_bAcornElectron)
    lineN++; // skip BBC line
  else
    pLine = text::StrN(commands, ++lineN);  // use BBC line
  while (pLine)
  {
    if (pgm_read_byte(pLine) != '\n')
      text::Draw(0, TEXT_SIZE + TEXT_SIZE*textRow, pLine);
    textRow++;
    strLen = text::StrLen(pLine);
    pLine = text::StrN(commands, ++lineN);
  }
  // blink the cursor at the end
  textRow--;
  int delayMS = 5000;
  const int blinkMS = 250;
  bool cursorOn = true;
  char cursorStr[2] = "_";
  while (delayMS > 0)
  {
    text::Draw(TEXT_SIZE*strLen, TEXT_SIZE + TEXT_SIZE*textRow, cursorStr, text::StdCharReader);
    cursorOn = !cursorOn;
    cursorStr[0] = cursorOn?'_':' ';
    delay(blinkMS);
    delayMS -= blinkMS;
  }
  LCD_FILL_BYTE(LCD_BEGIN_FILL(0, 0, LCD_WIDTH, LCD_HEIGHT), 0x00);
#endif  
}

void Init()
{
  config::Init();
  elite::Start();
}

void Loop()
{
  view::Loop();
}

void Start()
{
  // run the startup; commands, credits
#if defined(RTC_I2C_ADDRESS) && !defined(DEBUG)
  // seed the PRNG from time: hr, min, sec & day
  rtc.ReadTime(true);
  randomSeed(*(reinterpret_cast<unsigned long*>(&rtc.m_Hour24)));
#endif  
  LCD_FILL_BYTE(LCD_BEGIN_FILL(0, 0, LCD_WIDTH, LCD_HEIGHT), 0x00);
  Commands();
  loader::Init();
  view::Init();
}
}
