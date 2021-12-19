#pragma once

// Loader utilities
namespace loader
{
  // Style of non-Elite background areas
  enum Background
  {
    BlackBackground, 
    BeebBackground, 
    ElkBackground, 
    BeigeBackground,
    StarsBackground
  };
  
  void Init();
  void DrawBackground();
  void DrawSpaceArea();

  // Load screen-memory data
  void DrawMonoBitImage(const byte* pData, int y, int height, int insetBlocks = 0);
  void DrawColourBitImage(const byte* pData, int y, int height, int insetBlocks = 0);
  
  uint32_t Sqrt(uint32_t s);

  extern const word palette[4]; // black, red, yellow, green (or black, red, white, cyan)
};
