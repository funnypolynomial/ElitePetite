#ifndef _BTN_H
#define _BTN_H

class BTN
{
  public:
    void Init(int Pin);
    bool CheckButtonPress();
    bool IsDown();
    
  private:
    int m_iPin = 0;
    int m_iPrevReading = 0;
    int m_iPrevState = 0;
    unsigned long m_iTransitionTimeMS = 0UL;
};

extern BTN btn1Set;
extern BTN btn2Adj;

#endif
