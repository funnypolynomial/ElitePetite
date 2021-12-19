#include "arduino.h"
#include <avr/pgmspace.h>
#include "ILI948x.h"

#ifdef LCD_PORTRAIT
#ifdef LCD_PORTRAIT_TOP
#define MADCTL0x36 B00011000
#else
#define MADCTL0x36 B00001000
#endif
#elif defined LCD_LANDSCAPE_LEFT
#define MADCTL0x36 B10101000
#else
#define MADCTL0x36 B01101000
#endif   
//                  ||||||||  
//                  | B7 Page Address Order
//                   | B6 Column Address Order
//                    | B5 Page/Column Order
//                     | B4 Line Address Order
//                      | B3 RGB/BGR Order
//                       | B2 Display Data Latch Data Order
//                        | B1 Reserved Set to 0
//                         | B0 Reserved Set to 0
byte ILI948x::m_MADCTL0x36 = 0;

#ifdef SERIALIZE
bool ILI948x::_serialise = true;
#endif

const byte PROGMEM ILI948x::initialisation[]  = 
{
// len, cmd,   data...
// 255, sleep MS
     3, 0xC0,  0x19, 0x1A, // Power Control 1 "VREG1OUT +ve VREG2OUT -ve"
     3, 0xC1,  0x45, 0x00, // Power Control 2 "VGH,VGL    VGH>=14V."
     2, 0xC2,  0x33,       // Power Control 3
     3, 0xC5,  0x00, 0x28, // VCOM Control "VCM_REG[7:0]. <=0X80."
     3, 0xB1,  0x90, 0x11, // Frame Rate Control "OSC Freq set. 0xA0=62HZ,0XB0 =70HZ, <=0XB0."
     2, 0xB4,  0x02,       // Display Inversion Control "2 DOT FRAME MODE,F<=70HZ."
     4, 0xB6,  0x00, 0x42, 0x3B, // Display Function Control "0 GS SS SM ISC[3:0];"
     2, 0xB7,  0x07,       // Entry Mode Set
     // Positive Gamma Control:
    16, 0xE0,  0x1F, 0x25, 0x22, 0x0B, 0x06, 0x0A, 0x4E, 0xC6, 0x39, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     // Negative Gamma Control:
    16, 0xE1,  0x1F, 0x3F, 0x3F, 0x0F, 0x1F, 0x0F, 0x46, 0x49, 0x31, 0x05, 0x09, 0x03, 0x1C, 0x1A, 0x00,
     2, 0x36,  MADCTL0x36, // Memory Access Control BGR (row,col,exch)
     2, 0x3A,  0x55,       // Interface Pixel Format = 16 bits
     1, 0x11,              // Sleep OUT
   255, 120,               // delay
     //1, 0x29,              // Display ON, no, wait until we're ready
     0  // END
};

void ILI948x::Init()
{
  for(int p=2;p<10;p++)
  {
    pinMode(p,OUTPUT);
  }
  // An registres to OUTPUT, HIGH:
  DDRC = 0b00011111;
  PORTC =  0b00011111;

  digitalWrite(LCD_RST_PIN,HIGH);
  delay(50); 
  digitalWrite(LCD_RST_PIN,LOW);
  delay(150);
  digitalWrite(LCD_RST_PIN,HIGH);
  delay(150);

  digitalWrite(LCD_CS_PIN,HIGH);
  digitalWrite(LCD_WR_PIN,HIGH);
  digitalWrite(LCD_CS_PIN,LOW);

  const byte* init = initialisation;
  do
  {
    byte len = pgm_read_byte(init++);
    if (len == 255)
    {
      delay(pgm_read_byte(init++));
    }
    else if (len)
    {
      Cmd(pgm_read_byte(init++));
      while (--len)
        DataByte(pgm_read_byte(init++));
    }
    else
    {
      break;
    }
    
  } while (true);

  m_MADCTL0x36 = MADCTL0x36;
  SERIALISE_COMMENT("*** START")
  SERIALISE_INIT(LCD_WIDTH, LCD_HEIGHT, 1);
}

void ILI948x::DisplayOn()
{
   CMD(0x29);
}

void ILI948x::Cmd(byte cmd)
{
  PORTD = cmd & B11111100; 
  PORTB = LCD_OR_PORTB | (cmd & B00000011); 
  PORTC = LCD_OR_PORTC | LCD_RST_BIT | LCD_RD_BIT;
  PORTC |= LCD_WR_BIT;
}

void ILI948x::DataByte(byte data)
{
  PORTD = data & B11111100; 
  PORTB = LCD_OR_PORTB | (data & B00000011); 
  PORTC = LCD_OR_PORTC | LCD_RST_BIT | LCD_RD_BIT | LCD_RS_BIT;
  PORTC |= LCD_WR_BIT;
}

void ILI948x::DataWord(word data)
{
  DATA((byte)(data >> 8));
  DATA((byte)(data));
}

void ILI948x::ColourByte(byte colour, unsigned long count)
{
  SERIALISE_FILLBYTE(count, colour);
  if (count)
  {
    PORTD = colour;
    PORTB = LCD_OR_PORTB | colour;
    PORTC = LCD_OR_PORTC | LCD_RST_BIT | LCD_RD_BIT | LCD_RS_BIT;
    while (count--)
    {
      PINC = LCD_WR_BIT;
      PINC = LCD_WR_BIT;
      PINC = LCD_WR_BIT;
      PINC = LCD_WR_BIT;
    }
  }
}

void ILI948x::ColourWord(word colour, unsigned long count)
{
  SERIALISE_FILLCOLOUR(count, colour);
  byte hi = colour >> 8;
  while (count--)
  {
    DATA(hi);
    DATA(colour);
  }
}

void ILI948x::OneWhite()
{
  SERIALISE_FILLBYTE(1, 0xFF);
  PORTD = PORTB = 0xFF;
  PORTC = LCD_OR_PORTC | LCD_RST_BIT | LCD_RD_BIT | LCD_RS_BIT;
  PINC = LCD_WR_BIT;
  PINC = LCD_WR_BIT;
  PINC = LCD_WR_BIT;
  PINC = LCD_WR_BIT;
  
}

void ILI948x::OneBlack()
{
  SERIALISE_FILLBYTE(1, 0x00);
  PORTD = 0x00;
  PORTB = LCD_OR_PORTB;
  PORTC = LCD_OR_PORTC | LCD_RST_BIT | LCD_RD_BIT | LCD_RS_BIT;
  PINC = LCD_WR_BIT;
  PINC = LCD_WR_BIT;
  PINC = LCD_WR_BIT;
  PINC = LCD_WR_BIT;
}

unsigned long ILI948x::Window(word x,word y,word w,word h)
{
  SERIALISE_BEGINFILL(x, y, w, h);

  word x2 = x + w - 1;
  word y2 = y + h - 1;
  CMD(0x2A);
  DataWord(x);
  DataWord(x2);
  
  CMD(0x2B);
  DataWord(y);
  DataWord(y2);
  
  CMD(0x2C);

  unsigned long count = w;
  count *= h;
  return count;  
}

void ILI948x::ClearByte(byte colour)
{
  unsigned long count = Window(0, 0, LCD_WIDTH, LCD_HEIGHT);
  SERIALISE_FILLBYTE(count, colour);

  PORTD = colour & B11111100; 
  PORTB = LCD_OR_PORTB | (colour & B00000011); 
  while (count--)
  {
    PORTC = LCD_OR_PORTC | LCD_RST_BIT | LCD_RD_BIT | LCD_RS_BIT;
    PORTC |= LCD_WR_BIT;
    PORTC = LCD_OR_PORTC | LCD_RST_BIT | LCD_RD_BIT | LCD_RS_BIT;
    PORTC |= LCD_WR_BIT;
  }
}

void ILI948x::ClearWord(word colour)
{
  unsigned long count = Window(0, 0, LCD_WIDTH, LCD_HEIGHT);
  SERIALISE_FILLCOLOUR(count, colour);
  for (unsigned long pixel = 0; pixel < count; pixel++)
  {
    DataWord(colour);
  }
}

void ILI948x::SetScrollLeft(bool left)
{
  ILI948x::Cmd(0x36);
  if (left)
    ILI948x::DataByte(ILI948x::m_MADCTL0x36 | B00010000); // B4=Vertical Refresh Order
  else
    ILI948x::DataByte(ILI948x::m_MADCTL0x36 & ~B00010000); // B4=Vertical Refresh Order
}
