#pragma once

// Sparse pixel representation, the key to making this work on an Arduino
namespace sparse
{
  void Clear();
  void Line(int x0, int y0, int x1, int y1, int minX, int maxX);
  void Paint(int originX, int minRow, int maxRow, byte*& pRowStart);
  void XORPaint(int originX, int minRow, int maxRow, byte textX, byte textY, const char* str, text::CharReader charLoader, byte*& pRowStart);
  extern uint16_t highWater;
};
