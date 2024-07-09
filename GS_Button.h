// Provide a class to draw an Adafruit_GFX_Button with a custom font,
// since the Adafruit one only works correctly with system font. 
// The rest of it is shamelessly copied from Adafruit_GFX_Button.

class GS_Button
{
public:
  GS_Button(void) { _gfx = 0; }
  ~GS_Button() {  }

  void initButtonUL(Adafruit_GFX *gfx, int16_t x1,
                                       int16_t y1, uint16_t w, uint16_t h,
                                       uint16_t outline, uint16_t fill,
                                       uint16_t textcolor, char *label,
                                       uint8_t textsize_x, uint8_t textsize_y) 
  {
    _x1 = x1;
    _y1 = y1;
    _w = w;
    _h = h;
    _outlinecolor = outline;
    _fillcolor = fill;
    _textcolor = textcolor;
    _textsize_x = textsize_x;
    _textsize_y = textsize_y;
    _gfx = gfx;
    strncpy(_label, label, 9);
    _label[9] = 0; // strncpy does not place a null at the end.
                  // When 'label' is >9 characters, _label is not terminated.
  }

  void drawButtonCustom(bool inverted = false) 
  {
    uint16_t fill, outline, text;

    if (!inverted) 
    {
      fill = _fillcolor;
      outline = _outlinecolor;
      text = _textcolor;
    } 
    else 
    {
      fill = _textcolor;
      outline = _outlinecolor;
      text = _fillcolor;
    }

    uint8_t r = min(_w, _h) / 4; // Corner radius
    
    _gfx->fillRoundRect(_x1, _y1, _w, _h, r, fill);
    _gfx->drawRoundRect(_x1, _y1, _w, _h, r, outline);

    _gfx->setCursor(_x1 + (_w / 2) - (strlen(_label) * 3 * _textsize_x),
                    _y1 + (_h / 2) - (4 * _textsize_y));
    _gfx->setTextColor(text);
    _gfx->setTextSize(_textsize_x, _textsize_y);
    _gfx->print(_label);
  }

private:
  Adafruit_GFX *_gfx;
  int16_t _x1, _y1; // Coordinates of top-left corner
  uint16_t _w, _h;
  uint8_t _textsize_x;
  uint8_t _textsize_y;
  uint16_t _outlinecolor, _fillcolor, _textcolor;
  char _label[10];
};