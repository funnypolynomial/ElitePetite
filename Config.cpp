#include <Arduino.h>
#include <EEPROM.h>
#include "Elite.h"
#include "Dials.h"
#include "Ship.h"
#include "Pins.h"
#include "BTN.h"
#include "RTC.h"
#include "Text.h"
#include "View.h"
#include "Loader.h"
#include "Config.h"

// Load/Save configuration. Menu system to edit it

// NOTE: Configuring the time:
// If an RTC is present, some extra configuration items are available: 
//   * setting the ELITE title to be the time instead, 
//   * showing the time on a "ship"
//   * setting the time.
// Note that the latter is a little unusual, it uses menus and combines selecting the style 
// of time (12- or 24-hour mode) with setting the hour and minutes.  
// To keep the menu lists short this is split into FOUR levels
//   1: choose AM or PM and 12- or 24-hour
//   2: choose the hour
//   3: choose the 10's of the minutes
//   4: choose the 1's of the minutes

namespace config {
Data data;

template<typename Enum> void incrementEnum(Enum& e, Enum first, Enum last, bool exclusive = false)
{
  // increment enum value with wrap-around
  int temp = e;
  temp++;
  if ((temp >= last && exclusive) || temp > last)
    e = first;
  else
    e = static_cast<Enum>(temp);
}

void Save()
{
  // Save config to EEPROM
#ifndef RTC_I2C_ADDRESS
  data.m_bTimeTitle = false;
#endif
  int addr = 0;
  EEPROM.write(addr++, (byte)'E');  // signature
  EEPROM.write(addr++, data.m_bTimeTitle);
  EEPROM.write(addr++, data.m_bAcornElectron);
  EEPROM.write(addr++, data.m_Background);
  EEPROM.write(addr++, data.m_ShipType);
  EEPROM.write(addr++, data.m_b24HourTime);
  EEPROM.write(addr++, data.m_bShowLoadScreen);
}

void Load()
{
  // Load config from EEPROM
  int addr = 0;
  byte sig = EEPROM.read(addr++);
  if (sig != 'E')
  {
    // no config present, save one
    Save();
    return;
  }
  data.m_bTimeTitle       = EEPROM.read(addr++);
  data.m_bAcornElectron   = EEPROM.read(addr++);
  data.m_Background       = static_cast<loader::Background>(EEPROM.read(addr++));
  data.m_ShipType         = static_cast<ship::Type>(EEPROM.read(addr++));
  data.m_b24HourTime      = EEPROM.read(addr++);
  data.m_bShowLoadScreen  = EEPROM.read(addr++);
#ifndef RTC_I2C_ADDRESS
  data.m_bTimeTitle = false;
#endif
  if (data.m_ShipType > ship::LAST_SHIP)
    data.m_ShipType   = ship::CobraMk3;  
}

void Init()
{
  // Init config to defaults, then try loading from EEPROM
  data.m_bTimeTitle = false;
  data.m_bAcornElectron  = true;
  data.m_Background = loader::StarsBackground;
  data.m_ShipType   = ship::CobraMk3;
  data.m_b24HourTime = false;
  data.m_bShowLoadScreen = true;
  Load();
}


bool touchDown(int& x, int& y, bool& held)
{
  // True if the screen has been touched. held is true if > 2s
  // "debounced"
  int x1, y1;
  held = false;
  if (LCD_GET_TOUCH(x1, y1))
  {
    delay(10);
    if (LCD_GET_TOUCH(x, y)) // still touched?
    {
      if (abs(x - x1) < 5 && abs(y - y1) < 5) // same place?
      {
        unsigned long downTimeMS = millis();
        while ((held = LCD_GET_TOUCH(x1, y1)) && ((millis() - downTimeMS) < 2000))
          ;
        return true;
      }
    }
  }
  return false;
}

bool getTouch(int& x, int& y)
{
  // True if the screen has been touched.
  // "debounced"
  int x1, y1;
  if (LCD_GET_TOUCH(x1, y1))
  {
    delay(10);
    if (LCD_GET_TOUCH(x, y))
    {
      return (abs(x - x1) < 5 && abs(y - y1) < 5);
    }
  }
  return false;
}

char synthBuffer[8];
void I2A(int i, char*& pBuff, char leading)
{
  // convert i to 2-digit string at pBuff, using leading for the leading 0
  if (i < 10)
    *pBuff++ = leading;
  else
    *pBuff++ = '0' + i / 10;
  *pBuff++ = '0' + i % 10;
}

enum Context { contextNone = 0, contextHour12AM, contextHour12PM, contextHour24AM, contextHour24PM, contextMinuteTens, contextMinuteOnes};
const char* SynthMenuStr(int context, int idx)
{
  // Synthesize a menu item from the context and the index
  char* pBuff = synthBuffer;
  if (context == contextHour12AM || context == contextHour12PM)  // 1-12am, 1-12pm
  {
    if (idx >= 12)
      return NULL;
    I2A(idx ? idx : 12, pBuff, ' ');
    *pBuff++ = (context == contextHour12AM) ? 'A' : 'P';
    *pBuff++ = 'M';
  }
  else if (context == contextHour24AM || context == contextHour24PM)  // 00-11, 12-23
  {
    if (idx >= 12)
      return NULL;
    I2A(idx + ((context == contextHour24AM) ? 0 : 12), pBuff, '0');
  }
  else if (context == contextMinuteTens)  // 10's of minutes
  {
    if (idx >= 6)
      return NULL;
    I2A(idx, pBuff, '0');
    *synthBuffer = ':';
    *pBuff++ = 'x';
  }
  else if (context == contextMinuteOnes)  // 1's of minutes
  {
    if (idx >= 10)
      return NULL;
    *pBuff++ = ':';
    I2A(idx, pBuff, 'x');
  }
  *pBuff = 0;
  return synthBuffer;
}

void menuHzLine(int x, int y, int count, const char* start, const char* mid, const char* end)
{
  // Draw |-----| etc. start, count*mid, end
  text::Draw(x, y, start, text::StdCharReader);
  while (count--)
  {
    x += TEXT_SIZE;
    text::Draw(x, y, mid, text::StdCharReader);
  }
  x += TEXT_SIZE;
  text::Draw(x, y, end, text::StdCharReader);
}

size_t MaxStrLen(const char* pMultiStr)
{
  // widest string length in pgm multistring
  int idx = 0;
  size_t maxLen = 0;
  const char* pItem = text::StrN(pMultiStr, idx);
  while (pItem)
  {
    size_t ctrl = (text::ProgMemCharReader(pItem) < 32) ? 1 : 0; // ignore intial ctrl char
    maxLen = max(maxLen, strlen_P(pItem) - ctrl);
    pItem = text::StrN(pMultiStr, ++idx);
  }
  return maxLen;
}

bool PickFromMenu(const char* pMultiStr, int& X, int& Y, int& selection, int context = contextNone, text::CharReader reader = text::ProgMemCharReader)
{
  // Display the items in the multistring.
  // On entry highlights the selected item.  If true returns the new selection.
  // Handle Set and Sel buttons: Sel, cycles the highlight through the items.
  // Set pick the selected item and exits
  // Also handles screen touches, a touch on an item selects it.
  // A touch outside the menu returns false.
  // 30s of in-activity also returns false.
  bool redraw = true;
  bool waitup = true;
  bool first = true;
  int numItems = 0;
  size_t maxWidth = 0;
  bool blinkSelection = true;
  unsigned long idleTimeoutMS, blinkTimeoutMS;
  const char* pItem = pMultiStr;
  idleTimeoutMS = blinkTimeoutMS = millis();
  if (context)
    maxWidth = strlen(SynthMenuStr(context, 0));  // synthetics are all the same length
  else
    maxWidth = MaxStrLen(pMultiStr);
  int x, y;
  while (true)
  {
    if (redraw)
    {
      int idx = 0;
      if (context)
        pItem = SynthMenuStr(context, idx);
      else
        pItem = text::StrN(pMultiStr, idx);
      // top line
      menuHzLine(X, Y, maxWidth + 2, "\x80", "\x81", "\x82");
      while (pItem)
      {
        x = X;
        y = Y + (2 * idx + 1) * TEXT_SIZE;
        text::Draw(x, y, "\x83", text::StdCharReader);
        x += TEXT_SIZE;
        if (idx == selection && blinkSelection)
          text::Draw(x, y, ">", text::StdCharReader);
        else
          text::Draw(x, y, " ", text::StdCharReader);
        x += TEXT_SIZE;
        // colour the italicised "ElitePetite" item
        word colour = (reader(pItem) == '\xF')?loader::palette[3]:RGB(0xFF, 0xFF, 0xFF);
        text::Draw(x, y, pItem, reader, colour);
        size_t len = text::StrLen(pItem, reader);
        x += len * TEXT_SIZE;
        while (len < (maxWidth + 1))
        {
          len++;
          text::Draw(x, y, " ", text::StdCharReader);
          x += TEXT_SIZE;
        }
        text::Draw(x, y, "\x83", text::StdCharReader);
        if (context)
          pItem = SynthMenuStr(context, ++idx);
        else
          pItem = text::StrN(pMultiStr, ++idx);
        if (pItem)
        {
          if (selection != -1)
            menuHzLine(X, y + TEXT_SIZE, maxWidth + 2, "\x86", "\x81", "\x87");
          else
            menuHzLine(X, y + TEXT_SIZE, maxWidth + 2, "\x83", " ", "\x83");
        }
      }
      // bottom line
      menuHzLine(X, Y + 2 * idx * TEXT_SIZE, maxWidth + 2, "\x85", "\x81", "\x84");
      numItems = idx;
      if (waitup)
      {
        while (LCD_GET_TOUCH(x, y))
          ;
        if (!first)
        {
          // postition sub-menu
          X += TEXT_SIZE * (maxWidth + 2);
          Y += TEXT_SIZE;
          return true;
        }
      }
      first = waitup = redraw = false;
    }

    if (btn1Set.CheckButtonPress())
    {
      // postition sub-menu
      X += TEXT_SIZE * (maxWidth + 2);
      Y += TEXT_SIZE;
      return true;
    }
    else if (btn2Adj.CheckButtonPress())
    {
      if (selection < 0)
        return true;
      selection++;
      if (selection >= numItems)
        selection = 0;
      idleTimeoutMS = millis();
      redraw = true;
    }
    else if (getTouch(x, y))
    {
      if (X < x && x < (X + (3 + (int)maxWidth)*TEXT_SIZE))
      {
        y = (y - Y - TEXT_SIZE / 2) / (TEXT_SIZE * 2);
        if (0 <= y && y < numItems)
        {
          if (selection >= 0)
            selection = y;
          blinkSelection = waitup = redraw = true;
          continue;
        }
      }
      break;
    }
    else
    {
      if (RTC::CheckPeriod(idleTimeoutMS, 30000))
      {
        break;
      }
      if (RTC::CheckPeriod(blinkTimeoutMS, 500))
      {
        blinkSelection = !blinkSelection;
        redraw = true;
      }
    }
  }
  return false;
}


// enum of top-level menu items, some are optionally omitted.
enum ConfigurationMenu
{
  menuShip, 
#ifdef ENABLE_STATUS_SCREEN
  menuScreen, 
#endif
  menuMachine, 
  menuBkGnd, 
#ifdef RTC_I2C_ADDRESS
// RTC present, so are "Title" & "Time"
  menuTitle, menuTime, 
#endif
  menuStart, menuAbout, menuExit
};

// multi-string of the top level menu
const char ConfigurationMenuStr[] PROGMEM = TEXT_MSTR("Ship")
#ifdef ENABLE_STATUS_SCREEN
  TEXT_MSTR("Screen") 
#endif
  TEXT_MSTR("Machine") 
  TEXT_MSTR("Background") 
#ifdef RTC_I2C_ADDRESS
// RTC present, so are "Title" & "Time"
  TEXT_MSTR("Title") TEXT_MSTR("Time")
#endif  
  TEXT_MSTR("Restart") TEXT_MSTR("About") TEXT_MSTR("Exit");

// multi-strings of sub-menus
const char mchMenuStr[]   PROGMEM = TEXT_MSTR("BBC Micro") TEXT_MSTR("Acorn Electron");
const char scrnMenuStr[]  PROGMEM = TEXT_MSTR("Status") TEXT_MSTR("Load");
const char bkgndMenuStr[] PROGMEM = TEXT_MSTR("Black") TEXT_MSTR("BBC") TEXT_MSTR("Electron") TEXT_MSTR("Beige") TEXT_MSTR("Stars");
const char titleMenuStr[] PROGMEM = TEXT_MSTR("Elite") TEXT_MSTR("Time");
const char timeMenuStr[]  PROGMEM = TEXT_MSTR("12hAM") TEXT_MSTR("12hPM") TEXT_MSTR("24hAM") TEXT_MSTR("24hPM");
const char aboutMenuStr[] PROGMEM = TEXT_MSTR("\017ElitePetite")  // italic!
                                    TEXT_MSTR("Mark Wilson")
                                    TEXT_MSTR("Fecit MMXXI");

void ConfigurationMenu()
{
  // drive the config menu
  int selection = 0;
  const int orgX = SCREEN_OFFSET_X + TEXT_SIZE/2;
  int X = orgX;
  int Y = SCREEN_OFFSET_Y + TEXT_SIZE;
  bool save = true;
  if (PickFromMenu(ConfigurationMenuStr, X, Y, selection))
  {
    if (selection == menuShip)  // Ship
    {
      selection = data.m_ShipType;
#ifdef RTC_I2C_ADDRESS
      Y = SCREEN_OFFSET_Y + TEXT_SIZE/2;  // Clock item makes it too long to drop
#endif      
      if (PickFromMenu(ship::NameMultiStr_PGM, X, Y, selection))
      {
        data.m_ShipType = static_cast<ship::Type>(selection);
        view::LoadShip(false, data.m_ShipType == ship::LAST_SHIP);
      }
    }
#ifdef ENABLE_STATUS_SCREEN
    else if (selection == menuScreen)  // screen
    {
      selection = data.m_bShowLoadScreen;
      if (PickFromMenu(scrnMenuStr, X, Y, selection))
      {
        data.m_bShowLoadScreen = selection;
      }
    }
#endif    
    else if (selection == menuMachine)  // machine
    {
      selection = data.m_bAcornElectron;
      if (PickFromMenu(mchMenuStr, X, Y, selection))
      {
        data.m_bAcornElectron = selection;
        dials::Draw(true);
      }
    }
    else if (selection == menuBkGnd)  // Background
    {
      selection = data.m_Background;
      if (PickFromMenu(bkgndMenuStr, X, Y, selection))
      {
        data.m_Background = static_cast<loader::Background>(selection);
        loader::DrawBackground();
      }
    }
#ifdef RTC_I2C_ADDRESS
    else if (selection == menuTitle)  // Title
    {
      selection = data.m_bTimeTitle;
      if (PickFromMenu(titleMenuStr, X, Y, selection))
      {
        data.m_bTimeTitle = selection;
      }
    }
    else if (selection == menuTime) // Time
    {
      // Sets the time and selects the display style 12 or 24-hour mode
      // Multiple menus so they fit, but they're wide, so pull it left
      X = orgX + 6*TEXT_SIZE;
      selection = 2 * data.m_b24HourTime;
      rtc.ReadTime();
      if (rtc.m_Hour24 >= 12)
        selection++;
      if (PickFromMenu(timeMenuStr, X, Y, selection))  //  12/24 am/pm
      {
        int context = selection + 1;
        int hour = (selection == 1 || selection == 3) ? 12 : 0;
        selection = rtc.m_Hour24 % 12;  // 0..11
        if (PickFromMenu(NULL, X, Y, selection, context, text::StdCharReader))  // hour
        {
          hour += selection;
          selection = rtc.m_Minute / 10;
          if (PickFromMenu(NULL, X, Y, selection, contextMinuteTens, text::StdCharReader)) // minute 10's
          {
            int minute = 10 * selection;
            selection = rtc.m_Minute % 10;
            if (PickFromMenu(NULL, X, Y, selection, contextMinuteOnes, text::StdCharReader)) // minute 1's
            {
              // set the time
              minute += selection;
              rtc.m_Hour24 = hour;
              rtc.m_Minute = minute;
              rtc.m_Second = 0;
              rtc.WriteTime();
              data.m_b24HourTime = context > 2;
            }
          }
        }
      }
    }
#endif    
    else if (selection == menuStart)
    {
      elite::Start();
      return;
    }
    else if (selection == menuAbout)
    {
      selection = -1; // a dummy "menu"
      PickFromMenu(aboutMenuStr, X, Y, selection);
      save = false;
    }
    else if (selection == menuExit)
    {
      save = false;
    }
    if (save)
      Save();
  }
  loader::DrawSpaceArea();
  view::DrawTextLines();
  dials::Draw(true);
}

void Loop()
{
  // check for input, touches or buttons
  int x, y;
  bool held;
  if (btn1Set.CheckButtonPress())
  {
    ConfigurationMenu();
  }
  else if (btn2Adj.CheckButtonPress() && data.m_bShowLoadScreen)
  {
    // next ship
    incrementEnum(data.m_ShipType, ship::Adder, ship::LAST_SHIP, true); // exclusive of last ship
    view::LoadShip(false, true);
    Save();
  }
  else if (touchDown(x, y, held))
  {
    int textX, eliteY, loadY;
    text::CharReader charReader;
    view::GetTextLine(0, textX, eliteY, charReader);
    eliteY += TEXT_SIZE;
    view::GetTextLine(1, textX, loadY, charReader);

    if (held)
    {
      if (x > SCREEN_OFFSET_X && x < (SCREEN_OFFSET_X + SCREEN_WIDTH) && y > eliteY && y < loadY)
        view::ManualMode(x, y);
      else
        ConfigurationMenu();
      return;
    }

    // hit tests:
    if (x < SCREEN_OFFSET_X || x > (SCREEN_OFFSET_X + SCREEN_WIDTH))
    {
      // background
      incrementEnum(data.m_Background, loader::BlackBackground, loader::StarsBackground);
      Save();
      loader::DrawBackground();
    }
    else if (y > LCD_HEIGHT - DIALS_HEIGHT)
    {
      // dials
      data.m_bAcornElectron = !data.m_bAcornElectron;
      dials::Draw(true);
      Save();
    }
    else if (!data.m_bShowLoadScreen)
    {
      if (y < eliteY)
      {
        // title
        data.m_bTimeTitle = !data.m_bTimeTitle;
        Save();
      }
      else
      // status text=menu
        ConfigurationMenu();
    }
    else if (y > eliteY)
    {
      if (y < loadY)
      {
        // ship
        incrementEnum(data.m_ShipType, ship::Adder, ship::LAST_SHIP, true);
        view::LoadShip(false, true);
        Save();
      }
      else
      {
        // load text=menu
        ConfigurationMenu();
      }
    }
    else if (y < eliteY)
    {
      // title
      data.m_bTimeTitle = !data.m_bTimeTitle;
      view::DrawTextLines();
      Save();
    }
  }
}
}
