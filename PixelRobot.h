// Leigh Klotz Sat 03 Jul 2021 05:02:04 PM PDT
// Based on code that was 
// Retrieved from http://web.archive.org/web/20080304162908/http://www.davebollinger.com/works/pixelrobots/applet/PixelRobots.pde.txt
// Pixel Robots
// David Bollinger, May 2006
// http://www.davebollinger.com
// for Processing 0115 beta
// and here converted to draw one robot for Arduino

class PixelRobot {
public:
  PixelRobot();
  // Clear, if you plan to re-use
  void clear();

  // Return height including margin
  int getHeight();
  // Return width including margin
  int getWidth();
  // Set horizontal and vertical margin pixels for each robot
  void setMargins(int xm, int ym);
  // set X and Y size of pixels
  void setScales(int xs, int ys);

  // generate the robot pattern for the given seed
  void generate(int seed); 

  // draw the robot at given coordinates
  void draw(int basex, int basey); 
private:
  // these are black magic voodoo values
  static const int cols = 7;
  static const int rows = 11;
  // possible values for the grid array
  static const uint8_t EMPTY = 0; // aka "WHITE SPACE"
  static const uint8_t AVOID = 1; // aka "GUTS" or "INSIDES"
  static const uint8_t SOLID = 2; // aka "SKIN" or "OUTLINE"
  // the grid; only 4 columns of storage are really needed due to
  // symmetry, but the full 7 columns are allocated and processed,
  // which makes it easier to play with assymetrical designs etc.
  uint8_t grid[rows][cols];
  // colors for filling EMPTY/AVOID and SOLID areas, respectively
  int bgcolor, fgcolor;
  // scaling values (in pixel units)
  int xscale, yscale;
  // margins (in pixel units)
  int xmargin, ymargin;
};

extern PixelRobot pixel_robot;

