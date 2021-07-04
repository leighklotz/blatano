void fillRect(int x, int y, int width, int height, int color) {
  display.setColor(color == 0 ? BLACK : WHITE);
  display.fillRect(x, y, width, height);
  display.display();
}
