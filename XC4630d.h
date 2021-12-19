#pragma once
// Based on the hairball at https://www.jaycar.co.nz/rfid-keypad but that is only a .c file!

extern int XC4630_width,XC4630_height,XC4630_orientation;

//some colours with nice names
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0  
#define WHITE 0xFFFF
#define GREY 0x8410

//unit colours - multiply by 0-31
#define RED_1 0x0800
#define GREEN_1 0x0040 // 0-61
#define BLUE_1 0x0001

//touch calibration data=> raw values correspond to orientation 1
#define XC4630_TOUCHX0 920
#define XC4630_TOUCHY0 950
#define XC4630_TOUCHX1 120 //170
#define XC4630_TOUCHY1 160 //200

//defines for LCD pins, dependent on board
//For Mega Board
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
#define XC4630SETUP   "MEGA"
#define XC4630RSTOP   DDRF|= 16
#define XC4630RST0    PORTF&= 239
#define XC4630RST1    PORTF|= 16
#define XC4630CSOP    DDRF|=8
#define XC4630CS0     PORTF&=247
#define XC4630CS1     PORTF|=8
#define XC4630RSOP    DDRF|=4
#define XC4630RS0     PORTF&=251
#define XC4630RS1     PORTF|=4
#define XC4630WROP    DDRF|=2
#define XC4630WR0     PORTF&=253
#define XC4630WR1     PORTF|=2
#define XC4630RDOP    DDRF|=1
#define XC4630RD0     PORTF&=254
#define XC4630RD1     PORTF|=1
#define XC4630dataOP  DDRE=DDRE|56;DDRG=DDRG|32;DDRH=DDRH|120;
#define XC4630data(d) PORTE=(PORTE&199)|((d&12)<<2)|((d&32)>>2);PORTG=(PORTG&223)|((d&16)<<1);PORTH=(PORTH&135)|((d&192)>>3)|((d&3)<<5);
#endif

//For Leonardo Board
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega16U4__)
#define XC4630SETUP   "LEO"
#define XC4630RSTOP   DDRF |= 2
#define XC4630RST0    PORTF &= 253
#define XC4630RST1    PORTF |= 2
#define XC4630CSOP    DDRF |= 16
#define XC4630CS0     PORTF &= 239
#define XC4630CS1     PORTF |= 16
#define XC4630RSOP    DDRF |= 32
#define XC4630RS0     PORTF &= 223
#define XC4630RS1     PORTF |= 32
#define XC4630WROP    DDRF |= 64
#define XC4630WR0     PORTF &= 191
#define XC4630WR1     PORTF |= 64
#define XC4630RDOP    DDRF |= 128
#define XC4630RD0     PORTF &= 127
#define XC4630RD1     PORTF |= 128
#define XC4630dataOP  DDRB=DDRB|48;DDRC=DDRC|64;DDRD=DDRD|147;DDRE=DDRE|64;
#define XC4630data(d) PORTB=(PORTB&207)|((d&3)<<4);PORTC=(PORTC&191)|((d&32)<<1);PORTD=(PORTD&108)|((d&4)>>1)|((d&8)>>3)|((d&16))|((d&64)<<1);PORTE=(PORTE&191)|((d&128)>>1);
#endif

//for Uno board
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
#define XC4630SETUP   "UNO"
#define XC4630RSTOP   DDRC|=16
#define XC4630RST0    PORTC&=239
#define XC4630RST1    PORTC|=16
#define XC4630CSOP    DDRC|=8
#define XC4630CS0     PORTC&=247
#define XC4630CS1     PORTC|=8
#define XC4630RSOP    DDRC|=4
#define XC4630RS0     PORTC&=251
#define XC4630RS1     PORTC|=4
#define XC4630WROP    DDRC|=2
#define XC4630WR0     PORTC&=253
#define XC4630WR1     PORTC|=2
#define XC4630RDOP    DDRC|=1
#define XC4630RD0     PORTC&=254
#define XC4630RD1     PORTC|=1
#define XC4630dataOP  DDRD |= 252;DDRB |= 3;
#define XC4630data(d) PORTD=(PORTD&3)|(d&252); PORTB=(PORTB&252)|(d&3); 
#endif

//default that will work for any other board
#ifndef XC4630SETUP
#define XC4630SETUP   "DEFAULT"
#define XC4630RSTOP   pinMode(A4,OUTPUT)
#define XC4630RST0    digitalWrite(A4,LOW)
#define XC4630RST1    digitalWrite(A4,HIGH)
#define XC4630CSOP    pinMode(A3,OUTPUT)
#define XC4630CS0     digitalWrite(A3,LOW)
#define XC4630CS1     digitalWrite(A3,HIGH)
#define XC4630RSOP    pinMode(A2,OUTPUT)
#define XC4630RS0     digitalWrite(A2,LOW)
#define XC4630RS1     digitalWrite(A2,HIGH)
#define XC4630WROP    pinMode(A1,OUTPUT)
#define XC4630WR0     digitalWrite(A1,LOW)
#define XC4630WR1     digitalWrite(A1,HIGH)
#define XC4630RDOP    pinMode(A0,OUTPUT)
#define XC4630RD0     digitalWrite(A0,LOW)
#define XC4630RD1     digitalWrite(A0,HIGH)
#define XC4630dataOP  for(int i=2;i<10;i++){pinMode(i,OUTPUT);}
#define XC4630data(d) for(int i=2;i<10;i++){digitalWrite(i,d&(1<<(i%8)));}
#endif

 void XC4630_command(unsigned char d);
 void XC4630_data(unsigned char d);
 void XC4630_areaset(int x1,int y1,int x2,int y2);
 void XC4630_init();
 void XC4630_rotate(int n);
 void XC4630_charx(int x, int y, char c, unsigned int f, unsigned int b,byte s);
 void XC4630_charxa(int x,int y,char *c, unsigned int f, unsigned int b,byte s);
 void XC4630_char(int x,int y,char c, unsigned int f, unsigned int b);
 void XC4630_chars(int x,int y,char c,unsigned int f, unsigned int b);
 void XC4630_charsa(int x,int y,char *c, unsigned int f, unsigned int b);
 void XC4630_chara(int x,int y,char *c, unsigned int f, unsigned int b);
 void XC4630_box(int x1,int y1,int x2,int y2,unsigned int c);
 void XC4630_tbox(int x1,int y1, int x2,int y2, char *c, unsigned int f,unsigned int b,byte s);
 void XC4630_hline(int x1,int y1,int x2,unsigned int c);
 void XC4630_vline(int x1,int y1,int y2,unsigned int c);
 void XC4630_point(int x,int y, unsigned int c);
 void XC4630_fcircle(int xo,int yo,int r,unsigned int c);
 void XC4630_circle(int xo,int yo,int r,unsigned int c);
 void XC4630_line(int x1,int y1,int x2,int y2, unsigned int c);
 void XC4630_triangle(int x1,int y1,int x2,int y2,int x3,int y3,unsigned int c);
 void XC4630_clear(unsigned int c);

 int XC4630_touchrawx();
 int XC4630_touchrawy();
 int XC4630_touchx();
 int XC4630_touchy();
 int XC4630_istouch(int x1,int y1,int x2,int y2);
 void XC4630_mcimage(int x, int y, const byte img[],unsigned int fg, unsigned int bg);
 void XC4630_image(int x,int y,const byte img[]);
 void XC4630_imaget(int x,int y,const byte img[],unsigned int t);
 void XC4630_images(int x,int y,const byte img[],byte s);
 byte XC4630_imagewidth(const byte img[]);
 byte XC4630_imageheight(const byte img[]);
