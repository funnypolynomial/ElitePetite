#pragma once

// BBC font text is 8x8
#define TEXT_SIZE 8

// Helper for multi-string constants.
// Multi-strings are one or more strings concatenated into a single string, usually in PROGMEM.
// Strings are separated by a NUL.  The multi-string ends with two NUL's
#define TEXT_MSTR(s) s "\0"

// BBC Font and string utils
namespace text
{
  // Read text chars from PROGMEM or normal RAM
  typedef char (*CharReader)(const char*);
  char ProgMemCharReader(const char* a);
  char StdCharReader(const char* a);
  
  // Draw text on screen
  void Draw(int x, int y, const char* str, CharReader charReader = ProgMemCharReader, word colour = 0); // here, 0 means white!
  
  // Extract the nth string from a multi-string
  const char* StrN(const char* multiStr, byte n, CharReader charReader = ProgMemCharReader);
  
  // Return the length of the string
  size_t StrLen(const char* str, CharReader charReader = ProgMemCharReader);
};

// Provide the pixels of a text string as raster lines
namespace raster
{
  void Start(int x, int y, const char* str, text::CharReader charReader = text::ProgMemCharReader);
  void Row(int x, int y);
  bool Next();
};
