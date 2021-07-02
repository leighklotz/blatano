// Retrieved from http://web.archive.org/web/20080304162908/http://www.davebollinger.com/works/pixelrobots/applet/PixelRobots.pde.txt
// Pixel Robots
// David Bollinger, May 2006
// http://www.davebollinger.com
// for Processing 0115 beta
//

PixelRobotFitter fitter = new PixelRobotFitter(480/8, 480/12);
int currentSeed = 0;

void setup() {
  size(480,480);
  framerate(30);
  next();
}

void next() {
  background(color(255,255,255));
  fitter.make(++currentSeed);
}

void draw() {
  fitter.drawone();
  if (fitter.at00())
    next();
}


class PixelRobot {
  // these are black magic voodoo values
  static final int cols = 7;
  static final int rows = 11;
  // possible values for the grid array
  static final int EMPTY = 0; // aka "WHITE SPACE"
  static final int AVOID = 1; // aka "GUTS" or "INSIDES"
  static final int SOLID = 2; // aka "SKIN" or "OUTLINE"
  // the grid; only 4 columns of storage are really needed due to
  // symmetry, but the full 7 columns are allocated and processed,
  // which makes it easier to play with assymetrical designs etc.
  int[][] grid;
  // colors for filling EMPTY/AVOID and SOLID areas, respectively
  color bgcolor, fgcolor;
  // scaling values (in pixel units)
  int xscale, yscale;
  // margins (in pixel units)
  int xmargin, ymargin;
  PixelRobot() {
    grid = new int[rows][cols];
    bgcolor = color(255,255,255);
    fgcolor = color(0,0,0);
    xscale = yscale = 2;
    xmargin = ymargin = 1;
  }
  //
  int getHeight() {
    return rows*yscale+ymargin;
  }
  //
  int getWidth() {
    return cols*xscale+xmargin;
  }
  //
  void setMargins(int xm, int ym) {
    xmargin = xm;
    ymargin = ym;
  }
  //
  void setScales(int xs, int ys) {
    xscale = xs;
    yscale = ys;
  }
  // reset the entire grid to empty
  void wipe() {
    for (int r=0; r<rows; r++)
      for (int c=0; c<cols; c++)
        grid[r][c] = EMPTY;
  }
  // generate the robot pattern for the given seed
  void generate(int seed) {
    wipe();
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
  void draw(int basex, int basey) {
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
        rect(x1,y1,xscale,yscale);      
      }
    }
  } 
}

class PixelRobotFitter {
  PixelRobot robot;
  int cols, rows;
  int col, row;
  int seed;
  // cells store the box fit pattern, contents are byte encoded as:
  // (cell >> 16) & 0xff == work to do
  // (cell) & 0xff == size of allocated area
  // (this somewhat odd encoding scheme is a remnant of the
  //  fractal subdivision method used for the t-shirt image)
  int [][] cells;
  // the types of work that may occur in a cell
  static final int WORK_NONE = 0;
  static final int WORK_DONE = 1;
  PixelRobotFitter(int c, int r) {
    cols = c;
    rows = r;
    robot = new PixelRobot();
    cells = new int[rows][cols];
  }
  // reset the pattern grid
  void wipe() {
    for (int r=0; r<rows; r++)
      for (int c=0; c<cols; c++)
        cells[r][c] = 0; 
    col = row = 0;
  }
  // determines if cells already contain defined area(s)
  boolean isOccupied(int col, int row, int wid, int hei) {
    for (int r=row; r<row+hei; r++) {
      for (int c=col; c<col+wid; c++) {
        if (cells[r][c] != 0) {
          return true;
        }
      }
    }
    return false;
  }
  // marks cells as containing an area
  void doOccupy(int col, int row, int wid, int hei, int val) {
    for (int r=row; r<row+hei; r++) {
      for (int c=col; c<col+wid; c++) {
        cells[r][c] = val;
      }
    }
  }  
  // define the pattern grid
  void make(int s) {
    seed = s;
    randomSeed(seed);
    wipe();
    for (int r=0; r<rows; r++) {
      for (int c=0; c<cols; c++) {
        int cell = cells[r][c];
        if (cell != 0) continue;
        // figure out the size of area to occupy
        int sizer, limit;
        do {
          limit = min(cols-c, rows-r);
          limit = min(limit, 8);
          sizer = int(random(limit))+1;
        } while(isOccupied(c,r,sizer,sizer));
        // flag all cells as occupied by width x height area
        doOccupy(c,r,sizer,sizer,sizer);
        // indicate work to perform in upper-left cell
        int work = WORK_DONE;
        cells[r][c] |= (work<<16);
      } // for c
    } // for r
  }
  // is cursor at 0,0
  boolean at00() {
    return ((col==0) && (row==0));
  }
  // step the cursor forward
  void advance(int advcol) {
    col += advcol;
    if (col >= cols) {
      col = 0;
      row++;
      if (row >= rows) {
        row = 0;
      }
    }
  }
  // advance through the pattern and draw the next robot
  void drawone() {
    boolean drawn = false;
    do {
      int cell = cells[row][col];
      int work = (cell>>16) & 0xff;
      int sizer = (cell) & 0xff;
      if (work != WORK_NONE) {
        int y1 = 12*row;
        int x1 = 8*col;
        robot.setScales( sizer, sizer );
        robot.setMargins( sizer, sizer );
        robot.generate( int(random(0x1000000)) );
        robot.draw( x1, y1 );
        drawn = true;
      }
      advance(sizer);
      if (at00()) return;
    } while (!drawn);
  }
}
