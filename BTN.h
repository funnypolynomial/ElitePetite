#pragma once

class BTN
{
  public:
    void Init(int Pin);
    bool CheckButtonPress();
    bool IsDown();
    
  private:
    byte m_iPin = 0;
    byte m_iPrevReading = 0;
    byte m_iPrevState = 0;
    unsigned long m_iTransitionTimeMS = 0UL;
};

extern BTN btn1Set;
extern BTN btn2Adj;
