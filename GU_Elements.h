// UI elements for the Giga display.

#include <Arduino_GigaDisplay_GFX.h>
#include <GestureDetector.h>

// Provide a class to draw an Adafruit_GFX_Button with a custom font,
// since the Adafruit one only works correctly with system font. 
// The rest of it is shamelessly copied from Adafruit_GFX_Button.

class GU_Button
{
public:
  GU_Button(void) { _gfx = 0; }
  ~GU_Button() {  }

  // Set up the placement and appearance of a button.
  void initButtonUL(Adafruit_GFX *gfx, int16_t x1,
                                       int16_t y1, uint16_t w, uint16_t h,
                                       uint16_t outline, uint16_t fill,
                                       uint16_t textcolor, char *label,
                                       uint8_t textsize_x, uint8_t textsize_y);
  
  // Draw the button.
  void drawButton(bool inverted = false); 

private:
  Adafruit_GFX *_gfx;
  int16_t _x1, _y1; // Coordinates of top-left corner
  uint16_t _w, _h;
  uint8_t _textsize_x;
  uint8_t _textsize_y;
  uint16_t _outlinecolor, _fillcolor, _textcolor;
  char _label[10];
};

// The Menu class:
// - associates a drop-down menu with a button
// - allows multiple menu items to be added
// - allows menu items to be checked or disabled
// - calls a callback when a menu item is selected
class GU_Menu
{
  GU_Menu(void) { _gfx = 0; }
  ~GU_Menu() {  }

  // Set up a menu with placement, item sizes, and associated button.
  // Provide a callback giving the item selected when tapping or dragging
  // down to the item.
  void initMenu();

  // Set up a menu item at the given index within the menu.
  void setMenuItem(int indx, char *itemText, bool enabled = true, bool checked = false);

  // Disable/enable a menu item.
  void enableMenuItem(int indx, bool enabled);

  // Set the checkbox in a menu item.
  void checkMenuItem(int indx, bool checked);

private:
  typedef struct GU_MenuItem
  {
    char label[20];
    bool checked;
    bool enabled;
  } GU_MenuItem;

  Adafruit_GFX *_gfx;
  int16_t _x1, _y1;   // Coordinates of top-left corner of menu area
  uint16_t _w, _h;
  uint8_t _textsize_x;  // Can these come from the button?
  uint8_t _textsize_y;
  uint16_t _outlinecolor, _fillcolor, _highlight_color, _textcolor;
  GU_Button *_button;    // the associated button
  GU_MenuItem _items[20];
};