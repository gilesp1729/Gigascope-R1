#ifndef GU_ELEMENTS_H
#define GU_ELEMENTS_H

// UI elements for the Giga display.

#include <Arduino_GigaDisplay_GFX.h>
#include <GestureDetector.h>

// Provide a class to draw an Adafruit_GFX_Button with a custom font,
// since the Adafruit one only works correctly with system font. 
// The rest of it is shamelessly copied from Adafruit_GFX_Button.

// Callback function for batton or menu selections.
typedef void (*GU_Callback)(int param);

class GU_Button
{
public:
  GU_Button(Adafruit_GFX *gfx, GestureDetector *gd) { _gfx = gfx; _gd = gd; }
  ~GU_Button() {  }

  // Set up the placement and appearance of a button.
  
  // TODO arg list descriptions here and for menu

  void initButtonUL(int16_t x1, int16_t y1, uint16_t w, uint16_t h,
                    uint16_t outline, uint16_t fill,
                    uint16_t textcolor, char *label,
                    uint8_t textsize_x, uint8_t textsize_y,
                    TapCB callback, int indx, void *param);
  
  // Draw the button.
  void drawButton(bool inverted = false); 

private:
  Adafruit_GFX *_gfx;
  GestureDetector *_gd;
  int16_t _x1, _y1; // Coordinates of top-left corner
  uint16_t _w, _h;
  uint8_t _textsize_x;
  uint8_t _textsize_y;
  uint16_t _outlinecolor, _fillcolor, _textcolor;
  char _label[10];
  TapCB _callback;
  int _indx;
  void *_param;
};

// The Menu class:
// - associates a drop-down menu with a button
// - allows multiple menu items to be added
// - allows menu items to be checked or disabled
// - calls a callback when a menu item is selected
class GU_Menu
{
  GU_Menu(Adafruit_GFX *gfx, GestureDetector *gd) { _gfx = gfx; _gd = gd; }
  ~GU_Menu() {  }

  // Set up a menu with placement and item sizes from the associated button.
  // Provide a callback giving the item selected when tapping or dragging
  // down to the item.
  void initMenu(GU_Button *button, 
                uint16_t outline, uint16_t fill,
                uint16_t highlight_color, uint16_t textcolor,
                TapCB callback, int indx, void *param);
 

  // Set up a menu item at the given index within the menu.
  void setMenuItem(int indx, char *itemText, bool enabled = true, bool checked = false);

  // Disable/enable a menu item.
  void enableMenuItem(int indx, bool enabled);

  // Set the checkbox in a menu item.
  void checkMenuItem(int indx, bool checked);

private:
  typedef struct GU_MenuItem
  {
    char label[20];       // String to display on menu item
    int item_width;       // Width from getTextBounds
    bool checked;         // Whether checked or enabled/disables
    bool enabled;
  } GU_MenuItem;

  Adafruit_GFX *_gfx;
  GestureDetector *_gd;
  int16_t _x1, _y1;   // Coordinates of top-left corner of menu area
  uint16_t _w, _h;    // Width/height come from items extents
  uint8_t _textsize_x;  // Text sizes come from the button text sizes
  uint8_t _textsize_y;
  uint16_t _outlinecolor, _fillcolor, _highlight_color, _textcolor;
  GU_Button *_button;    // the associated button
  GU_MenuItem _items[20];
  TapCB _callback;
  int _indx;
  void *_param;
};

#endif // def GU_ELEMENTS_H
