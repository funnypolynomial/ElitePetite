#pragma once

// Talk to the DS3231/DS1307

// Undefine for no RTC attached
#define RTC_I2C_ADDRESS 0x68

class RTC
{
  public:
    RTC();
    void Setup();
    byte BCD2Dec(byte BCD);
    byte Dec2BCD(byte Dec);
    void ReadTime();
    byte ReadSecond();
    byte ReadMinute();
    void WriteTime();

    byte m_Hour24;      // 0..23
    byte m_Minute;      // 0..59
    byte m_Second;      // 0..59
    byte m_Unused;      // was day

    static bool CheckPeriod(unsigned long& Timer, unsigned long PeriodMS);
};

extern RTC rtc;
