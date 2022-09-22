#pragma once

// Global declarations & defines

// The LCD interface: 
// Initialise/define a window/fill it with colour.
// Done with macros, LCD_BEGIN_FILL, LCD_FILL_COLOUR etc
//#define LCD_LARGE
#ifdef LCD_LARGE
#include "Elite_Large.h"
#else
#include "Elite_Small.h"
#endif

// Lower dials part
#define DIALS_WIDTH SCREEN_WIDTH
#define DIALS_HEIGHT 56

// Upper space part
#define SPACE_WIDTH SCREEN_WIDTH
#define SPACE_HEIGHT (SCREEN_HEIGHT - DIALS_HEIGHT)

// The area the ship rotates within
#define SHIP_WINDOW_SIZE (SCREEN_HEIGHT - DIALS_HEIGHT - 2*TEXT_SIZE)
#define SHIP_WINDOW_ORIGIN_X ((LCD_WIDTH - SHIP_WINDOW_SIZE)/2)
#define SHIP_WINDOW_ORIGIN_Y (SCREEN_OFFSET_Y + TEXT_SIZE)

// Customisation (see also RTC_I2C_ADDRESS)
#define ENABLE_COMMAND          // Show CHAIN command screen
#define ENABLE_CREDITS          // Show Saturn screen
#define ENABLE_APPROACH         // Show ship approach animation
#define ENABLE_RANDOM_ROTATION  // Randomize pitch/roll rates
#define ENABLE_SPARSE_WIDE      // Full width single sparse render
#define ENABLE_STD_CLOCK_DIGITS // Clock ship has generous digits
#define ENABLE_STATUS_SCREEN    // Status text screen as alternative to ship
#define ENABLE_GREEN_PALETTE    // Red, Green & Yellow dials vs Red, White & Cyan
#define ENABLE_RAM_OVERFLOW     // Program load overflows into video RAM (Electron)

// Debugging, Serial, stats etc
//#define DEBUG_STACK_CHECK          // Check that there's enough stack. Reported in Title
//#define DEBUG
#ifdef DEBUG
// Dump variable to serial
#define DBG(_x) { Serial.print(#_x);Serial.print(":");Serial.println(_x); }
// start up faster:
#undef ENABLE_COMMAND
#undef ENABLE_CREDITS
#undef ENABLE_APPROACH
#else
#define DBG(_x)
#endif


// Main app
namespace elite
{
  void Init();
  void Loop();
  void Start();
};
