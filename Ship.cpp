#include <Arduino.h>
#include "Text.h"
#include "Ship.h"

namespace ship {
// The ship data is from the original game.  See, for example, https://github.com/markmoxon/disc-elite-beebasm/tree/master/1-source-files/main-sources
// Visibility values are *NOT* stored. Culling detail probably only matters when the lines are being XOR'd -- distant ships may end up being a mess.
// Only the *SIGNS* of the original normals are stored. See computeNewNormals

// Various packing/conversion macros
#define VERTEX(x,y,z,f1,f2,f3,f4,vis) {x,y,z, (f1==15)?0xFFFF:(1U<<f1)|(1U<<f2)|(1U<<f3)|(1U<<f4)},
#define EDGE(v1,v2,f1,f2,vis) {v1,v2,f1,f2},
#define SIGN(_n) ((_n>=0)?+1:-1)
#define FACE(nx,ny,nz,vis) {SIGN(nx),SIGN(ny),SIGN(nz)},
#define SHIP_ARRAY(_a) _a, (uint8_t)(sizeof(_a)/sizeof(_a[0]))
#define SHIP_ARRAYS(_s) {SHIP_ARRAY(_s ## Vertices), SHIP_ARRAY(_s ## Edges), SHIP_ARRAY(_s ## Faces)}

// ----------------------------------------------------------------------
// Python
static const Vertex pythonVertices[] PROGMEM =
{
  //        x,    y,    z, face1, face2, face3, face4, visibility
  VERTEX(   0,    0,  224,     0,      1,    2,     3,         31)  // Vertex 0
  VERTEX(   0,   48,   48,     0,      1,    4,     5,         31)  // Vertex 1
  VERTEX(  96,    0,  -16,    15,     15,   15,    15,         31)  // Vertex 2
  VERTEX( -96,    0,  -16,    15,     15,   15,    15,         31)  // Vertex 3
  VERTEX(   0,   48,  -32,     4,      5,    8,     9,         31)  // Vertex 4
  VERTEX(   0,   24, -112,     9,      8,   12,    12,         31)  // Vertex 5
  VERTEX( -48,    0, -112,     8,     11,   12,    12,         31)  // Vertex 6
  VERTEX(  48,    0, -112,     9,     10,   12,    12,         31)  // Vertex 7
  VERTEX(   0,  -48,   48,     2,      3,    6,     7,         31)  // Vertex 8
  VERTEX(   0,  -48,  -32,     6,      7,   10,    11,         31)  // Vertex 9
  VERTEX(   0,  -24, -112,    10,     11,   12,    12,         31)  // Vertex 10
};

static const Edge pythonEdges[] PROGMEM =
{
  //    vertex1, vertex2, face1, face2, visibility
  EDGE(       0,       8,     2,     3,         31)  // Edge 0
  EDGE(       0,       3,     0,     2,         31)  // Edge 1
  EDGE(       0,       2,     1,     3,         31)  // Edge 2
  EDGE(       0,       1,     0,     1,         31)  // Edge 3
  EDGE(       2,       4,     9,     5,         31)  // Edge 4
  EDGE(       1,       2,     1,     5,         31)  // Edge 5
  EDGE(       2,       8,     7,     3,         31)  // Edge 6
  EDGE(       1,       3,     0,     4,         31)  // Edge 7
  EDGE(       3,       8,     2,     6,         31)  // Edge 8
  EDGE(       2,       9,     7,    10,         31)  // Edge 9
  EDGE(       3,       4,     4,     8,         31)  // Edge 10
  EDGE(       3,       9,     6,    11,         31)  // Edge 11
  EDGE(       3,       5,     8,     8,          7)  // Edge 12
  EDGE(       3,      10,    11,    11,          7)  // Edge 13
  EDGE(       2,       5,     9,     9,          7)  // Edge 14
  EDGE(       2,      10,    10,    10,          7)  // Edge 15
  EDGE(       2,       7,     9,    10,         31)  // Edge 16
  EDGE(       3,       6,     8,    11,         31)  // Edge 17
  EDGE(       5,       6,     8,    12,         31)  // Edge 18
  EDGE(       5,       7,     9,    12,         31)  // Edge 19
  EDGE(       7,      10,    12,    10,         31)  // Edge 20
  EDGE(       6,      10,    11,    12,         31)  // Edge 21
  EDGE(       4,       5,     8,     9,         31)  // Edge 22
  EDGE(       9,      10,    10,    11,         31)  // Edge 23
  EDGE(       1,       4,     4,     5,         31)  // Edge 24
  EDGE(       8,       9,     6,     7,         31)  // Edge 25
};

static const Face pythonFaces[] PROGMEM =
{
  //    normal_x, normal_y, normal_z, visibility
  FACE(      -27,       40,       11,        31)  // Face 0
  FACE(       27,       40,       11,        31)  // Face 1
  FACE(      -27,      -40,       11,        31)  // Face 2
  FACE(       27,      -40,       11,        31)  // Face 3
  FACE(      -19,       38,        0,        31)  // Face 4
  FACE(       19,       38,        0,        31)  // Face 5
  FACE(      -19,      -38,        0,        31)  // Face 6
  FACE(       19,      -38,        0,        31)  // Face 7
  FACE(      -25,       37,      -11,        31)  // Face 8
  FACE(       25,       37,      -11,        31)  // Face 9
  FACE(       25,      -37,      -11,        31)  // Face 10
  FACE(      -25,      -37,      -11,        31)  // Face 11
  FACE(        0,        0,     -112,        31)  // Face 12
};

// ----------------------------------------------------------------------
// Viper
static const Vertex viperVertices[] PROGMEM =
{
  //        x,    y,    z, face1, face2, face3, face4, visibility
  VERTEX(   0,    0,   72,     1,      2,    3,     4,         31)  // Vertex 0
  VERTEX(   0,   16,   24,     0,      1,    2,     2,         30)  // Vertex 1
  VERTEX(   0,  -16,   24,     3,      4,    5,     5,         30)  // Vertex 2
  VERTEX(  48,    0,  -24,     2,      4,    6,     6,         31)  // Vertex 3
  VERTEX( -48,    0,  -24,     1,      3,    6,     6,         31)  // Vertex 4
  VERTEX(  24,  -16,  -24,     4,      5,    6,     6,         30)  // Vertex 5
  VERTEX( -24,  -16,  -24,     5,      3,    6,     6,         30)  // Vertex 6
  VERTEX(  24,   16,  -24,     0,      2,    6,     6,         31)  // Vertex 7
  VERTEX( -24,   16,  -24,     0,      1,    6,     6,         31)  // Vertex 8
  VERTEX( -32,    0,  -24,     6,      6,    6,     6,         19)  // Vertex 9
  VERTEX(  32,    0,  -24,     6,      6,    6,     6,         19)  // Vertex 10
  VERTEX(   8,    8,  -24,     6,      6,    6,     6,         19)  // Vertex 11
  VERTEX(  -8,    8,  -24,     6,      6,    6,     6,         19)  // Vertex 12
  VERTEX(  -8,   -8,  -24,     6,      6,    6,     6,         18)  // Vertex 13
  VERTEX(   8,   -8,  -24,     6,      6,    6,     6,         18)  // Vertex 14
};

static const Edge viperEdges[] PROGMEM =
{
  //    vertex1, vertex2, face1, face2, visibility
  EDGE(       0,       3,     2,     4,         31)  // Edge 0
  EDGE(       0,       1,     1,     2,         30)  // Edge 1
  EDGE(       0,       2,     3,     4,         30)  // Edge 2
  EDGE(       0,       4,     1,     3,         31)  // Edge 3
  EDGE(       1,       7,     0,     2,         30)  // Edge 4
  EDGE(       1,       8,     0,     1,         30)  // Edge 5
  EDGE(       2,       5,     4,     5,         30)  // Edge 6
  EDGE(       2,       6,     3,     5,         30)  // Edge 7
  EDGE(       7,       8,     0,     6,         31)  // Edge 8
  EDGE(       5,       6,     5,     6,         30)  // Edge 9
  EDGE(       4,       8,     1,     6,         31)  // Edge 10
  EDGE(       4,       6,     3,     6,         30)  // Edge 11
  EDGE(       3,       7,     2,     6,         31)  // Edge 12
  EDGE(       3,       5,     6,     4,         30)  // Edge 13
  EDGE(       9,      12,     6,     6,         19)  // Edge 14
  EDGE(       9,      13,     6,     6,         18)  // Edge 15
  EDGE(      10,      11,     6,     6,         19)  // Edge 16
  EDGE(      10,      14,     6,     6,         18)  // Edge 17
  EDGE(      11,      14,     6,     6,         16)  // Edge 18
  EDGE(      12,      13,     6,     6,         16)  // Edge 19
};

static const Face viperFaces[] PROGMEM =
{
  //    normal_x, normal_y, normal_z, visibility
  FACE(        0,       32,        0,         31)  // Face 0
  FACE(      -22,       33,       11,         31)  // Face 1
  FACE(       22,       33,       11,         31)  // Face 2
  FACE(      -22,      -33,       11,         31)  // Face 3
  FACE(       22,      -33,       11,         31)  // Face 4
  FACE(        0,      -32,        0,         31)  // Face 5
  FACE(        0,        0,      -48,         31)  // Face 6
};

// ----------------------------------------------------------------------
// Sidewinder
static const Vertex sidewinderVertices[] PROGMEM =
{
  //        x,    y,    z, face1, face2, face3, face4, visibility
  VERTEX( -32,    0,   36,     0,      1,    4,     5,         31)  // Vertex 0
  VERTEX(  32,    0,   36,     0,      2,    5,     6,         31)  // Vertex 1
  VERTEX(  64,    0,  -28,     2,      3,    6,     6,         31)  // Vertex 2
  VERTEX( -64,    0,  -28,     1,      3,    4,     4,         31)  // Vertex 3
  VERTEX(   0,   16,  -28,     0,      1,    2,     3,         31)  // Vertex 4
  VERTEX(   0,  -16,  -28,     3,      4,    5,     6,         31)  // Vertex 5
  VERTEX( -12,    6,  -28,     3,      3,    3,     3,         15)  // Vertex 6
  VERTEX(  12,    6,  -28,     3,      3,    3,     3,         15)  // Vertex 7
  VERTEX(  12,   -6,  -28,     3,      3,    3,     3,         12)  // Vertex 8
  VERTEX( -12,   -6,  -28,     3,      3,    3,     3,         12)  // Vertex 9
};

static const Edge sidewinderEdges[] PROGMEM =
{
  //    vertex1, vertex2, face1, face2, visibility
  EDGE(       0,       1,     0,     5,         31)  // Edge 0
  EDGE(       1,       2,     2,     6,         31)  // Edge 1
  EDGE(       1,       4,     0,     2,         31)  // Edge 2
  EDGE(       0,       4,     0,     1,         31)  // Edge 3
  EDGE(       0,       3,     1,     4,         31)  // Edge 4
  EDGE(       3,       4,     1,     3,         31)  // Edge 5
  EDGE(       2,       4,     2,     3,         31)  // Edge 6
  EDGE(       3,       5,     3,     4,         31)  // Edge 7
  EDGE(       2,       5,     3,     6,         31)  // Edge 8
  EDGE(       1,       5,     5,     6,         31)  // Edge 9
  EDGE(       0,       5,     4,     5,         31)  // Edge 10
  EDGE(       6,       7,     3,     3,         15)  // Edge 11
  EDGE(       7,       8,     3,     3,         12)  // Edge 12
  EDGE(       6,       9,     3,     3,         12)  // Edge 13
  EDGE(       8,       9,     3,     3,         12)  // Edge 14
};

static const Face sidewinderFaces[] PROGMEM =
{
  //    normal_x, normal_y, normal_z, visibility
  FACE(        0,       32,        8,         31)  // Face 0
  FACE(      -12,       47,        6,         31)  // Face 1
  FACE(       12,       47,        6,         31)  // Face 2
  FACE(        0,        0,     -112,         31)  // Face 3
  FACE(      -12,      -47,        6,         31)  // Face 4
  FACE(        0,      -32,        8,         31)  // Face 5
  FACE(       12,      -47,        6,         31)  // Face 6
};

// ----------------------------------------------------------------------
// Mamba
static const Vertex mambaVertices[] PROGMEM =
{
  //        x,    y,    z, face1, face2, face3, face4, visibility
  VERTEX(   0,    0,   64,     0,      1,    2,     3,         31)  // Vertex 0
  VERTEX( -64,   -8,  -32,     0,      2,    4,     4,         31)  // Vertex 1
  VERTEX( -32,    8,  -32,     1,      2,    4,     4,         30)  // Vertex 2
  VERTEX(  32,    8,  -32,     1,      3,    4,     4,         30)  // Vertex 3
  VERTEX(  64,   -8,  -32,     0,      3,    4,     4,         31)  // Vertex 4
  VERTEX(  -4,    4,   16,     1,      1,    1,     1,         14)  // Vertex 5
  VERTEX(   4,    4,   16,     1,      1,    1,     1,         14)  // Vertex 6
  VERTEX(   8,    3,   28,     1,      1,    1,     1,         13)  // Vertex 7
  VERTEX(  -8,    3,   28,     1,      1,    1,     1,         13)  // Vertex 8
  VERTEX( -20,   -4,   16,     0,      0,    0,     0,         20)  // Vertex 9
  VERTEX(  20,   -4,   16,     0,      0,    0,     0,         20)  // Vertex 10
  VERTEX( -24,   -7,  -20,     0,      0,    0,     0,         20)  // Vertex 11
  VERTEX( -16,   -7,  -20,     0,      0,    0,     0,         16)  // Vertex 12
  VERTEX(  16,   -7,  -20,     0,      0,    0,     0,         16)  // Vertex 13
  VERTEX(  24,   -7,  -20,     0,      0,    0,     0,         20)  // Vertex 14
  VERTEX(  -8,    4,  -32,     4,      4,    4,     4,         13)  // Vertex 15
  VERTEX(   8,    4,  -32,     4,      4,    4,     4,         13)  // Vertex 16
  VERTEX(   8,   -4,  -32,     4,      4,    4,     4,         14)  // Vertex 17
  VERTEX(  -8,   -4,  -32,     4,      4,    4,     4,         14)  // Vertex 18
  VERTEX( -32,    4,  -32,     4,      4,    4,     4,          7)  // Vertex 19
  VERTEX(  32,    4,  -32,     4,      4,    4,     4,          7)  // Vertex 20
  VERTEX(  36,   -4,  -32,     4,      4,    4,     4,          7)  // Vertex 21
  VERTEX( -36,   -4,  -32,     4,      4,    4,     4,          7)  // Vertex 22
  VERTEX( -38,    0,  -32,     4,      4,    4,     4,          5)  // Vertex 23
  VERTEX(  38,    0,  -32,     4,      4,    4,     4,          5)  // Vertex 24
};

static const Edge mambaEdges[] PROGMEM =
{
  //    vertex1, vertex2, face1, face2, visibility
  EDGE(       0,       1,     0,     2,         31)  // Edge 0
  EDGE(       0,       4,     0,     3,         31)  // Edge 1
  EDGE(       1,       4,     0,     4,         31)  // Edge 2
  EDGE(       1,       2,     2,     4,         30)  // Edge 3
  EDGE(       2,       3,     1,     4,         30)  // Edge 4
  EDGE(       3,       4,     3,     4,         30)  // Edge 5
  EDGE(       5,       6,     1,     1,         14)  // Edge 6
  EDGE(       6,       7,     1,     1,         12)  // Edge 7
  EDGE(       7,       8,     1,     1,         13)  // Edge 8
  EDGE(       5,       8,     1,     1,         12)  // Edge 9
  EDGE(       9,      11,     0,     0,         20)  // Edge 10
  EDGE(       9,      12,     0,     0,         16)  // Edge 11
  EDGE(      10,      13,     0,     0,         16)  // Edge 12
  EDGE(      10,      14,     0,     0,         20)  // Edge 13
  EDGE(      13,      14,     0,     0,         14)  // Edge 14
  EDGE(      11,      12,     0,     0,         14)  // Edge 15
  EDGE(      15,      16,     4,     4,         13)  // Edge 16
  EDGE(      17,      18,     4,     4,         14)  // Edge 17
  EDGE(      15,      18,     4,     4,         12)  // Edge 18
  EDGE(      16,      17,     4,     4,         12)  // Edge 19
  EDGE(      20,      21,     4,     4,          7)  // Edge 20
  EDGE(      20,      24,     4,     4,          5)  // Edge 21
  EDGE(      21,      24,     4,     4,          5)  // Edge 22
  EDGE(      19,      22,     4,     4,          7)  // Edge 23
  EDGE(      19,      23,     4,     4,          5)  // Edge 24
  EDGE(      22,      23,     4,     4,          5)  // Edge 25
  EDGE(       0,       2,     1,     2,         30)  // Edge 26
  EDGE(       0,       3,     1,     3,         30)  // Edge 27
};

static const Face mambaFaces[] PROGMEM =
{
  //    normal_x, normal_y, normal_z, visibility
  FACE(        0,      -24,        2,         30)  // Face 0
  FACE(        0,       24,        2,         30)  // Face 1
  FACE(      -32,       64,       16,         30)  // Face 2
  FACE(       32,       64,       16,         30)  // Face 3
  FACE(        0,        0,     -127,         30)  // Face 4
};

// ----------------------------------------------------------------------
// Krait
static const Vertex kraitVertices[] PROGMEM =
{
  //        x,    y,    z, face1, face2, face3, face4, visibility
  VERTEX(   0,    0,   96,     1,      0,    3,     2,         31)  // Vertex 0
  VERTEX(   0,   18,  -48,     3,      0,    5,     4,         31)  // Vertex 1
  VERTEX(   0,  -18,  -48,     2,      1,    5,     4,         31)  // Vertex 2
  VERTEX(  90,    0,   -3,     1,      0,    4,     4,         31)  // Vertex 3
  VERTEX( -90,    0,   -3,     3,      2,    5,     5,         31)  // Vertex 4
  VERTEX(  90,    0,   87,     1,      0,    1,     1,         30)  // Vertex 5
  VERTEX( -90,    0,   87,     3,      2,    3,     3,         30)  // Vertex 6
  VERTEX(   0,    5,   53,     0,      0,    3,     3,          9)  // Vertex 7
  VERTEX(   0,    7,   38,     0,      0,    3,     3,          6)  // Vertex 8
  VERTEX( -18,    7,   19,     3,      3,    3,     3,          9)  // Vertex 9
  VERTEX(  18,    7,   19,     0,      0,    0,     0,          9)  // Vertex 10
  VERTEX(  18,   11,  -39,     4,      4,    4,     4,          8)  // Vertex 11
  VERTEX(  18,  -11,  -39,     4,      4,    4,     4,          8)  // Vertex 12
  VERTEX(  36,    0,  -30,     4,      4,    4,     4,          8)  // Vertex 13
  VERTEX( -18,   11,  -39,     5,      5,    5,     5,          8)  // Vertex 14
  VERTEX( -18,  -11,  -39,     5,      5,    5,     5,          8)  // Vertex 15
  VERTEX( -36,    0,  -30,     5,      5,    5,     5,          8)  // Vertex 16
};

static const Edge kraitEdges[] PROGMEM =
{
  //    vertex1, vertex2, face1, face2, visibility
  EDGE(       0,       1,     3,     0,         31)  // Edge 0
  EDGE(       0,       2,     2,     1,         31)  // Edge 1
  EDGE(       0,       3,     1,     0,         31)  // Edge 2
  EDGE(       0,       4,     3,     2,         31)  // Edge 3
  EDGE(       1,       4,     5,     3,         31)  // Edge 4
  EDGE(       4,       2,     5,     2,         31)  // Edge 5
  EDGE(       2,       3,     4,     1,         31)  // Edge 6
  EDGE(       3,       1,     4,     0,         31)  // Edge 7
  EDGE(       3,       5,     1,     0,         30)  // Edge 8
  EDGE(       4,       6,     3,     2,         30)  // Edge 9
  EDGE(       1,       2,     5,     4,          8)  // Edge 10
  EDGE(       7,      10,     0,     0,          9)  // Edge 11
  EDGE(       8,      10,     0,     0,          6)  // Edge 12
  EDGE(       7,       9,     3,     3,          9)  // Edge 13
  EDGE(       8,       9,     3,     3,          6)  // Edge 14
  EDGE(      11,      13,     4,     4,          8)  // Edge 15
  EDGE(      13,      12,     4,     4,          8)  // Edge 16
  EDGE(      12,      11,     4,     4,          7)  // Edge 17
  EDGE(      14,      15,     5,     5,          7)  // Edge 18
  EDGE(      15,      16,     5,     5,          8)  // Edge 19
  EDGE(      16,      14,     5,     5,          8)  // Edge 20
};

static const Face kraitFaces[] PROGMEM =
{
  //    normal_x, normal_y, normal_z, visibility
  FACE(        3,       24,        3,         31)  // Face 0
  FACE(        3,      -24,        3,         31)  // Face 1
  FACE(       -3,      -24,        3,         31)  // Face 2
  FACE(       -3,       24,        3,         31)  // Face 3
  FACE(       38,        0,      -77,         31)  // Face 4
  FACE(      -38,        0,      -77,         31)  // Face 5
};

// ----------------------------------------------------------------------
// Fer De Lance
static const Vertex ferDeLanceVertices[] PROGMEM =
{
  //        x,    y,    z, face1, face2, face3, face4, visibility
  VERTEX(   0,  -14,  108,     1,      0,    9,     5,         31)  // Vertex 0
  VERTEX( -40,  -14,   -4,     2,      1,    9,     9,         31)  // Vertex 1
  VERTEX( -12,  -14,  -52,     3,      2,    9,     9,         31)  // Vertex 2
  VERTEX(  12,  -14,  -52,     4,      3,    9,     9,         31)  // Vertex 3
  VERTEX(  40,  -14,   -4,     5,      4,    9,     9,         31)  // Vertex 4
  VERTEX( -40,   14,   -4,     1,      0,    6,     2,         28)  // Vertex 5
  VERTEX( -12,    2,  -52,     3,      2,    7,     6,         28)  // Vertex 6
  VERTEX(  12,    2,  -52,     4,      3,    8,     7,         28)  // Vertex 7
  VERTEX(  40,   14,   -4,     4,      0,    8,     5,         28)  // Vertex 8
  VERTEX(   0,   18,  -20,     6,      0,    8,     7,         15)  // Vertex 9
  VERTEX(  -3,  -11,   97,     0,      0,    0,     0,         11)  // Vertex 10
  VERTEX( -26,    8,   18,     0,      0,    0,     0,          9)  // Vertex 11
  VERTEX( -16,   14,   -4,     0,      0,    0,     0,         11)  // Vertex 12
  VERTEX(   3,  -11,   97,     0,      0,    0,     0,         11)  // Vertex 13
  VERTEX(  26,    8,   18,     0,      0,    0,     0,          9)  // Vertex 14
  VERTEX(  16,   14,   -4,     0,      0,    0,     0,         11)  // Vertex 15
  VERTEX(   0,  -14,  -20,     9,      9,    9,     9,         12)  // Vertex 16
  VERTEX( -14,  -14,   44,     9,      9,    9,     9,         12)  // Vertex 17
  VERTEX(  14,  -14,   44,     9,      9,    9,     9,         12)  // Vertex 18
};

static const Edge ferDeLanceEdges[] PROGMEM =
{
  //    vertex1, vertex2, face1, face2, visibility
  EDGE(       0,       1,     9,     1,         31)  // Edge 0
  EDGE(       1,       2,     9,     2,         31)  // Edge 1
  EDGE(       2,       3,     9,     3,         31)  // Edge 2
  EDGE(       3,       4,     9,     4,         31)  // Edge 3
  EDGE(       0,       4,     9,     5,         31)  // Edge 4
  EDGE(       0,       5,     1,     0,         28)  // Edge 5
  EDGE(       5,       6,     6,     2,         28)  // Edge 6
  EDGE(       6,       7,     7,     3,         28)  // Edge 7
  EDGE(       7,       8,     8,     4,         28)  // Edge 8
  EDGE(       0,       8,     5,     0,         28)  // Edge 9
  EDGE(       5,       9,     6,     0,         15)  // Edge 10
  EDGE(       6,       9,     7,     6,         11)  // Edge 11
  EDGE(       7,       9,     8,     7,         11)  // Edge 12
  EDGE(       8,       9,     8,     0,         15)  // Edge 13
  EDGE(       1,       5,     2,     1,         14)  // Edge 14
  EDGE(       2,       6,     3,     2,         14)  // Edge 15
  EDGE(       3,       7,     4,     3,         14)  // Edge 16
  EDGE(       4,       8,     5,     4,         14)  // Edge 17
  EDGE(      10,      11,     0,     0,          8)  // Edge 18
  EDGE(      11,      12,     0,     0,          9)  // Edge 19
  EDGE(      10,      12,     0,     0,         11)  // Edge 20
  EDGE(      13,      14,     0,     0,          8)  // Edge 21
  EDGE(      14,      15,     0,     0,          9)  // Edge 22
  EDGE(      13,      15,     0,     0,         11)  // Edge 23
  EDGE(      16,      17,     9,     9,         12)  // Edge 24
  EDGE(      16,      18,     9,     9,         12)  // Edge 25
  EDGE(      17,      18,     9,     9,          8)  // Edge 26
};

static const Face ferDeLanceFaces[] PROGMEM =
{
  //    normal_x, normal_y, normal_z, visibility
  FACE(        0,       24,        6,         28)  // Face 0
  FACE(      -68,        0,       24,         31)  // Face 1
  FACE(      -63,        0,      -37,         31)  // Face 2
  FACE(        0,        0,     -104,         31)  // Face 3
  FACE(       63,        0,      -37,         31)  // Face 4
  FACE(       68,        0,       24,         31)  // Face 5
  FACE(      -12,       46,      -19,         28)  // Face 6
  FACE(        0,       45,      -22,         28)  // Face 7
  FACE(       12,       46,      -19,         28)  // Face 8
  FACE(        0,      -28,        0,         31)  // Face 9
};

// ----------------------------------------------------------------------
// Cobra Mk 3
static const Vertex cobraMk3Vertices[] PROGMEM =
{
  //        x,    y,    z, face1, face2, face3, face4, visibility
  VERTEX(  32,    0,   76,    15,     15,   15,    15,         31)  // Vertex 0
  VERTEX( -32,    0,   76,    15,     15,   15,    15,         31)  // Vertex 1
  VERTEX(   0,   26,   24,    15,     15,   15,    15,         31)  // Vertex 2
  VERTEX(-120,   -3,   -8,     3,      7,   10,    10,         31)  // Vertex 3
  VERTEX( 120,   -3,   -8,     4,      8,   12,    12,         31)  // Vertex 4
  VERTEX( -88,   16,  -40,    15,     15,   15,    15,         31)  // Vertex 5
  VERTEX(  88,   16,  -40,    15,     15,   15,    15,         31)  // Vertex 6
  VERTEX( 128,   -8,  -40,     8,      9,   12,    12,         31)  // Vertex 7
  VERTEX(-128,   -8,  -40,     7,      9,   10,    10,         31)  // Vertex 8
  VERTEX(   0,   26,  -40,     5,      6,    9,     9,         31)  // Vertex 9
  VERTEX( -32,  -24,  -40,     9,     10,   11,    11,         31)  // Vertex 10
  VERTEX(  32,  -24,  -40,     9,     11,   12,    12,         31)  // Vertex 11
  VERTEX( -36,    8,  -40,     9,      9,    9,     9,         20)  // Vertex 12
  VERTEX(  -8,   12,  -40,     9,      9,    9,     9,         20)  // Vertex 13
  VERTEX(   8,   12,  -40,     9,      9,    9,     9,         20)  // Vertex 14
  VERTEX(  36,    8,  -40,     9,      9,    9,     9,         20)  // Vertex 15
  VERTEX(  36,  -12,  -40,     9,      9,    9,     9,         20)  // Vertex 16
  VERTEX(   8,  -16,  -40,     9,      9,    9,     9,         20)  // Vertex 17
  VERTEX(  -8,  -16,  -40,     9,      9,    9,     9,         20)  // Vertex 18
  VERTEX( -36,  -12,  -40,     9,      9,    9,     9,         20)  // Vertex 19
  VERTEX(   0,    0,   76,     0,     11,   11,    11,          6)  // Vertex 20
  VERTEX(   0,    0,   90,     0,     11,   11,    11,         31)  // Vertex 21
  VERTEX( -80,   -6,  -40,     9,      9,    9,     9,          8)  // Vertex 22
  VERTEX( -80,    6,  -40,     9,      9,    9,     9,          8)  // Vertex 23
  VERTEX( -88,    0,  -40,     9,      9,    9,     9,          6)  // Vertex 24
  VERTEX(  80,    6,  -40,     9,      9,    9,     9,          8)  // Vertex 25
  VERTEX(  88,    0,  -40,     9,      9,    9,     9,          6)  // Vertex 26
  VERTEX(  80,   -6,  -40,     9,      9,    9,     9,          8)  // Vertex 27
};

static const Edge cobraMk3Edges[] PROGMEM =
{
  //    vertex1, vertex2, face1, face2, visibility
  EDGE(       0,       1,     0,    11,         31)  // Edge 0
  EDGE(       0,       4,     4,    12,         31)  // Edge 1
  EDGE(       1,       3,     3,    10,         31)  // Edge 2
  EDGE(       3,       8,     7,    10,         31)  // Edge 3
  EDGE(       4,       7,     8,    12,         31)  // Edge 4
  EDGE(       6,       7,     8,     9,         31)  // Edge 5
  EDGE(       6,       9,     6,     9,         31)  // Edge 6
  EDGE(       5,       9,     5,     9,         31)  // Edge 7
  EDGE(       5,       8,     7,     9,         31)  // Edge 8
  EDGE(       2,       5,     1,     5,         31)  // Edge 9
  EDGE(       2,       6,     2,     6,         31)  // Edge 10
  EDGE(       3,       5,     3,     7,         31)  // Edge 11
  EDGE(       4,       6,     4,     8,         31)  // Edge 12
  EDGE(       1,       2,     0,     1,         31)  // Edge 13
  EDGE(       0,       2,     0,     2,         31)  // Edge 14
  EDGE(       8,      10,     9,    10,         31)  // Edge 15
  EDGE(      10,      11,     9,    11,         31)  // Edge 16
  EDGE(       7,      11,     9,    12,         31)  // Edge 17
  EDGE(       1,      10,    10,    11,         31)  // Edge 18
  EDGE(       0,      11,    11,    12,         31)  // Edge 19
  EDGE(       1,       5,     1,     3,         29)  // Edge 20
  EDGE(       0,       6,     2,     4,         29)  // Edge 21
  EDGE(      20,      21,     0,    11,          6)  // Edge 22
  EDGE(      12,      13,     9,     9,         20)  // Edge 23
  EDGE(      18,      19,     9,     9,         20)  // Edge 24
  EDGE(      14,      15,     9,     9,         20)  // Edge 25
  EDGE(      16,      17,     9,     9,         20)  // Edge 26
  EDGE(      15,      16,     9,     9,         19)  // Edge 27
  EDGE(      14,      17,     9,     9,         17)  // Edge 28
  EDGE(      13,      18,     9,     9,         19)  // Edge 29
  EDGE(      12,      19,     9,     9,         19)  // Edge 30
  EDGE(       2,       9,     5,     6,         30)  // Edge 31
  EDGE(      22,      24,     9,     9,          6)  // Edge 32
  EDGE(      23,      24,     9,     9,          6)  // Edge 33
  EDGE(      22,      23,     9,     9,          8)  // Edge 34
  EDGE(      25,      26,     9,     9,          6)  // Edge 35
  EDGE(      26,      27,     9,     9,          6)  // Edge 36
  EDGE(      25,      27,     9,     9,          8)  // Edge 37
};

static const Face cobraMk3Faces[] PROGMEM =
{
  //    normal_x, normal_y, normal_z, visibility
  FACE(        0,       62,       31,         31)  // Face 0
  FACE(      -18,       55,       16,         31)  // Face 1
  FACE(       18,       55,       16,         31)  // Face 2
  FACE(      -16,       52,       14,         31)  // Face 3
  FACE(       16,       52,       14,         31)  // Face 4
  FACE(      -14,       47,        0,         31)  // Face 5
  FACE(       14,       47,        0,         31)  // Face 6
  FACE(      -61,      102,        0,         31)  // Face 7
  FACE(       61,      102,        0,         31)  // Face 8
  FACE(        0,        0,      -80,         31)  // Face 9
  FACE(       -7,      -42,        9,         31)  // Face 10
  FACE(        0,      -30,        6,         31)  // Face 11
  FACE(        7,      -42,        9,         31)  // Face 12
};

// ----------------------------------------------------------------------
// Adder
static const Vertex adderVertices[] PROGMEM =
{
  //        x,    y,    z, face1, face2, face3, face4, visibility
  VERTEX( -18,    0,   40,     1,      0,   12,    11,         31)  // Vertex 0
  VERTEX(  18,    0,   40,     1,      0,    3,     2,         31)  // Vertex 1
  VERTEX(  30,    0,  -24,     3,      2,    5,     4,         31)  // Vertex 2
  VERTEX(  30,    0,  -40,     5,      4,    6,     6,         31)  // Vertex 3
  VERTEX(  18,   -7,  -40,     6,      5,   14,     7,         31)  // Vertex 4
  VERTEX( -18,   -7,  -40,     8,      7,   14,    10,         31)  // Vertex 5
  VERTEX( -30,    0,  -40,     9,      8,   10,    10,         31)  // Vertex 6
  VERTEX( -30,    0,  -24,    10,      9,   12,    11,         31)  // Vertex 7
  VERTEX( -18,    7,  -40,     8,      7,   13,     9,         31)  // Vertex 8
  VERTEX(  18,    7,  -40,     6,      4,   13,     7,         31)  // Vertex 9
  VERTEX( -18,    7,   13,     9,      0,   13,    11,         31)  // Vertex 10
  VERTEX(  18,    7,   13,     2,      0,   13,     4,         31)  // Vertex 11
  VERTEX( -18,   -7,   13,    10,      1,   14,    12,         31)  // Vertex 12
  VERTEX(  18,   -7,   13,     3,      1,   14,     5,         31)  // Vertex 13
  VERTEX( -11,    3,   29,     0,      0,    0,     0,          5)  // Vertex 14
  VERTEX(  11,    3,   29,     0,      0,    0,     0,          5)  // Vertex 15
  VERTEX(  11,    4,   24,     0,      0,    0,     0,          4)  // Vertex 16
  VERTEX( -11,    4,   24,     0,      0,    0,     0,          4)  // Vertex 17
};

static const Edge adderEdges[] PROGMEM =
{
  //    vertex1, vertex2, face1, face2, visibility
  EDGE(       0,       1,     1,     0,         31)  // Edge 0
  EDGE(       1,       2,     3,     2,          7)  // Edge 1
  EDGE(       2,       3,     5,     4,         31)  // Edge 2
  EDGE(       3,       4,     6,     5,         31)  // Edge 3
  EDGE(       4,       5,    14,     7,         31)  // Edge 4
  EDGE(       5,       6,    10,     8,         31)  // Edge 5
  EDGE(       6,       7,    10,     9,         31)  // Edge 6
  EDGE(       7,       0,    12,    11,          7)  // Edge 7
  EDGE(       3,       9,     6,     4,         31)  // Edge 8
  EDGE(       9,       8,    13,     7,         31)  // Edge 9
  EDGE(       8,       6,     9,     8,         31)  // Edge 10
  EDGE(       0,      10,    11,     0,         31)  // Edge 11
  EDGE(       7,      10,    11,     9,         31)  // Edge 12
  EDGE(       1,      11,     2,     0,         31)  // Edge 13
  EDGE(       2,      11,     4,     2,         31)  // Edge 14
  EDGE(       0,      12,    12,     1,         31)  // Edge 15
  EDGE(       7,      12,    12,    10,         31)  // Edge 16
  EDGE(       1,      13,     3,     1,         31)  // Edge 17
  EDGE(       2,      13,     5,     3,         31)  // Edge 18
  EDGE(      10,      11,    13,     0,         31)  // Edge 19
  EDGE(      12,      13,    14,     1,         31)  // Edge 20
  EDGE(       8,      10,    13,     9,         31)  // Edge 21
  EDGE(       9,      11,    13,     4,         31)  // Edge 22
  EDGE(       5,      12,    14,    10,         31)  // Edge 23
  EDGE(       4,      13,    14,     5,         31)  // Edge 24
  EDGE(      14,      15,     0,     0,          5)  // Edge 25
  EDGE(      15,      16,     0,     0,          3)  // Edge 26
  EDGE(      16,      17,     0,     0,          4)  // Edge 27
  EDGE(      17,      14,     0,     0,          3)  // Edge 28
};

static const Face adderFaces[] PROGMEM =
{
  //    normal_x, normal_y, normal_z, visibility
  FACE(        0,       39,       10,         31)  // Face 0
  FACE(        0,      -39,       10,         31)  // Face 1
  FACE(       69,       50,       13,         31)  // Face 2
  FACE(       69,      -50,       13,         31)  // Face 3
  FACE(       30,       52,        0,         31)  // Face 4
  FACE(       30,      -52,        0,         31)  // Face 5
  FACE(        0,        0,     -160,         31)  // Face 6
  FACE(        0,        0,     -160,         31)  // Face 7
  FACE(        0,        0,     -160,         31)  // Face 8
  FACE(      -30,       52,        0,         31)  // Face 9
  FACE(      -30,      -52,        0,         31)  // Face 10
  FACE(      -69,       50,       13,         31)  // Face 11
  FACE(      -69,      -50,       13,         31)  // Face 12
  FACE(        0,       28,        0,         31)  // Face 13
  FACE(        0,      -28,        0,         31)  // Face 14
};

// ----------------------------------------------------------------------
// Boa
static const Vertex boaVertices[] PROGMEM =
{
  //        x,    y,    z, face1, face2, face3, face4, visibility
  VERTEX(   0,    0,   93,    15,     15,   15,    15,         31)  // Vertex 0
  VERTEX(   0,   40,  -87,     2,      0,    3,     3,         24)  // Vertex 1
  VERTEX(  38,  -25,  -99,     1,      0,    4,     4,         24)  // Vertex 2
  VERTEX( -38,  -25,  -99,     2,      1,    5,     5,         24)  // Vertex 3
  VERTEX( -38,   40,  -59,     3,      2,    9,     6,         31)  // Vertex 4
  VERTEX(  38,   40,  -59,     3,      0,   11,     6,         31)  // Vertex 5
  VERTEX(  62,    0,  -67,     4,      0,   11,     8,         31)  // Vertex 6
  VERTEX(  24,  -65,  -79,     4,      1,   10,     8,         31)  // Vertex 7
  VERTEX( -24,  -65,  -79,     5,      1,   10,     7,         31)  // Vertex 8
  VERTEX( -62,    0,  -67,     5,      2,    9,     7,         31)  // Vertex 9
  VERTEX(   0,    7, -107,     2,      0,   10,    10,         22)  // Vertex 10
  VERTEX(  13,   -9, -107,     1,      0,   10,    10,         22)  // Vertex 11
  VERTEX( -13,   -9, -107,     2,      1,   12,    12,         22)  // Vertex 12
};

static const Edge boaEdges[] PROGMEM =
{
  //    vertex1, vertex2, face1, face2, visibility
  EDGE(       0,       5,    11,     6,         31)  // Edge 0
  EDGE(       0,       7,    10,     8,         31)  // Edge 1
  EDGE(       0,       9,     9,     7,         31)  // Edge 2
  EDGE(       0,       4,     9,     6,         29)  // Edge 3
  EDGE(       0,       6,    11,     8,         29)  // Edge 4
  EDGE(       0,       8,    10,     7,         29)  // Edge 5
  EDGE(       4,       5,     6,     3,         31)  // Edge 6
  EDGE(       5,       6,    11,     0,         31)  // Edge 7
  EDGE(       6,       7,     8,     4,         31)  // Edge 8
  EDGE(       7,       8,    10,     1,         31)  // Edge 9
  EDGE(       8,       9,     7,     5,         31)  // Edge 10
  EDGE(       4,       9,     9,     2,         31)  // Edge 11
  EDGE(       1,       4,     3,     2,         24)  // Edge 12
  EDGE(       1,       5,     3,     0,         24)  // Edge 13
  EDGE(       3,       9,     5,     2,         24)  // Edge 14
  EDGE(       3,       8,     5,     1,         24)  // Edge 15
  EDGE(       2,       6,     4,     0,         24)  // Edge 16
  EDGE(       2,       7,     4,     1,         24)  // Edge 17
  EDGE(       1,      10,     2,     0,         22)  // Edge 18
  EDGE(       2,      11,     1,     0,         22)  // Edge 19
  EDGE(       3,      12,     2,     1,         22)  // Edge 20
  EDGE(      10,      11,    12,     0,         14)  // Edge 21
  EDGE(      11,      12,    12,     1,         14)  // Edge 22
  EDGE(      12,      10,    12,     2,         14)  // Edge 23
};

static const Face boaFaces[] PROGMEM =
{
  //    normal_x, normal_y, normal_z, visibility
  FACE(       43,       37,      -60,         31)  // Face 0
  FACE(        0,      -45,      -89,         31)  // Face 1
  FACE(      -43,       37,      -60,         31)  // Face 2
  FACE(        0,       40,        0,         31)  // Face 3
  FACE(       62,      -32,      -20,         31)  // Face 4
  FACE(      -62,      -32,      -20,         31)  // Face 5
  FACE(        0,       23,        6,         31)  // Face 6
  FACE(      -23,      -15,        9,         31)  // Face 7
  FACE(       23,      -15,        9,         31)  // Face 8
  FACE(      -26,       13,       10,         31)  // Face 9
  FACE(        0,      -31,       12,         31)  // Face 10
  FACE(       26,       13,       10,         31)  // Face 11
  FACE(        0,        0,     -107,         14)  // Face 12
};

// ----------------------------------------------------------------------
// Constrictor
static const Vertex constrictorVertices[] PROGMEM =
{
  //        x,    y,    z, face1, face2, face3, face4, visibility
  VERTEX(  20,   -7,   80,     2,      0,    9,     9,         31)  // Vertex 0
  VERTEX( -20,   -7,   80,     1,      0,    9,     9,         31)  // Vertex 1
  VERTEX( -54,   -7,   40,     4,      1,    9,     9,         31)  // Vertex 2
  VERTEX( -54,   -7,  -40,     5,      4,    9,     8,         31)  // Vertex 3
  VERTEX( -20,   13,  -40,     6,      5,    8,     8,         31)  // Vertex 4
  VERTEX(  20,   13,  -40,     7,      6,    8,     8,         31)  // Vertex 5
  VERTEX(  54,   -7,  -40,     7,      3,    9,     8,         31)  // Vertex 6
  VERTEX(  54,   -7,   40,     3,      2,    9,     9,         31)  // Vertex 7
  VERTEX(  20,   13,    5,    15,     15,   15,    15,         31)  // Vertex 8
  VERTEX( -20,   13,    5,    15,     15,   15,    15,         31)  // Vertex 9
  VERTEX(  20,   -7,   62,     9,      9,    9,     9,         18)  // Vertex 10
  VERTEX( -20,   -7,   62,     9,      9,    9,     9,         18)  // Vertex 11
  VERTEX(  25,   -7,  -25,     9,      9,    9,     9,         18)  // Vertex 12
  VERTEX( -25,   -7,  -25,     9,      9,    9,     9,         18)  // Vertex 13
  VERTEX(  15,   -7,  -15,     9,      9,    9,     9,         10)  // Vertex 14
  VERTEX( -15,   -7,  -15,     9,      9,    9,     9,         10)  // Vertex 15
  VERTEX(   0,   -7,    0,    15,      9,    1,     0,          0)  // Vertex 16
};

static const Edge constrictorEdges[] PROGMEM =
{
  //    vertex1, vertex2, face1, face2, visibility
  EDGE(       0,       1,     9,     0,         31)  // Edge 0
  EDGE(       1,       2,     9,     1,         31)  // Edge 1
  EDGE(       1,       9,     1,     0,         31)  // Edge 2
  EDGE(       0,       8,     2,     0,         31)  // Edge 3
  EDGE(       0,       7,     9,     2,         31)  // Edge 4
  EDGE(       7,       8,     3,     2,         31)  // Edge 5
  EDGE(       2,       9,     4,     1,         31)  // Edge 6
  EDGE(       2,       3,     9,     4,         31)  // Edge 7
  EDGE(       6,       7,     9,     3,         31)  // Edge 8
  EDGE(       6,       8,     7,     3,         31)  // Edge 9
  EDGE(       5,       8,     7,     6,         31)  // Edge 10
  EDGE(       4,       9,     6,     5,         31)  // Edge 11
  EDGE(       3,       9,     5,     4,         31)  // Edge 12
  EDGE(       3,       4,     8,     5,         31)  // Edge 13
  EDGE(       4,       5,     8,     6,         31)  // Edge 14
  EDGE(       5,       6,     8,     7,         31)  // Edge 15
  EDGE(       3,       6,     9,     8,         31)  // Edge 16
  EDGE(       8,       9,     6,     0,         31)  // Edge 17
  EDGE(      10,      12,     9,     9,         18)  // Edge 18
  EDGE(      12,      14,     9,     9,          5)  // Edge 19
  EDGE(      14,      10,     9,     9,         10)  // Edge 20
  EDGE(      11,      15,     9,     9,         10)  // Edge 21
  EDGE(      13,      15,     9,     9,          5)  // Edge 22
  EDGE(      11,      13,     9,     9,         18)  // Edge 23
};

static const Face constrictorFaces[] PROGMEM =
{
  //    normal_x, normal_y, normal_z, visibility
  FACE(        0,       55,       15,         31)  // Face 0
  FACE(      -24,       75,       20,         31)  // Face 1
  FACE(       24,       75,       20,         31)  // Face 2
  FACE(       44,       75,        0,         31)  // Face 3
  FACE(      -44,       75,        0,         31)  // Face 4
  FACE(      -44,       75,        0,         31)  // Face 5
  FACE(        0,       53,        0,         31)  // Face 6
  FACE(       44,       75,        0,         31)  // Face 7
  FACE(        0,        0,     -160,         31)  // Face 8
  FACE(        0,      -27,        0,         31)  // Face 9
};

// ----------------------------------------------------------------------
// Moray
static const Vertex morayVertices[] PROGMEM =
{
  //        x,    y,    z, face1, face2, face3, face4, visibility
  VERTEX(  15,    0,   65,     2,      0,    8,     7,         31)  // Vertex 0
  VERTEX( -15,    0,   65,     1,      0,    7,     6,         31)  // Vertex 1
  VERTEX(   0,   18,  -40,    15,     15,   15,    15,         17)  // Vertex 2
  VERTEX( -60,    0,    0,     3,      1,    6,     6,         31)  // Vertex 3
  VERTEX(  60,    0,    0,     5,      2,    8,     8,         31)  // Vertex 4
  VERTEX(  30,  -27,  -10,     5,      4,    8,     7,         24)  // Vertex 5
  VERTEX( -30,  -27,  -10,     4,      3,    7,     6,         24)  // Vertex 6
  VERTEX(  -9,   -4,  -25,     4,      4,    4,     4,          7)  // Vertex 7
  VERTEX(   9,   -4,  -25,     4,      4,    4,     4,          7)  // Vertex 8
  VERTEX(   0,  -18,  -16,     4,      4,    4,     4,          7)  // Vertex 9
  VERTEX(  13,    3,   49,     0,      0,    0,     0,          5)  // Vertex 10
  VERTEX(   6,    0,   65,     0,      0,    0,     0,          5)  // Vertex 11
  VERTEX( -13,    3,   49,     0,      0,    0,     0,          5)  // Vertex 12
  VERTEX(  -6,    0,   65,     0,      0,    0,     0,          5)  // Vertex 13
};

static const Edge morayEdges[] PROGMEM =
{
  //    vertex1, vertex2, face1, face2, visibility
  EDGE(       0,       1,     7,     0,         31)  // Edge 0
  EDGE(       1,       3,     6,     1,         31)  // Edge 1
  EDGE(       3,       6,     6,     3,         24)  // Edge 2
  EDGE(       5,       6,     7,     4,         24)  // Edge 3
  EDGE(       4,       5,     8,     5,         24)  // Edge 4
  EDGE(       0,       4,     8,     2,         31)  // Edge 5
  EDGE(       1,       6,     7,     6,         15)  // Edge 6
  EDGE(       0,       5,     8,     7,         15)  // Edge 7
  EDGE(       0,       2,     2,     0,         15)  // Edge 8
  EDGE(       1,       2,     1,     0,         15)  // Edge 9
  EDGE(       2,       3,     3,     1,         17)  // Edge 10
  EDGE(       2,       4,     5,     2,         17)  // Edge 11
  EDGE(       2,       5,     5,     4,         13)  // Edge 12
  EDGE(       2,       6,     4,     3,         13)  // Edge 13
  EDGE(       7,       8,     4,     4,          5)  // Edge 14
  EDGE(       7,       9,     4,     4,          7)  // Edge 15
  EDGE(       8,       9,     4,     4,          7)  // Edge 16
  EDGE(      10,      11,     0,     0,          5)  // Edge 17
  EDGE(      12,      13,     0,     0,          5)  // Edge 18
};

static const Face morayFaces[] PROGMEM =
{
  //    normal_x, normal_y, normal_z, visibility
  FACE(        0,       43,        7,         31)  // Face 0
  FACE(      -10,       49,        7,         31)  // Face 1
  FACE(       10,       49,        7,         31)  // Face 2
  FACE(      -59,      -28,     -101,         24)  // Face 3
  FACE(        0,      -52,      -78,         24)  // Face 4
  FACE(       59,      -28,     -101,         24)  // Face 5
  FACE(      -72,      -99,       50,         31)  // Face 6
  FACE(        0,      -83,       30,         31)  // Face 7
  FACE(       72,      -99,       50,         31)  // Face 8
};

// ----------------------------------------------------------------------
// Anaconda
static const Vertex anacondaVertices[] PROGMEM =
{
  //        x,    y,    z, face1, face2, face3, face4, visibility
  VERTEX(   0,    7,  -58,     1,      0,    5,     5,         30)  // Vertex 0
  VERTEX( -43,  -13,  -37,     1,      0,    2,     2,         30)  // Vertex 1
  VERTEX( -26,  -47,   -3,     2,      0,    3,     3,         30)  // Vertex 2
  VERTEX(  26,  -47,   -3,     3,      0,    4,     4,         30)  // Vertex 3
  VERTEX(  43,  -13,  -37,     4,      0,    5,     5,         30)  // Vertex 4
  VERTEX(   0,   48,  -49,     5,      1,    6,     6,         30)  // Vertex 5
  VERTEX( -69,   15,  -15,     2,      1,    7,     7,         30)  // Vertex 6
  VERTEX( -43,  -39,   40,     3,      2,    8,     8,         31)  // Vertex 7
  VERTEX(  43,  -39,   40,     4,      3,    9,     9,         31)  // Vertex 8
  VERTEX(  69,   15,  -15,     5,      4,   10,    10,         30)  // Vertex 9
  VERTEX( -43,   53,  -23,    15,     15,   15,    15,         31)  // Vertex 10
  VERTEX( -69,   -1,   32,     7,      2,    8,     8,         31)  // Vertex 11
  VERTEX(   0,    0,  254,    15,     15,   15,    15,         31)  // Vertex 12
  VERTEX(  69,   -1,   32,     9,      4,   10,    10,         31)  // Vertex 13
  VERTEX(  43,   53,  -23,    15,     15,   15,    15,         31)  // Vertex 14
};

static const Edge anacondaEdges[] PROGMEM =
{
  //    vertex1, vertex2, face1, face2, visibility
  EDGE(       0,       1,     1,     0,         30)  // Edge 0
  EDGE(       1,       2,     2,     0,         30)  // Edge 1
  EDGE(       2,       3,     3,     0,         30)  // Edge 2
  EDGE(       3,       4,     4,     0,         30)  // Edge 3
  EDGE(       0,       4,     5,     0,         30)  // Edge 4
  EDGE(       0,       5,     5,     1,         29)  // Edge 5
  EDGE(       1,       6,     2,     1,         29)  // Edge 6
  EDGE(       2,       7,     3,     2,         29)  // Edge 7
  EDGE(       3,       8,     4,     3,         29)  // Edge 8
  EDGE(       4,       9,     5,     4,         29)  // Edge 9
  EDGE(       5,      10,     6,     1,         30)  // Edge 10
  EDGE(       6,      10,     7,     1,         30)  // Edge 11
  EDGE(       6,      11,     7,     2,         30)  // Edge 12
  EDGE(       7,      11,     8,     2,         30)  // Edge 13
  EDGE(       7,      12,     8,     3,         31)  // Edge 14
  EDGE(       8,      12,     9,     3,         31)  // Edge 15
  EDGE(       8,      13,     9,     4,         30)  // Edge 16
  EDGE(       9,      13,    10,     4,         30)  // Edge 17
  EDGE(       9,      14,    10,     5,         30)  // Edge 18
  EDGE(       5,      14,     6,     5,         30)  // Edge 19
  EDGE(      10,      14,    11,     6,         30)  // Edge 20
  EDGE(      10,      12,    11,     7,         31)  // Edge 21
  EDGE(      11,      12,     8,     7,         31)  // Edge 22
  EDGE(      12,      13,    10,     9,         31)  // Edge 23
  EDGE(      12,      14,    11,    10,         31)  // Edge 24
};

static const Face anacondaFaces[] PROGMEM =
{
  //    normal_x, normal_y, normal_z, visibility
  FACE(        0,      -51,      -49,         30)  // Face 0
  FACE(      -51,       18,      -87,         30)  // Face 1
  FACE(      -77,      -57,      -19,         30)  // Face 2
  FACE(        0,      -90,       16,         31)  // Face 3
  FACE(       77,      -57,      -19,         30)  // Face 4
  FACE(       51,       18,      -87,         30)  // Face 5
  FACE(        0,      111,      -20,         30)  // Face 6
  FACE(      -97,       72,       24,         31)  // Face 7
  FACE(     -108,      -68,       34,         31)  // Face 8
  FACE(      108,      -68,       34,         31)  // Face 9
  FACE(       97,       72,       24,         31)  // Face 10
  FACE(        0,       94,       18,         31)  // Face 11
};


// ----------------------------------------------------------------------
// Clock "ship"
#ifdef RTC_I2C_ADDRESS
#define W 70 // half base width
#define V 50 // half top width
#define D 20 // half depth
#define H 40 // half height
#define Q 10 // half back offset

static const Vertex clockVertices[] PROGMEM =
{
  //        x,    y,    z, face1, face2, face3, face4, visibility
  VERTEX(  -W,   -H,   +D,     0,      1,    4,     4,         31)  // Vertex 0,  front-bottom-left
  VERTEX(  -V,   +H,   +D,     0,      1,    2,     2,         31)  // Vertex 1,  front-top-left
  VERTEX(  +V,   +H,   +D,     0,      2,    3,     3,         31)  // Vertex 2,  front-top-right
  VERTEX(  +W,   -H,   +D,     0,      3,    4,     4,         31)  // Vertex 3,  front-bottom-right
  VERTEX(  -Q,   -Q,   -D,     1,      2,    3,     4,         31)  // Vertex 4,  back-left
  VERTEX(  +Q,   -Q,   -D,     1,      2,    3,     4,         31)  // Vertex 5,  back-right
};

static const Edge clockEdges[] PROGMEM =
{
  //    vertex1, vertex2, face1, face2, visibility
  EDGE(       0,       1,     0,     1,         31)  // Edge 0,  front-left
  EDGE(       1,       2,     0,     2,         31)  // Edge 1,  front-top
  EDGE(       2,       3,     0,     3,         31)  // Edge 2,  front-right
  EDGE(       3,       0,     0,     4,         31)  // Edge 3,  front-bottom
  EDGE(       0,       4,     1,     4,         31)  // Edge 4,  back-left-bottom
  EDGE(       1,       4,     1,     2,         31)  // Edge 5,  bsck-left-top
  EDGE(       2,       5,     2,     3,         31)  // Edge 6,  back-right-top
  EDGE(       3,       5,     3,     4,         31)  // Edge 7,  back-right-bottom
  EDGE(       4,       5,     2,     4,         31)  // Edge 8,  back-horz
};

static const Face clockFaces[] PROGMEM =
{
  //    normal_x, normal_y, normal_z, visibility
  FACE(        0,        0,       +1,         31)  // Face 0, front
  FACE(       -1,       +1,       -1,         31)  // Face 1, left
  FACE(       +1,        0,       -1,         31)  // Face 2, top
  FACE(       +1,       +1,       -1,         31)  // Face 3, right
  FACE(       -1,       -1,       -1,         31)  // Face 4, bottom
};
#endif

static const Details shipList[LAST_SHIP] PROGMEM =
{
  SHIP_ARRAYS(adder),
  SHIP_ARRAYS(anaconda),
  SHIP_ARRAYS(boa),
  SHIP_ARRAYS(cobraMk3),
  SHIP_ARRAYS(constrictor),
  SHIP_ARRAYS(ferDeLance),
  SHIP_ARRAYS(krait),
  SHIP_ARRAYS(mamba),
  SHIP_ARRAYS(moray),
  SHIP_ARRAYS(python),
  SHIP_ARRAYS(sidewinder),
  SHIP_ARRAYS(viper),
#ifdef RTC_I2C_ADDRESS
  SHIP_ARRAYS(clock),
#endif  
};

void GetDetails(Type type, Details& details)
{
  // Copy ship details out of PROGMEM
  memcpy_P(&details, shipList + type, sizeof(Details));
}

// multi-string with the ship names
extern const char NameMultiStr_PGM[] PROGMEM =
  TEXT_MSTR("Adder")
  TEXT_MSTR("Anaconda")
  TEXT_MSTR("Boa")
  TEXT_MSTR("Cobra Mk 3")
  TEXT_MSTR("Constrictor")
  TEXT_MSTR("Fer De Lance")
  TEXT_MSTR("Krait")
  TEXT_MSTR("Mamba")
  TEXT_MSTR("Moray")
  TEXT_MSTR("Python")
  TEXT_MSTR("Sidewinder")
  TEXT_MSTR("Viper")
#ifdef RTC_I2C_ADDRESS
  TEXT_MSTR("CLOCK")
#endif  
  TEXT_MSTR("RANDOM");
  
}
