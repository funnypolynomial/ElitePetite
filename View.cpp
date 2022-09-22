#include <Arduino.h>
#include "RTC.h"
#include "BTN.h"
#include "Elite.h"
#include "Dials.h"
#include "Text.h"
#include "Loader.h"
#include "Sparse.h"
#include "Ship.h"
#include "Config.h"
#include "View.h"

#ifdef DEBUG
unsigned long sumLinesMS = 0;
unsigned long sumPaintMS = 0;
#endif

#ifdef DEBUG_STACK_CHECK
extern uint16_t stackHeadroom;
#endif

// see https://www.bbcelite.com/deep_dives/drawing_ships.html & https://www.bbcelite.com/electron/main/subroutine/title.html
namespace view {
/*
  It's all about drawing the ship, it's the reason we are here at all.
  The ship blueprints make this efficient.  
  The view is a simple projection onto a plane (discard the Z component) whereas Elite does perspective.
  For a given pitch and roll (there is no yaw):
    each 3D face normal is rotated in 3D.  If it's Z is +ve then the face is towards us and visible
    each 3D vertex which is associated with a visible face is rotated in 3D and the corresponding 2D point X & Y is stored
    each edge associated with a visible face is drawn as a 2D line between 2D vertices from above
    the line drawing is done into the sparse representation
    the sparse representation is painted to the LCD, combined with rasterized text to create the XOR effect

  The 3D rotations are performed with integer calculations, basically fractions.
  The ship view is just (re)painted in it's entirety, whereas Elite XOR'd lines to draw them and XOR'd them again to erase them.
*/
const int VIEW_MAX_VERTICES = 28; // Cobra has 28, Transporter has 38 (but is excluded)!
const int VIEW_MAX_FACES = 15;    // Adder has 15. 16 is the upper limit since we're using a 16-bit mask

static const char pElite[] PROGMEM  = "---- E L I T E ----";
static const char pLoad[]  PROGMEM  = "Load New Commander (Y/N)";
static const char pAcorn[] PROGMEM  = "(C) Acornsoft 1984";
static const char pManual[] PROGMEM = "   Drag to pitch & roll ";
char pTimeTitle[21];  // "---- H H : M M ----";

bool manualMode = false;  // manually pitching & rolling

void GetTimeStr(char* Buffer)
{
  // puts HHMM into buffer, 12 or 24-hour format
  rtc.ReadTime();
  if (config::data.m_b24HourTime)
  {
    Buffer[0] = '0' + (rtc.m_Hour24 / 10);
    Buffer[1] = '0' + (rtc.m_Hour24 % 10);
  }
  else
  {
    int hr = rtc.m_Hour24 % 12;
    if (hr == 0)
      hr = 12;
    if (hr >= 10)
      Buffer[0] = '0' + (hr / 10);
    else
      Buffer[0] = ' ';
    Buffer[1] = '0' + (hr % 10);
  }
  Buffer[2] = '0' + (rtc.m_Minute / 10);
  Buffer[3] = '0' + (rtc.m_Minute % 10);
}

const char* GetTextLine(int item, int& x, int& y, text::CharReader& charReader)
{
  // Get the item'th line of text and its LCD coords, item=0/1/2 for ELITE/Load/Acorn
  charReader = text::ProgMemCharReader;
  if (item == 0)
  {
    // ELITE is on the second line
    x = SCREEN_OFFSET_X + TEXT_SIZE * 6;
    y = SCREEN_OFFSET_Y + TEXT_SIZE * 1;
#ifdef DEBUG_STACK_CHECK
    // report stack headroom in score
    char* pStr2 = pTimeTitle;
    config::I2A(stackHeadroom / 100, pStr2, ' ');
    config::I2A(stackHeadroom % 100, pStr2, '0');
    *pStr2 = 0;
    charReader = text::StdCharReader;
    return pTimeTitle;
#else
    if (config::data.m_bTimeTitle)
    {
      // the title is ---- H H : M M ----
      strcpy_P(pTimeTitle, pElite);
      char strTime[4];
      GetTimeStr(strTime);
      pTimeTitle[5] = strTime[0];
      pTimeTitle[7] = strTime[1];
      pTimeTitle[9] = ':';
      pTimeTitle[11] = strTime[2];
      pTimeTitle[13] = strTime[3];
      charReader = text::StdCharReader;
      return pTimeTitle;
    }
    return pElite;
#endif
  }
  else if (item == 1)
  {
    // Load is three lines up from the bottom of the space area
    x = SCREEN_OFFSET_X + TEXT_SIZE * 3;
    y = SCREEN_OFFSET_Y + SPACE_HEIGHT - TEXT_SIZE * 3;
    if (manualMode)
      return pManual; // instructions
    else
      return pLoad;
  }
  else
  {
    // Acorn is the bottom line of the space area
    x = SCREEN_OFFSET_X + TEXT_SIZE * 7;
    y = SCREEN_OFFSET_Y + SPACE_HEIGHT - TEXT_SIZE * 1;
    return pAcorn;
  }
}

void DrawTextLines()
{
  // draws the 3 pieces of text
  if (config::data.m_bShowLoadScreen)  
    for (int item = 0; item < 3; item++)
    {
      int x, y;
      text::CharReader charReader;
      const char* pStr = GetTextLine(item, x, y, charReader);
      text::Draw(x, y, pStr, charReader);
    }
}

// The ship's orientation
int16_t rollDegrees = 0, pitchDegrees = 0;

// Max rate of rotation
const int16_t maxStepDegrees = +4;
// Current rates of rotation
int16_t rollStepDegrees = +1, pitchStepDegrees = +3;
// Scale factor to maximize fit of ship's MBR within window, encroaching only on "ELITE" & "Load"
int16_t maxShipScale = 64;
// Current scale factor, less than max when approaching
int16_t currentShipScale = 64;
// Flash the name of the ship?
bool labelShip = false;
// The current ship
ship::Details currentShip;
ship::Type currentShipType = ship::CobraMk3;
int16_t frameCount = 0;

int16_t randomStepDegrees()
{
  // returns a number -maxStepDegrees ... +maxStepDegrees (excluding 0)
  int16_t stepDegrees = maxStepDegrees - random(2*maxStepDegrees);
  if (stepDegrees <= 0)
    stepDegrees--;
  return stepDegrees;
}

struct Face
{
  int16_t normal_x, normal_y, normal_z;
};

Face viewFaces[VIEW_MAX_FACES];

void Subtract(ship::Vertex& v1, const ship::Vertex& v2)
{
  // v1 = v1-v2
  v1.x -= v2.x;
  v1.y -= v2.y;
  v1.z -= v2.z;
}

void CrossProduct(Face& n, ship::Vertex& v1, const ship::Vertex& v2)
{
  // n = v1 x v2
  n.normal_x = v1.y * v2.z - v1.z * v2.y;
  n.normal_y = v1.z * v2.x - v1.x * v2.z;
  n.normal_z = v1.x * v2.y - v1.y * v2.x;
}

void SignOf(Face& n, const ship::Face& f)
{
  // f1 components with the same SIGNS as f2
  n.normal_x = abs(n.normal_x) * f.normal_x;
  n.normal_y = abs(n.normal_y) * f.normal_y;
  n.normal_z = abs(n.normal_z) * f.normal_z;
}

void computeNewNormals()
{
  // The original face normals don't work for me.
  // I see glitches -- edges drawn when they shouldn't be and vice-versa.
  // I can only guess that this is somehow caused by the normal data being suited to
  // Elite's 8-bit calculations, whereas my calcs use more bits, follow a different path and compute
  // different values.
  // To solve that, this routine computes new normals from the cross-product of pairs of edges on each triangles.
  // To correct the sense, it compares the signs of the original normal with the one computed, and flips it if required.
  const ship::Face* pFace = currentShip.faces;
  for (size_t faceIdx = 0; faceIdx < currentShip.numFaces; faceIdx++, pFace++)
  {
    // for each face, find the first three unique vertices, via the edges
    byte a = 0xFF, b = 0, c = 0;  // indices
    ship::Face face;
    memcpy_P(&face, pFace, sizeof(ship::Face));
    const ship::Edge* pEdge = currentShip.edges;
    for (size_t edgeIdx = 0; edgeIdx < currentShip.numEdges; edgeIdx++, pEdge++)
    {
      ship::Edge edge;
      memcpy_P(&edge, pEdge, sizeof(ship::Edge));
      if (edge.face1 == faceIdx || edge.face2 == faceIdx)
      {
        if (a == 0xFF)
        {
          a = edge.vertex1;
          b = edge.vertex2;
        }
        else if (a != edge.vertex1 && b != edge.vertex1)
        {
          c = edge.vertex1;
          break;
        }
        else if (a != edge.vertex2 && b != edge.vertex2)
        {
          c = edge.vertex2;
          break;
        }
      }
    }

    ship::Vertex A, B, C;
    memcpy_P(&A, currentShip.vertices + a, sizeof(ship::Vertex));
    memcpy_P(&B, currentShip.vertices + b, sizeof(ship::Vertex));
    memcpy_P(&C, currentShip.vertices + c, sizeof(ship::Vertex));
    // compute the cross-product of the two edges to get a new normal to the face
    Face newNormal;
    Subtract(B, A);
    Subtract(C, A);
    // (B-A)x(C-A)
    CrossProduct(newNormal, B, C);
    // make the sense the same as the blueprint normal
    SignOf(newNormal, face);
    // store it
    viewFaces[faceIdx] = newNormal;
  }
}

// sin(a) and cos(b) are:
//  * for integer degrees, a=0...359
//  * represented as fractions, the numerator over 2^TRIG_FRACTION_BITS
#define TRIG_FRACTION_BITS 8

#if TRIG_FRACTION_BITS >= 7
typedef int16_t tTrigFraction;
#else
typedef int8_t tTrigFraction; // 6 or less fits in a char
#endif

#define SIN_TABLE(_d) (tTrigFraction)pgm_read_word_near(m_SinTable + _d)
#define COS_TABLE(_d) (tTrigFraction)pgm_read_word_near(m_SinTable + ((_d<=90)?(90-_d):(360+90 -_d)))

// Could have half the table but program space is plentiful (so far)
static const tTrigFraction m_SinTable[] PROGMEM =
{
  // 8-bit:
  0, 4, 9, 13, 18, 22, 27, 31, 36, 40, 44, 49, 53, 58, 62, 66, 71, 75, 79, 83, 88, 92, 96, 100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 139, 143, 147, 150, 154, 158, 161, 165, 168, 171, 175, 178, 181, 184, 187, 190, 193, 196, 199, 202, 204, 207, 210, 212, 215, 217, 219, 222, 224, 226, 228, 230, 232, 234, 236, 237, 239, 241, 242, 243, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 254, 255, 255, 255, 256, 256, 256, 256, 256, 256, 256, 255, 255, 255, 254, 254, 253, 252, 251, 250, 249, 248, 247, 246, 245, 243, 242, 241, 239, 237, 236, 234, 232, 230, 228, 226, 224, 222, 219, 217, 215, 212, 210, 207, 204, 202, 199, 196, 193, 190, 187, 184, 181, 178, 175, 171, 168, 165, 161, 158, 154, 150, 147, 143, 139, 136, 132, 128, 124, 120, 116, 112, 108, 104, 100, 96, 92, 88, 83, 79, 75, 71, 66, 62, 58, 53, 49, 44, 40, 36, 31, 27, 22, 18, 13, 9, 4, 0, -4, -9, -13, -18, -22, -27, -31, -36, -40, -44, -49, -53, -58, -62, -66, -71, -75, -79, -83, -88, -92, -96, -100, -104, -108, -112, -116, -120, -124, -128, -132, -136, -139, -143, -147, -150, -154, -158, -161, -165, -168, -171, -175, -178, -181, -184, -187, -190, -193, -196, -199, -202, -204, -207, -210, -212, -215, -217, -219, -222, -224, -226, -228, -230, -232, -234, -236, -237, -239, -241, -242, -243, -245, -246, -247, -248, -249, -250, -251, -252, -253, -254, -254, -255, -255, -255, -256, -256, -256, -256, -256, -256, -256, -255, -255, -255, -254, -254, -253, -252, -251, -250, -249, -248, -247, -246, -245, -243, -242, -241, -239, -237, -236, -234, -232, -230, -228, -226, -224, -222, -219, -217, -215, -212, -210, -207, -204, -202, -199, -196, -193, -190, -187, -184, -181, -178, -175, -171, -168, -165, -161, -158, -154, -150, -147, -143, -139, -136, -132, -128, -124, -120, -116, -112, -108, -104, -100, -96, -92, -88, -83, -79, -75, -71, -66, -62, -58, -53, -49, -44, -40, -36, -31, -27, -22, -18, -13, -9, -4,
};

// The ship size is scaled by a fraction with a numerator over 2^SCALE_FRACTION_BITS
#define SCALE_FRACTION_BITS 8

int16_t ComputeMaxShipScale()
{
  // Compute a scale fraction to make the ship exactly fix in the ship window
  const ship::Vertex* pVertex = currentShip.vertices;
  uint32_t maxR = 1ULL;
  for (size_t vertexIdx = 0; vertexIdx < currentShip.numVertices; vertexIdx++, pVertex++)
  {
    ship::Vertex vertex;
    memcpy_P(&vertex, pVertex, sizeof(ship::Vertex));
    int64_t x = vertex.x, y = vertex.y, z = vertex.z;
    uint32_t r = loader::Sqrt(x*x + y*y + z*z);
    if (r > maxR)
      maxR = r;
  }
  maxR++;
  return SHIP_WINDOW_SIZE * (1UL << SCALE_FRACTION_BITS) / maxR / 2UL;
}

void Init()
{
#if 0
  // dump the m_SinTable entries to Serial
  for (short angle = 0; angle < 360; angle++)
  {
    tTrigFraction s = (tTrigFraction)round((1 << TRIG_FRACTION_BITS) * sin(angle * 3.1415926535897932384626433832795 / 180.0));
    Serial.print(s);
    Serial.print(",");
  }
  Serial.println();
#endif
  dials::Draw(true);
  LoadShip(true, false);
  labelShip = false;
  DrawTextLines();
}

void LoadShip(bool animateApproach, bool label)
{
  // Start using the configured ship
  currentShipType = config::data.m_ShipType;
  if (currentShipType == ship::LAST_SHIP)
  {
    // pick a random ship
#ifdef RTC_I2C_ADDRESS
    // but don't include the clock in random ships
    currentShipType = static_cast<ship::Type>(random(ship::Clock));
#else    
    currentShipType = static_cast<ship::Type>(random(ship::LAST_SHIP));
#endif  
  }
  ship::GetDetails(currentShipType, currentShip);
  maxShipScale = ComputeMaxShipScale();
  
#ifdef ENABLE_APPROACH  
  currentShipScale = animateApproach ? 1 : maxShipScale;
#else
  (void)animateApproach;
  currentShipScale = maxShipScale;
#endif

#ifdef DEBUG
  if (sparse::highWater)
    DBG(sparse::highWater); 
#endif     
  computeNewNormals();
  labelShip = label;

#ifdef ENABLE_RANDOM_ROTATION
  // random pitch & roll rates
  pitchStepDegrees = randomStepDegrees();
  rollStepDegrees  = randomStepDegrees();
#endif  
}

static int64_t NORM(int64_t a)
{
  // normalize numerators, basically multiply by denominator
  if (a >= 0)
    return a << TRIG_FRACTION_BITS;
  else
    return -(-a << (TRIG_FRACTION_BITS));   // shifting -ve values is undefined
}

static int16_t CONV(int64_t a, int16_t n, int16_t scale)
{
  // Convert fractional form to useable value -- multiply be scale then divide by denominator, with rounding
  a *= scale;
  int64_t div;
  bool neg = a < 0;
  if (neg)    // Shifting -ve values is undefined
    a = -a;
  div = a >> (n * TRIG_FRACTION_BITS - 1 + SCALE_FRACTION_BITS);
  // We're dividing by shifting right by m
  // The remainder is the lower (m-1) bits.
  // We should round up if the remainder's high bit is set.
  int16_t round = div & 1;
  div >>= 1;
  div += round;
  if (neg)
    div = -div;
  return (int16_t)div;
}

// Rotations
// We rotate about the view's Z axis (roll) and X axis (pitch). There is no yaw (Y).

// Just returns the UNSCALED rotated Z
static int64_t RotateZ(int16_t x, int16_t y, int16_t z, int16_t rollDeg, int16_t pitchDeg)
{
  // These values are numerators of fractional representations of the trig values, from tables
  // The denominator is 1 << TRIG_FRACTION_BITS
  int64_t cosRoll  = COS_TABLE(rollDeg),  sinRoll  = SIN_TABLE(rollDeg);
  int64_t cosPitch = COS_TABLE(pitchDeg), sinPitch = SIN_TABLE(pitchDeg);

  // These values are the matrix multiplication terms from [x,y,z][rotation 3x3], like
  //    y' = x*(cosA*sinB*sinG - sinA*cosG) + y*(sinA*sinB*sinG + cosA*cosG) + z*cosB*sinG
  // But refactored and NORM ensures all terms have the same denominator, (1 << TRIG_FRACTION_BITS)^2 or ^3.
  return  x * sinRoll * sinPitch - y * cosRoll * sinPitch + NORM(z * cosPitch);
}

// Just updates the x & y with rotated & scaled values (see above)
static void RotateXY(int16_t& x, int16_t& y, int16_t z, int16_t rollDeg, int16_t pitchDeg)
{
  int64_t cosRoll  = COS_TABLE(rollDeg),  sinRoll  = SIN_TABLE(rollDeg);
  int64_t cosPitch = COS_TABLE(pitchDeg), sinPitch = SIN_TABLE(pitchDeg);

  int64_t X =  x * cosRoll          + y * sinRoll;
  int64_t Y = -x * sinRoll * cosPitch + y * cosRoll * cosPitch + NORM(z * sinPitch);
  x = CONV(X, 1, currentShipScale);
  y = CONV(Y, 2, currentShipScale);
}


void NormalizeAngle(int16_t& angle)
{
  // make angle valid
  while (angle >= 360)
    angle -= 360;
  while (angle < 0)
    angle += 360;
}


// the coords we will need to draw the edges, tansformed to the screen
struct Coords
{
  uint8_t x, y;
};

Coords m_transformedCoords[VIEW_MAX_VERTICES];


// "font" for the digits on the clock ship
// Each nibble in char defn's below specifies a node on a grid:
// draw      move
// 6  7      E  F
//
// 3  4      B  C
//      5         D
// 0  1 2    8  9 A
#ifdef ENABLE_STD_CLOCK_DIGITS
static const uint32_t char_Defns[] PROGMEM = {0x00010679, 0x00000079, 0x0010347E, 0x003C017E, 0x00043E1F, // 0-9, pretty standard
                                              0x00763418, 0x0003410E, 0x0000017E, 0x04B06718, 0x00043679};
#else
static const uint32_t char_Defns[] PROGMEM = {0x0004730C, 0x00000079, 0x00067409, 0x003C6748, 0x00004379, // 0-9, aggressively diagonal, fewer pixels
                                              0x00076348, 0x0003403F, 0x00000678, 0x0034730C, 0x00043748};
#endif
void DrawClockFace(int minX, int maxX)
{
  // draw the symthetic "edges" on the clock ship that show the time
  ship::Vertex org;
  char strTime[4];
  bool dotOn = rtc.ReadSecond() % 2;
  GetTimeStr(strTime);
  memcpy_P(&org,  currentShip.vertices, sizeof(ship::Vertex));
  org.x += CLOCK_SHIP_DIGIT_X;
  org.y += CLOCK_SHIP_DIGIT_Y;
  for (int digit = 0; digit < 4; digit++)
  {
    if (strTime[digit] != ' ')
    {
      uint32_t defn = pgm_read_dword(char_Defns  + strTime[digit] - '0');
      bool dot = dotOn && (digit == 1);
      Coords prevCoord = {0, 0};
      while (defn)
      {
        byte thisNode = defn & 0x07;
        int dX = 16*(thisNode % 3);
        int dY = 16*(thisNode / 3);
        if (dX == 32)
        {
          // dot coords are a special case
          dY >>= 1;
          dX = 20;
        }
        Coords thisCoord;
        int16_t x = org.x + dX*CLOCK_SHIP_DIGIT_W/16;
        int16_t y = org.y + dY*CLOCK_SHIP_DIGIT_H/16;
        RotateXY(x, y, org.z, rollDegrees, pitchDegrees);
        thisCoord.x = SHIP_WINDOW_SIZE / 2 + x;
        thisCoord.y = SHIP_WINDOW_SIZE / 2 - y;
        if (!(defn & 0x08))
        {
          sparse::Line(prevCoord.x, prevCoord.y, thisCoord.x, thisCoord.y, minX, maxX);
        }
        prevCoord = thisCoord;
        defn >>= 4;
        if (!defn & dot)
        {
          defn = 0x5A;  // add 'dot' nodes
          dot = false;
        }
      }
    }
    org.x += CLOCK_SHIP_DIGIT_W + CLOCK_SHIP_DIGIT_G;
  }
}

// Status screen
static const char statusText[] PROGMEM =
  //         ------------------------------
  TEXT_MSTR("      COMMANDER MARK          ") // (or the Time)
  TEXT_MSTR("\n")
  TEXT_MSTR("\n")
  TEXT_MSTR("Present System      :Lave")
  TEXT_MSTR("Hyperspace System   :Lave")
  TEXT_MSTR("Condition           :Docked")
  TEXT_MSTR("Fuel:7.0 Light Years")
  TEXT_MSTR("Cash: 100000.0 Cr")
  TEXT_MSTR("Legal Status: Clean")
  TEXT_MSTR("Rating: ---- E L I T E ----")  // Harmless, Mostly Harmless, Poor, Average, Above Average, Competent, Dangerous, Deadly or ---- E L I T E ----
  TEXT_MSTR("\n")
  TEXT_MSTR("EQUIPMENT:")
     //TEXT_MSTR("Escape Capsule")
       TEXT_MSTR("Fuel Scoops")
       TEXT_MSTR("E.C.M.System")
       TEXT_MSTR("Energy Bomb")
       TEXT_MSTR("Energy Unit")
       TEXT_MSTR("Docking Computer")
       TEXT_MSTR("Galactic Hyperdrive")
       TEXT_MSTR("Front Beam Laser")  // Lasers are Pulse, Beam, Military or Mining?
     //TEXT_MSTR("Rear Pulse Laser")
     //TEXT_MSTR("Left Beam Laser")
     //TEXT_MSTR("Right Beam Laser")
;

void DrawStatus()
{
  // draw the status text screen
  const char* pLine = statusText;
  int textRow = 0;
  while (pgm_read_byte(pLine))
  {
    if (textRow == 0 && config::data.m_bTimeTitle)
    {
      // replace commander with time
      int x, y;
      text::CharReader charReader;
      const char* pTimeStr = GetTextLine(0, x, y, charReader);  // line 0 is time (or elite)
      text::Draw(x, y, pTimeStr, charReader);
    }
    else if (pgm_read_byte(pLine) != '\n')
      text::Draw((textRow > 11)?SCREEN_OFFSET_X + 6*TEXT_SIZE:SCREEN_OFFSET_X + TEXT_SIZE, TEXT_SIZE + TEXT_SIZE*textRow, pLine);
    pLine += text::StrLen(pLine) + 1;
    textRow++;
  }
  LCD_FILL_BYTE(LCD_BEGIN_FILL(SCREEN_OFFSET_X, SCREEN_OFFSET_Y + 19, SPACE_WIDTH, 1), 0xFF);
}

void Loop()
{
#ifdef ENABLE_STATUS_SCREEN
  if (config::data.m_bShowLoadScreen)
    AnimateShip();
  else
    DrawStatus();
#else
  AnimateShip();
#endif  
}

void AnimateShip()
{
  // draw a frame of the ship animation
  frameCount++;
  if (frameCount > 1000)
  {
    frameCount = 0;
    if (config::data.m_ShipType == ship::LAST_SHIP && random(2))
    {
      // every 1000 frames (~100s) toss a coin. If heads, pick a new ship at random
      LoadShip(false, true);
    }
  }

  if (currentShipScale < maxShipScale)
  {
    dials::UpdateRadar(currentShipScale, maxShipScale);
  }

  DrawShip();

  if (currentShipScale < maxShipScale)
  {
    currentShipScale += 2;
    if (currentShipScale >= maxShipScale)
    {
      dials::UpdateRadar(0, 0);  // remove it?
      currentShipScale = maxShipScale;
    }
  }

  rollDegrees += rollStepDegrees;
  pitchDegrees += pitchStepDegrees;

  if (labelShip)
  {
    const char* pShipName = text::StrN(ship::NameMultiStr_PGM, currentShipType);
    int len = strlen_P(pShipName);
    const int margin = TEXT_SIZE/2;
    int x = (LCD_WIDTH - len*TEXT_SIZE)/2;
    int y = SHIP_WINDOW_ORIGIN_Y + (SHIP_WINDOW_SIZE - TEXT_SIZE)/2;
    LCD_FILL_BYTE(LCD_BEGIN_FILL(x - margin, y - margin, len*TEXT_SIZE + 2*margin, TEXT_SIZE + 2*margin), 0x00);
    text::Draw(x, y, pShipName);
    delay(1000);
    labelShip = false;
  }
}

void DrawShip()
{
  // Does the work of drawing the ship
  NormalizeAngle(rollDegrees);
  NormalizeAngle(pitchDegrees);
#ifdef ENABLE_SPARSE_WIDE
  const int minX = 0, maxX = SHIP_WINDOW_SIZE;
  {
#ifdef DEBUG
    unsigned long nowMS = millis();
#endif    
#else
  int minX = 0, maxX = SHIP_WINDOW_SIZE / 2;
  for (int pass = 0; pass < 2; pass++)
  {
#ifdef DEBUG
    unsigned long nowMS = millis();
#endif    
    if (pass == 1)
    {
      minX = maxX;
      maxX = SHIP_WINDOW_SIZE;
    }
#endif    
    sparse::Clear();
    uint16_t visibleFaces = 0;
    const Face* pNormal = viewFaces;
    // build a bitset of the faces that are visible, those with a +ve normal
    for (size_t faceIdx = 0; faceIdx < currentShip.numFaces; faceIdx++, pNormal++)
    {
      if (RotateZ(pNormal->normal_x, pNormal->normal_y, pNormal->normal_z, rollDegrees, pitchDegrees) > 0LL)
        visibleFaces |= 1 << faceIdx;
    }
    // transform just the visible vertices!
    const ship::Vertex* pVertex = currentShip.vertices;
    Coords* pCoord = m_transformedCoords;
    for (size_t vertexIdx = 0; vertexIdx < currentShip.numVertices; vertexIdx++, pVertex++, pCoord++)
    {
      ship::Vertex vertex;
      memcpy_P(&vertex, pVertex, sizeof(ship::Vertex));
      if (vertex.faces & visibleFaces)
      {
        RotateXY(vertex.x, vertex.y, vertex.z, rollDegrees, pitchDegrees);
        pCoord->x = SHIP_WINDOW_SIZE / 2 + vertex.x;
        pCoord->y = SHIP_WINDOW_SIZE / 2 - vertex.y;
      }
    }

    const ship::Edge* pEdge = currentShip.edges;
    // draw the visible edges
    for (size_t edgeIdx = 0; edgeIdx < currentShip.numEdges; edgeIdx++, pEdge++)
    {
      ship::Edge edge;
      memcpy_P(&edge, pEdge, sizeof(ship::Edge));
      if ((visibleFaces & (1 << edge.face1)) || (visibleFaces & (1 << edge.face2)))
      {
        sparse::Line(m_transformedCoords[edge.vertex1].x, m_transformedCoords[edge.vertex1].y, m_transformedCoords[edge.vertex2].x, m_transformedCoords[edge.vertex2].y, minX, maxX);
      }
    }
#ifdef RTC_I2C_ADDRESS
    // Draw the time on the clock ship
    if (currentShipType == ship::Clock && visibleFaces & 1)
      DrawClockFace(minX, maxX);
#endif      
#ifdef DEBUG
    unsigned long durationMS = millis() - nowMS;
    sumLinesMS += durationMS;
    nowMS = millis();
#endif

    // paint the result
    byte* pRowStart = NULL;
    int textX, textY;
    text::CharReader charReader;
    // XOR-paintt the upper rows
    const char* pStr = GetTextLine(0, textX, textY, charReader);  // ELITE
    sparse::XORPaint(minX, 0, TEXT_SIZE, textX, textY, pStr, charReader, pRowStart);
    pStr = GetTextLine(1, textX, textY, charReader);   // Load
    int LoadRow = textY - SHIP_WINDOW_ORIGIN_Y;
    // paint the middle part
    sparse::Paint(minX, TEXT_SIZE, LoadRow, pRowStart);
    // XOR-paint the lower rows
    sparse::XORPaint(minX, LoadRow, SHIP_WINDOW_SIZE, textX, textY, pStr, charReader, pRowStart);
    
#ifdef DEBUG
    durationMS = millis() - nowMS;
    sumPaintMS += durationMS;
#endif    
  }
}

void ManualMode(int holdX, int holdY)
{
  // drag on the touch screen to pitch & roll
  manualMode = true;
  DrawTextLines();
  currentShipScale = maxShipScale;
  int prevX = holdX, prevY = holdY;
  DrawShip();
  bool dragging = true;
  int16_t start_RollDegrees = rollDegrees, start_PitchDegrees = pitchDegrees;
  unsigned long idleStartMS = millis();
  unsigned long dragStartMS = millis();
  bool firstDrag = true;

  int textX, eliteY, loadY;
  text::CharReader charReader;
  GetTextLine(0, textX, eliteY, charReader);
  eliteY += TEXT_SIZE;
  GetTextLine(1, textX, loadY, charReader);
  while (true)
  {
    if (dragging)
    {
      int thisX, thisY;
      if (LCD_GET_TOUCH(thisX, thisY))
      {
        // drag
        int16_t delta_RollDegrees = (prevX - thisX) / 2, delta_PitchDegrees = (prevY - thisY) / 2;
        bool draw = false;
        if (delta_RollDegrees)
        {
          rollDegrees  = start_RollDegrees  + delta_RollDegrees;
          draw = true;
        }
        if (delta_PitchDegrees)
        {
          pitchDegrees = start_PitchDegrees + delta_PitchDegrees;
          draw = true;
        }
        if (draw)
          DrawShip();
      }
      else
      {
        // release
        if (!firstDrag && millis() - dragStartMS < 100)
        {
          // tap -- exit
          break;
        }
        idleStartMS = millis();
        dragging = false;
      }
    }
    else
    {
      if (LCD_GET_TOUCH(prevX, prevY))
      {
        if (prevX > SCREEN_OFFSET_X && prevX < (SCREEN_OFFSET_X + SCREEN_WIDTH) && prevY > eliteY && prevY < loadY)
        {
          // touch inside extended ship area
          dragging = true;
          start_RollDegrees = rollDegrees;
          start_PitchDegrees = pitchDegrees;
          dragStartMS = millis();
          firstDrag = false;
        }
        else
          break;
      }
      else
      {
        if (millis() - idleStartMS > 20000)
        {
          break;
        }
      }
    }

    if (btn1Set.CheckButtonPress() || btn2Adj.CheckButtonPress())
      break;
  }

  manualMode = false;
  DrawTextLines();
  DrawShip();
}
}
