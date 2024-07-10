#include "GU_Elements.h"

void GU_Button::initButtonUL(int16_t x1, int16_t y1, uint16_t w, uint16_t h,
                            uint16_t outline, uint16_t fill,
                            uint16_t textcolor, char *label,
                            uint8_t textsize_x, uint8_t textsize_y,
                            TapCB callback, int indx, void *param);

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
  strncpy(_label, label, 9);
  _label[9] = 0; // strncpy does not place a null at the end.
                // When 'label' is >9 characters, _label is not terminated.
  _callback = callback;
  _indx = indx;
  _param = param;
  _gd->onTap(EV_TAP, _x1, _y1, _w, _h, _callback, _indx, _param); 
}

void GU_Button::drawButton(bool inverted) 
{
  uint16_t fill, outline, text;
  int16_t x, y;
  uint16_t w, h;

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
  
  // Original code for system font only
  //_gfx->setCursor(_x1 + (_w / 2) - (strlen(_label) * 3 * _textsize_x),
  //                _y1 + (_h / 2) - (4 * _textsize_y));

  _gfx->setTextColor(text);
  _gfx->setTextSize(_textsize_x, _textsize_y);
  _gfx->getTextBounds(_label, _x1, _y1, &x, &y, &w, &h);
#if 0    
  {
    char buf[64];
    sprintf(buf, "x/y %d %d xywh %d %d %d %d", _x1, _y1, x, y, w, h);
    Serial.println(buf);
  }
#endif

  // System font is drawn from the upper left, but custom fonts are
  // drawn from the lower left. Adjust by the Y returned from getTextBounds().
  _gfx->setCursor(_x1 + (_w / 2) - (w / 2),
                  _y1 + (_h / 2) - (h / 2) + (_y1 - y));

  _gfx->print(_label);
}

