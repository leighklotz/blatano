// Leigh Klotz Sat 03 Jul 2021 05:02:04 PM PDT
// Based on code that was 
// Retrieved from http://web.archive.org/web/20080304162908/http://www.davebollinger.com/works/pixelrobots/applet/PixelRobots.pde.txt
// Pixel Robots
// David Bollinger, May 2006
// http://www.davebollinger.com
// for Processing 0115 beta
// and here converted to draw one robot for Arduino

#include <Arduino.h>

#include "PixelRobot.h"

int SQUARE_PIXELS = 480;
int robot_scale = 5;
PixelRobot pixel_robot = PixelRobot();
boolean PRINT_ROBOT = true;
#define PRINT Serial.print
#define PRINTLN Serial.println


// stubs
void noStroke() {Serial.println();}
void fill(int color) {Serial.print(color); }
void rect(int x, int y, int width, int height) { if (x == 0) Serial.println(); }
void background(int color);
///

// test with robot_num = 0x1af824;

void draw(int robot_num) {
  background(0);
  robot.setScales(5, 5);
  robot.setMargins(1, 1);
  robot.generate(robot_num);
  robot.draw(0, 0);
}

PixelRobot::PixelRobot() {
  bgcolor = 0;
  fgcolor = 1;
  xscale = yscale = 2;
  xmargin = ymargin = 1;
}

void PixelRobot::clear() {
  memset(grid, 0, sizeof(*grid));
}

int PixelRobot::getHeight() {
  return rows*yscale+ymargin;
}

int PixelRobot::getWidth() {
  return cols*xscale+xmargin;
}

void PixelRobot::setMargins(int xm, int ym) {
  xmargin = xm;
  ymargin = ym;
}

void PixelRobot::setScales(int xs, int ys) {
  xscale = xs;
  yscale = ys;
}

// generate the robot pattern for the given seed
void PixelRobot::generate(int seed) {
  // HEAD
  int hseed = (seed) & 0xff;
  grid[1][1]  = ((hseed&1)>0)   ? AVOID : EMPTY;
  grid[1][2]  = ((hseed&2)>0)   ? AVOID : EMPTY;
  grid[1][3]  = ((hseed&4)>0)   ? AVOID : EMPTY;
  grid[2][1]  = ((hseed&8)>0)   ? AVOID : EMPTY;
  grid[2][2]  = ((hseed&16)>0)  ? AVOID : EMPTY;
  grid[2][3]  = ((hseed&32)>0)  ? AVOID : EMPTY;
  grid[3][2]  = ((hseed&64)>0)  ? AVOID : EMPTY;
  grid[3][3]  = ((hseed&128)>0) ? AVOID : EMPTY;
  // BODY
  int bseed = (seed>>8) & 0xff;
  grid[4][3]  = ((bseed&1)>0)   ? AVOID : EMPTY;
  grid[5][1]  = ((bseed&2)>0)   ? AVOID : EMPTY;
  grid[5][2]  = ((bseed&4)>0)   ? AVOID : EMPTY;
  grid[5][3]  = ((bseed&8)>0)   ? AVOID : EMPTY;
  grid[6][1]  = ((bseed&16)>0)  ? AVOID : EMPTY;
  grid[6][2]  = ((bseed&32)>0)  ? AVOID : EMPTY;
  grid[6][3]  = ((bseed&64)>0)  ? AVOID : EMPTY;
  grid[7][3]  = ((bseed&128)>0) ? AVOID : EMPTY;
  // FEET
  int fseed = (seed>>16) & 0xff;
  grid[8][3]  = ((fseed&1)>0)   ? AVOID : EMPTY;
  grid[9][1]  = ((fseed&2)>0)   ? AVOID : EMPTY;
  grid[9][2]  = ((fseed&4)>0)   ? AVOID : EMPTY;
  grid[9][3]  = ((fseed&8)>0)   ? AVOID : EMPTY;
  grid[10][0] = ((fseed&16)>0)  ? AVOID : EMPTY;
  grid[10][1] = ((fseed&32)>0)  ? AVOID : EMPTY;
  grid[10][2] = ((fseed&64)>0)  ? AVOID : EMPTY;
  grid[10][3] = ((fseed&128)>0) ? AVOID : EMPTY;
  // edge detector
  // wrap the AVOIDs with SOLIDs where EMPTY
  for (int r=0; r<rows; r++) {
    for (int c=0; c<=cols/2; c++) {
      int here = grid[r][c];
      if (here != EMPTY) continue;
      boolean needsolid = false;
      if ((c>0) && (grid[r][c-1]==AVOID)) needsolid=true;
      if ((c<cols-1) && (grid[r][c+1]==AVOID)) needsolid=true;
      if ((r>0) && (grid[r-1][c]==AVOID)) needsolid=true;
      if ((r<rows-1) && (grid[r+1][c]==AVOID)) needsolid=true;
      if (needsolid)
	grid[r][c] = SOLID;
    }
  }
  // mirror left side into right side, force symmetry
  for (int r=0; r<rows; r++) {
    grid[r][4] = grid[r][2];
    grid[r][5] = grid[r][1];
    grid[r][6] = grid[r][0];
  }
} 
  // draw the robot at given coordinates
void PixelRobot::draw(int basex, int basey) {
  noStroke();
  for (int r=0; r<rows; r++) {
    int y1 = basey + ymargin/2 + r * yscale;
    for (int c=0; c<cols; c++) {
      int x1 = basex + xmargin/2 + c * xscale;
      int m = grid[r][c];
      switch(m) {
      case EMPTY :
      case AVOID : fill(bgcolor); break;
      case SOLID : fill(fgcolor); break;
      }
      if (PRINT_ROBOT) PRINT(m == SOLID ? "X" : " ");
      rect(x1,y1,xscale,yscale);
    }
    if (PRINT_ROBOT) PRINTLN();
  }
  if (PRINT_ROBOT) PRINT_ROBOT=false;
  noStroke();
} 



