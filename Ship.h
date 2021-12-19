#pragma once
#include "RTC.h"

// Ship blueprints
namespace ship
{
  struct Vertex
  {
    int16_t  x, y, z;
    uint16_t faces;
  };
  
  struct Edge
  {
    uint8_t vertex1, vertex2, face1, face2;
  };
  
  struct Face
  {
    // Note: only the *signs* of the original data, so we pack them all into a single byte
    int normal_x:2;
    int normal_y:2;
    int normal_z:2;
  };
  
  struct Details
  {
    const Vertex* vertices;
    uint8_t numVertices;
    const Edge* edges;
    uint8_t numEdges;
    const Face* faces;
    uint8_t numFaces;
  };
  
  // https://www.bbcelite.com/deep_dives/ship_blueprints_in_the_disc_version.html
  // But just the snakes. (and not Asp Mk 2, it glitches)
  enum Type
  {
    Adder,
    Anaconda,
    Boa,
    CobraMk3,
    Constrictor,
    FerDeLance,
    Krait,
    Mamba,
    Moray,
    Python,
    Sidewinder,
    Viper,
#ifdef RTC_I2C_ADDRESS    
    Clock,
#endif    
    LAST_SHIP // = random ship
  };
  
  void GetDetails(Type type, Details& detail);
  extern const char NameMultiStr_PGM[];
};

// Clock "Ship" digits size, gap and origin
#define CLOCK_SHIP_DIGIT_W 16
#define CLOCK_SHIP_DIGIT_H 20
#define CLOCK_SHIP_DIGIT_G 8
#define CLOCK_SHIP_DIGIT_X 28
#define CLOCK_SHIP_DIGIT_Y 15
