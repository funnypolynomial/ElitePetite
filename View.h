#pragma once

// The 3D ship view
namespace view
{
  void Init();
  void Loop();
  void LoadShip(bool animateApproach, bool label);
  void AnimateShip();
  void DrawShip();
  void DrawTextLines();
  void ManualMode(int holdX, int holdY);
  const char* GetTextLine(int item, int& x, int& y, text::CharReader& charReader);
};
