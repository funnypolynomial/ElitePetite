#pragma once

// Configuration settings
namespace config
{
  struct Data
  {
    bool m_bTimeTitle;                // H H : M M vs E L I T E
    bool m_b24HourTime;               // 21:00 vs  9:00
    bool m_bAcornElectron;            // Monochrome dials vs 4-colour
    loader::Background m_Background;  // Stars etc
    ship::Type m_ShipType;            // Cobra etc
    bool m_bShowLoadScreen;           // Ship & Load New Commander vs COMMANDER & Present System etc
    
    // Not stored in EEPROM:
    const unsigned long minimumFramePeriodMS = 0; // vs, for example 100ms for evened-out 10Hz minimum
  };
  
  void Init();
  void Loop();
  
  extern Data data;
  void I2A(int i, char*& pBuff, char leading);
};
