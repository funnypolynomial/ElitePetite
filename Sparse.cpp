#include <Arduino.h>
#include "Text.h"
#include "Elite.h"
#include "Sparse.h"

namespace sparse {
/*
  Sparse pixels! The key to making this work on an Arduino.
  
  On an Electron (or a BBC Micor), Elite could write graphics data to memory-mapped video RAM, 10k of it.  
  No such luxury on the Arduino Uno with a maximum of 2k RAM (some of which is needed for variables and the stack). 
  My solution is for pixel writes to update a compact representation of "on" pixels.  This is effectively run-length-encoding, on-the-fly, on a row-by-row basis.
  
  Each row is a contiguous block of bytes representing runs of horizontal pixels. The runs are sorted from left to right (but they may grow into each other; Draw() handles this).  
  All the run data is in a single array of bytes, pool[]. rows[r] stores the number of bytes in row r.
  Setting a pixel at (row, col) is a matter of finding the start of the row block by adding values in rows then scanning across the block to find where col belongs and 
  either inserting a new single pixel entry, or extending an existing run. Adding a byte to a run means shuffling up all the subsequent bytes.
  Drawing is a matter of scanning all the rows and run data and sending bands of black/white pixel data to the LCD.

  There are two variations of the algorithm, selected by ENABLE_SPARSE_WIDE
  sparse-A
  A run is encoded as 1 or 2 bytes. The low 7-bits of first byte is the start column. 
  If the high bit is clear, this is a 1-byte representation and the run length is 1.
  If the high bit is set, this is a 2-byte representation and the run length is the following byte.
  Columns are limited to 0..127 so the ship is drawn in two halves. This causes some "tearing" but it does require a modest amount of RAM. 
  
  sparse-B (WIDE)
  This is a response to the tearing effect.
  A run is encoded as 1, 2 or 3 bytes.  The first byte is the start column.
  If the second byte is greater than the column, or the first byte is the last in a row block, this is a 1-byte representation and the run length is 1.
  If the second byte is less than or equal to the column, this is a 2-byte representation and the second byte is the run length.
  If the second byte is NUL (0x00) this is a 3-byte representation and the run length is the third byte.
  Columns are limited to 0..254 but that is not an issue here, so the whole ship can be drawn at once, but the pool does need to be larger. 

  Consider this triangle:
      012345678901234567890
    0|      ***
    1|     *   ***
    2|    *       ***
    3|   *           ***
    4|  *      ******
    5| ********

  Sparse-A would encode this as:
    rows  pool bytes
    [2]   {134,3},
    [3]            {5}, {137,3},
    [3]                          {4}, {140,3},
    [3]                                       {3}, {143,3},
    [3]                                                     {2}, {137,6},
    [2]                                                                   {129,8},
*/

// Special byte values
#define INF 0xFF
#define NUL 0x00

#ifdef ENABLE_SPARSE_WIDE
// Drawn in full
const int SPARSE_ROWS = SHIP_WINDOW_SIZE;
const int SPARSE_COLS = SHIP_WINDOW_SIZE;
const int SPARSE_POOL_SIZE = 1024;

// the x at _ptr in the pool
#define SPARSE_GET_X(_ptr)      (*_ptr)
// the number of pixels in the run at _ptr in the pool
#define SPARSE_GET_LEN(_last, _ptr)    (((_last) || *_ptr < *(_ptr+1)) ? 1 : ((*(_ptr+1) == NUL) ? *(_ptr+2) : *(_ptr+1)))
// number of bytes in the entry at _ptr in the pool
#define SPARSE_GET_SIZE(_last, _ptr)   (((_last) || *_ptr < *(_ptr+1)) ? 1 : ((*(_ptr+1) == NUL) ? 3 : 2))

#else
// Drawn as left and right halves
const int SPARSE_ROWS = SHIP_WINDOW_SIZE;
const int SPARSE_COLS = SHIP_WINDOW_SIZE / 2;
const int SPARSE_POOL_SIZE = 550;

// the x at _ptr in the pool
#define SPARSE_GET_X(_ptr)      (*(_ptr) & 0x7F)
// the number of pixels in the run at _ptr in the pool
#define SPARSE_GET_LEN(_last, _ptr)    ((*(_ptr) & 0x80)?*((_ptr) + 1):1)
// number of bytes in the entry at _ptr in the pool
#define SPARSE_GET_SIZE(_last, _ptr)   ((*(_ptr) & 0x80)?2:1)
// true if data at _ptr is 2-bytes
#define SPARSE_IS_DUAL(_ptr)     (*(_ptr) & 0x80)
// set the data to be 2-byte
#define SPARSE_SET_DUAL(_ptr)    (*(_ptr) |= 0x80)
#endif

byte rows[SPARSE_ROWS]; // number of bytes in row's sparse representation
byte pool[SPARSE_POOL_SIZE]; // representations go here ("values", "cols"!)
uint16_t pool_top;    // index to next free byte
byte* cachePtr = NULL;
byte cacheY = INF;
byte cacheX = INF;
bool cacheLast = false;
const int numMidRows = 4;
uint16_t sumToMidRow[numMidRows]; // cache the sum of the rows up to n*SPARSE_ROWS/numMidRows
uint16_t highWater = 0;
#ifdef DEBUG  
#define SET_HIGHWATER(_rhs) highWater = _rhs
#else
#define SET_HIGHWATER(_rhs)
#endif

static bool Insert(byte y, byte* pValue, byte value, byte value2 = 0)
{
  // inserts a new byte *at* pValue. Bytes above are shuffled
  // does most of the work, shuffles up the pool to make room for the new value
  // true if there was room
  // Also handles two bytes, if value2 != 0

  if (value2)
  {
    // special case, insert NUL & 2
    if (pool_top < SPARSE_POOL_SIZE - 1)
    {
      // there's room for 2 bytes
      // shuffle pool bytes up
      memmove(pValue + 2, pValue, (pool + pool_top) - pValue);
      *pValue = value;
      *(pValue + 1) = value2;
      pool_top += 2;
      rows[y] += 2;
      for (int midRow = 1; midRow < numMidRows; midRow++)
        if (y < midRow*SPARSE_ROWS/numMidRows)
          sumToMidRow[midRow] += 2;
      SET_HIGHWATER(max(highWater, pool_top));
      return true;
    }
  }
  else if (pool_top < SPARSE_POOL_SIZE)
  {
    // there's room
    // shuffle pool bytes up
    memmove(pValue + 1, pValue, (pool + pool_top) - pValue);
    *pValue = value;
    pool_top++;
    rows[y]++;
    for (int midRow = 1; midRow < numMidRows; midRow++)
      if (y < midRow*SPARSE_ROWS/numMidRows)
        sumToMidRow[midRow]++;
    SET_HIGHWATER(max(highWater, pool_top));
    return true;
  }
  SET_HIGHWATER(SPARSE_POOL_SIZE);
  return false;
}

#ifdef ENABLE_SPARSE_WIDE
static void Pixel(byte x, byte y)
{
  // set the pixel in the sparse data
  if (y >= SPARSE_ROWS || x >= SPARSE_COLS)
    return;

  if (cacheY == y && cacheX == x)
  {
    // cache the last location to quickly append to horizontal sequences
    byte* thisPtr = cachePtr;
    byte bytes = SPARSE_GET_SIZE(cacheLast, thisPtr);
    x = SPARSE_GET_X(thisPtr);
    cacheLast = false;
    if (bytes == 1)
    {
      if (x >= 2) // 2 bytes will work
      {
        if (!Insert(y, thisPtr + 1, 2))  // len=2
          cacheY = INF;
      }
      else // need 3
      {
        if (!Insert(y, thisPtr + 1, NUL, 2))  // len=2, as 3 bytes
          cacheY = INF;
      }
    }
    else if (bytes == 2)
    {
      thisPtr++;
      if (x > *thisPtr)
        (*thisPtr)++;   // len++
      else if (Insert(y, thisPtr++, NUL)) // need to go to 3 bytes
        (*thisPtr)++;   // len++
      else
        cacheY = INF;
    }
    else // 3 bytes
    {
      thisPtr += 2;
      (*thisPtr)++;   // len++
    }
    cacheX++;
    return;
  }

  byte* thisPtr = pool;
  byte* rowPtr = rows;
  int startRow = 0;
  // check short-cuts
  for (int midRow = numMidRows-1; midRow; midRow--)
    if (y >= midRow*SPARSE_ROWS/numMidRows)
    {
      startRow = midRow*SPARSE_ROWS/numMidRows;
      thisPtr += sumToMidRow[midRow];
      rowPtr  += startRow;
      break;
    }
  // add up the bytes per row to find the start of the data for row y
  for (int row = startRow; row < y; row++, rowPtr++)
    thisPtr += *rowPtr;

  byte bytesInRow = *rowPtr;
  while (bytesInRow && ((int)x - (int)(SPARSE_GET_X(thisPtr) + SPARSE_GET_LEN(bytesInRow == 1, thisPtr))) >= 1)
  {
    byte bytes = SPARSE_GET_SIZE(bytesInRow == 1, thisPtr);
    bytesInRow -= bytes;
    thisPtr += bytes;
  }
  cacheY = INF;
  if (!bytesInRow) // append. got to the end without finding a place to insert/update
  {
    Insert(y, thisPtr, x);
    cacheX = x + 1;
    cacheY = y;
    cachePtr = thisPtr;
    cacheLast = true;
  }
  // thisPtr is an item with an x larger than ours, or within 1 of ours
  else if (x == SPARSE_GET_X(thisPtr) - 1)  // expand thisPtr left
  {
    byte bytes = SPARSE_GET_SIZE(bytesInRow == 1, thisPtr);
    (*thisPtr)--;     // x--
    if (bytes == 1)
    {
      if (x >= 2) // 2 bytes will work
        Insert(y, thisPtr + 1, 2);  // len=2
      else // need 3
        Insert(y, thisPtr + 1, NUL, 2);  // len=2, as 3 bytes
    }
    else if (bytes == 2)
    {
      thisPtr++;
      if (x > *thisPtr)
        (*thisPtr)++;   // len++
      else if (Insert(y, thisPtr++, NUL)) // need to go to 3 bytes
        (*thisPtr)++;   // len++
    }
    else // 3 bytes
    {
      thisPtr += 2;
      (*thisPtr)++;   // len++
    }
  }
  else if (x == (SPARSE_GET_X(thisPtr) + SPARSE_GET_LEN(bytesInRow == 1, thisPtr))) // expand thisPtr right
  {
    byte bytes = SPARSE_GET_SIZE(bytesInRow == 1, thisPtr);
    x = SPARSE_GET_X(thisPtr);
    if (bytes == 1)
    {
      if (x >= 2) // 2 bytes will work
        Insert(y, thisPtr + 1, 2);  // len=2
      else // need 3
        Insert(y, thisPtr + 1, NUL, 2);  // len=2, as 3 bytes
    }
    else if (bytes == 2)
    {
      thisPtr++;
      if (x > *thisPtr)
        (*thisPtr)++;   // len++
      else if (Insert(y, thisPtr++, NUL)) // need to go to 3 bytes
        (*thisPtr)++;   // len++
    }
    else // 3 bytes
    {
      thisPtr += 2;
      (*thisPtr)++;   // len++
    }
  }
  else if (x < SPARSE_GET_X(thisPtr))     // insert before thisPtr
  {
    Insert(y, thisPtr, x);
    cacheX = x + 1;
    cacheY = y;
    cachePtr = thisPtr;
    cacheLast = bytesInRow == 1;
  }
}
#else
static void Pixel(byte x, byte y)
{
  // set the pixel in the sparse data
  if (y >= SPARSE_ROWS || x >= SPARSE_COLS)
    return;

  if (cacheY == y && cacheX == x)
  {
    // cached the last location to quickly append to horizontal sequences
    if (SPARSE_IS_DUAL(cachePtr))
    {
      (*(cachePtr + 1))++;
    }
    else if (Insert(cacheY, cachePtr + 1, 2))  // len=2
    {
      SPARSE_SET_DUAL(cachePtr);
    }
    else
      cacheY = INF;
    cacheX++;
    return;
  }

  byte* thisPtr = pool;
  byte* rowPtr = rows;
  int startRow = 0;
  // check short-cuts
  for (int midRow = numMidRows-1; midRow; midRow--)
    if (y >= midRow*SPARSE_ROWS/numMidRows)
    {
      startRow = midRow*SPARSE_ROWS/numMidRows;
      thisPtr += sumToMidRow[midRow];
      rowPtr  += startRow;
      break;
    }
  // add up the bytes per row to find the start of the data for row y
  for (int row = startRow; row < y; row++, rowPtr++)
    thisPtr += *rowPtr;

  byte bytesInRow = *rowPtr;
  while (bytesInRow && ((int)x - (int)(SPARSE_GET_X(thisPtr) + SPARSE_GET_LEN(false, thisPtr))) >= 1)
  {
    bytesInRow -= SPARSE_GET_SIZE(false, thisPtr);
    thisPtr += SPARSE_GET_SIZE(false, thisPtr);
  }
  cacheX = INF;
  if (!bytesInRow) // append. got to the end without finding a place to insert/update
  {
    Insert(y, thisPtr, x);
    cacheX = x + 1;
    cacheY = y;
    cachePtr = thisPtr;
  }
  // thisPtr is an item with an x larger than ours, or within 1 of ours
  else if (x == SPARSE_GET_X(thisPtr) - 1)  // expand thisPtr left
  {
    if (SPARSE_IS_DUAL(thisPtr))
    {
      (*thisPtr)--;     // x--
      thisPtr++;
      (*thisPtr)++;   // len++
    }
    else if (Insert(y, thisPtr + 1, 2))  // len=2
    {
      SPARSE_SET_DUAL(thisPtr);
      (*thisPtr)--;     // x--
    }
  }
  else if (x == (SPARSE_GET_X(thisPtr) + SPARSE_GET_LEN(false, thisPtr)))  // expand thisPtr right
  {
    if (SPARSE_IS_DUAL(thisPtr))
    {
      thisPtr++;
      (*thisPtr)++;   // len++
    }
    else
    {
      if (Insert(y, thisPtr + 1, 2))  // len=2
      {
        SPARSE_SET_DUAL(thisPtr);
      }
    }
  }
  else if (x < SPARSE_GET_X(thisPtr))     // insert before thisPtr
  {
    Insert(y, thisPtr, x);
    cacheX = x + 1;
    cacheY = y;
    cachePtr = thisPtr;
  }
}
#endif

void Clear()
{
  // prepare for another render
  memset(rows, 0x00, sizeof(rows));
  pool_top = 0;
  for (int midRow = 0; midRow < numMidRows; midRow++)
    sumToMidRow[midRow] = 0;
}

void Line(int x0, int y0, int x1, int y1, int minX, int maxX)
{
  // Draw a line {x0, y0} to {x1, y1}. Clipped to minX..maxX
  // Always drawn left-to-right
  // Results in a series of calls to Pixel()
  int dx, dy;
  int     sy;
  int er, e2;
  if (x0 > x1)
  {
    // ensure x0 <= x1;
    dx = x0; x0 = x1; x1 = dx;
    dy = y0; y0 = y1; y1 = dy;
  }

  dx = x1 - x0;
  dy = (y1 >= y0) ? y0 - y1 : y1 - y0;
  sy = (y0 <  y1) ? 1       : -1;
  er = dx + dy;

  while (1)
  {
    if (minX <= x0 && x0 < maxX)
      Pixel(x0 - minX, y0);
    else if (x0 > maxX)   // clip
      return;
    if ((x0 == x1) && (y0 == y1))
      break;
    e2 = 2 * er;
    if (e2 >= dy)
    {
      er += dy;
      x0++;
    }
    if (e2 <= dx)
    {
      er += dx;
      y0 += sy;
    }
  }
}

void Paint(int originX, int minRow, int maxRow, byte*& pRowStart)
{
  // Paint the sparse pixels. Left edge is inset into window by originX.
  // Rows painted are minRow..maxRow
  // pRowStart optionally points to the start in the pool, updated to the end
  LCD_BEGIN_FILL(SHIP_WINDOW_ORIGIN_X + originX, SHIP_WINDOW_ORIGIN_Y + minRow, SPARSE_COLS, maxRow - minRow);
  if (!pRowStart)
    pRowStart = pool;
  byte* pRow = rows + minRow;
  for (int row = minRow; row < maxRow; row++, pRow++)
  {
    byte* pValue = pRowStart;
    byte rowLen = *pRow;
    pRowStart += rowLen;
    byte prevX = 0;
    if (rowLen)
    {
      while (rowLen)
      {
        byte x = SPARSE_GET_X(pValue);
        byte len = SPARSE_GET_LEN(rowLen == 1, pValue);
        byte size = SPARSE_GET_SIZE(rowLen == 1, pValue);
        if (prevX < x)
        {
          LCD_FILL_BYTE(x - prevX, 0x00);
          LCD_FILL_BYTE(len, 0xFF);
          prevX = x + len;
        }
        else if (prevX < x + len)
        {
          // deal with overlaps
          byte extra = x + len - prevX;
          LCD_FILL_BYTE(extra, 0xFF);
          prevX += extra;
        }
        rowLen -= size;
        pValue += size;
      }
      if (prevX < SPARSE_COLS)
        LCD_FILL_BYTE(SPARSE_COLS - prevX, 0x00);
    }
    else
    {
      // blank row
      LCD_FILL_BYTE(SPARSE_COLS, 0x00);
    }
  }
}

// convenience macros to "XOR" a sparse black or white pixel with the current raster pixel
#define EOR_BLACK(_cols) while (_cols--) if (raster::Next()) LCD_ONE_WHITE(); else LCD_ONE_BLACK();
#define EOR_WHITE(_cols) while (_cols--) if (raster::Next()) LCD_ONE_BLACK(); else LCD_ONE_WHITE();

void XORPaint(int originX, int minRow, int maxRow, byte textX, byte textY, const char* str, text::CharReader charLoader, byte*& pRowStart)
{
  // As above, paints the sparse rows XOR'ed on-the-fly with the string, drawn at textX, textY
  int fillX = SHIP_WINDOW_ORIGIN_X + originX;
  int fillY = SHIP_WINDOW_ORIGIN_Y + minRow;

  LCD_BEGIN_FILL(fillX, fillY, SPARSE_COLS, maxRow - minRow);
  raster::Start(textX, textY, str, charLoader);
  if (!pRowStart)
    pRowStart = pool;
  for (int row = minRow; row < maxRow; row++)
  {
    byte* pValue = pRowStart;
    byte rowLen = rows[row];
    pRowStart += rowLen;
    raster::Row(fillX, fillY++);
    byte prevX = 0;
    int cols;
    if (rowLen)
    {
      while (rowLen)
      {
        byte x = SPARSE_GET_X(pValue);
        byte len = SPARSE_GET_LEN(rowLen == 1, pValue);
        byte size = SPARSE_GET_SIZE(rowLen == 1, pValue);
        if (prevX < x)
        {
          cols = x - prevX;
          EOR_BLACK(cols);
          prevX = x + len;
          EOR_WHITE(len);
        }
        else if (prevX < x + len)
        {
          // deal with overlaps
          byte extra = x + len - prevX;
          prevX += extra;
          EOR_WHITE(extra);
        }
        rowLen -= size;
        pValue += size;
      }
      if (prevX < SPARSE_COLS)
      {
        cols = SPARSE_COLS - prevX;
        EOR_BLACK(cols);
      }
    }
    else
    {
      // blank row
      cols = SPARSE_COLS;
      EOR_BLACK(cols);
    }
  }
}
}
