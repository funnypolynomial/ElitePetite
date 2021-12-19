#pragma once
#include "Elite.h"

// Both variants use LCD shields so the only pins of interest are: 
//   SDA/SCL for an RTC, if present
//   SET/ADJ for push-buttons, if present 
// Both shields use D2-D9 for data and A0-A4 for control
// The small LCD uses A2+D8 for touch-x & A3+D9 for touch-y

#ifdef LCD_LARGE
// *** Large ***
#define PIN_BTN_SET A5
#define PIN_BTN_ADJ 10

#define PIN_RTC_SDA 11
#define PIN_RTC_SCL 12

#else
// *** Small ***
#define PIN_RTC_SDA 12
#define PIN_RTC_SCL 11

// no buttons (touch screen) 
#endif
