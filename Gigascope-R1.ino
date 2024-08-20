#include <Arduino_AdvancedAnalog.h>
#include <GU_Elements.h>
#include "gscope.h"
#include <fonts/FreeSans18pt7b.h>
#include <fonts/UISymbolSans18pt7b.h>

// Oscilloscope for the Giga R1 and Giga Display.
// Dual channel with two independent 10bit, 1Msps ADC's.

// Uses libraries:
// GestureDetector for screen interaction
// GU_Elements for UI elements 
// FontCollection for text and symbol display
// Arduino_GigaDisplay_GFX for screen display
// Arduino_AdvancedAnalog for trace acquisition
// (and all their dependencies)

AdvancedADC adc(A0, A1);
uint64_t last_millis = 0;

GestureDetector detector;
GigaDisplay_GFX tft;
FontCollection fc(&tft, &FreeSans18pt7b, &UISymbolSans18pt7b, 1, 1);

GU_Button b_ch0(&fc, &detector);      // CH0 enable/disable button
GU_Button b_ch1(&fc, &detector);      // CH1 enable/disable button

GU_Menu m_tb(&fc, &detector);         // Timebase menu
GU_Menu m_ch0(&fc, &detector);        // CH0 voltage menu
GU_Menu m_ch1(&fc, &detector);        // CH1 voltage menu
GU_Menu m_trig(&fc, &detector);       // Trigger options menu

GU_Button mb_tb(&fc, &detector);      // Buttons for the menus
GU_Button mb_ch0(&fc, &detector);
GU_Button mb_ch1(&fc, &detector);
GU_Button mb_trig(&fc, &detector);

// Globals used during pinch and drag of traces
bool pinching = false;
bool dragging = false;
float orig_tdiv;
int orig_yoffset;

// Priority of events set up dynamically after the fixed UI is defined
int trace_pri;

// Draw a trace from a 2-channel interleaved sample buffer, starting
// at start_pos (0 or 1) which is also the channel number.
void draw_trace(SampleBuffer buf, int start_pos)
{
  int i, p, x, y;
  int y_off = chan[start_pos].y_offset;
  int y_ind = chan[start_pos].y_index;

  int prev_x = x_offset + start_pos * tb[x_index].p_sam;
  int prev_y = y_off - (buf[0] - voltage[y_ind].sign_offset) * voltage[y_ind].pix_count;
  chan[start_pos].y_min = 9999;
  chan[start_pos].y_max = 0;
  for (i = 0, p = start_pos + 2; p < buf.size(); i++, p += 2) 
  {
    x = x_offset + i * tb[x_index].p_sam;
    y = y_off - (buf[p] - voltage[y_ind].sign_offset) * voltage[y_ind].pix_count;
    tft.drawLine(prev_x, prev_y, x, y, WHITE);
    prev_x = x;
    prev_y = y;

    // Accumulate the trace's screen Y extent
    if (y < chan[start_pos].y_min)
      chan[start_pos].y_min = y;
    if (y > chan[start_pos].y_max)
      chan[start_pos].y_max = y;
  }
  // Draw the zero point triangle on the far left side. The offset of 12 pixels
  // puts the point of the "play" symbol on the line.
  fc.drawText((char)8, 0, chan[start_pos].y_offset + 12, WHITE);
}

// Draw the trigger level indicator on the left.
void draw_trig()
{
  if (trig != 0)
  {
    int y_ind = chan[0].y_index;
    int adc_count = ADC_RANGE * (trig_level / V_MAX);
    int y_trig = chan[0].y_offset - (adc_count - voltage[y_ind].sign_offset) * voltage[y_ind].pix_count;
    fc.drawText((char)'T', 0, y_trig + 12, WHITE);
  }
}

// Find the trigger point in channel 0.
void find_trigger(SampleBuffer buf)
{
  int adc_count = ADC_RANGE * (trig_level / V_MAX);
  int i;

  x_offset = 0;
  switch (trig)
  {
  case 1:     // rising
    // Look for the buffer readings crossing adc_count.
    // If signal is high, wait till it goes low then high again.
    i = 0;
    while (buf[i] > adc_count && i < buf.size())
      i += 2;
    if (i >= buf.size())
      return;     // no trigger found, leave x_offset alone      

    while (buf[i] < adc_count && i < buf.size())
      i += 2;
    if (i >= buf.size())
      return; 

    x_offset = -(i / 2) * tb[x_index].p_sam;
    break;

  case 2:     // falling
    i = 0;
    while (buf[i] < adc_count && i < buf.size())
      i += 2;
    if (i >= buf.size())
      return;     // no trigger found, leave x_offset alone      

    while (buf[i] > adc_count && i < buf.size())
      i += 2;
    if (i >= buf.size())
      return; 

    x_offset = -(i / 2) * tb[x_index].p_sam;
    break;
  }
}

// Get short strings for each field
void tb_tdiv_str(int indx, char *str)
{
  if (tb[indx].t_div >= 1000)
    sprintf(str, "%.0fms", tb[indx].t_div / 1000);
  else
    sprintf(str, "%.0fus", tb[indx].t_div);
}


void tb_sps_str(int indx, char *str)
{
  if (tb[indx].sps >= 1000000)
    sprintf(str, " %dMsps", tb[indx].sps / 1000000);
  else
    sprintf(str, " %dksps", tb[indx].sps / 1000);
}

void ch_vdiv_str(int indx, char *str)
{
  if (voltage[indx].v_div < 1.0f)
    sprintf(str, "%.1fV", voltage[indx].v_div);
  else
    sprintf(str, "%.0fV",voltage[indx].v_div);
}

// Draw furniture at top of screen. Timebase, 2 channels, and trigger settings.
void draw_tb()
{
  char str[9];

  b_ch0.drawButton();
  b_ch1.drawButton();
  mb_tb.drawButton();
  mb_ch0.drawButton();
  mb_ch1.drawButton();
  mb_trig.drawButton();
  tb_sps_str(x_index, str);
  fc.drawText(str, 120, 40, WHITE);
}

// Toggle a channel on and off when tapped. The channel number
// is in the param.
void ch_tapCB(EventType ev, int indx, void *param, int x, int y)
{
  int ch = (int)param;

  if ((ev & EV_RELEASED) == 0)
    return;   // we only act on the releases

  if (chan[ch].shown)
  {
    chan[ch].shown = false;
    chan[ch].b->setColor(YELLOW, BLACK, WHITE); 
  }
  else
  {
    chan[ch].shown = true;
    chan[ch].b->setColor(YELLOW, YELLOW, BLACK);
  }
}

// When x_index is updated, check mark the right menu item in the TB menu,
// and set the button text to reflect the new setting
void check_tb_menu(int indx)
{
  char str[9];

  for (int i = 0; i < TB_MAX; i++)
    m_tb.checkMenuItem(i, false);
  m_tb.checkMenuItem(indx, true);
  tb_tdiv_str(indx, str);
  mb_tb.setText(str);
}

// When channel y_index is updated, check mark the right menu item in its menu,
// and set the button text to reflect the new setting. 
void check_ch_menu(int ch, int indx)
{
  char str[9];

  for (int i = 0; i < VOLTS_MAX; i++)
    chan[ch].m->checkMenuItem(i, false);
  chan[ch].m->checkMenuItem(indx, true);
  ch_vdiv_str(chan[ch].y_index, str);
  chan[ch].mb->setText(str);
}

// Callbacks are called whenever a menu item is selected.
// The timebase menu
void tb_menuCB(EventType ev, int indx, void *param, int x, int y)
{
  if ((indx & 0xFF) == 0xFF)
    return;
  x_index = indx & 0xFF;   // menu item # in the low byte

  // Take down the ADC and start it up again with the new clock freq
  adc.stop();
  adc.begin(ADC_RESOLUTION, tb[x_index].sps, 1024, 32);

  // Set the text in the button and check mark the right menu item
  check_tb_menu(x_index);
}

// The voltage menu. The channel number is in the param.
void ch_menuCB(EventType ev, int indx, void *param, int x, int y)
{
  int ch = (int)param;

  if ((indx & 0xFF) == 0xFF)
    return;
  chan[ch].y_index = indx & 0xFF;
  check_ch_menu(ch, chan[ch].y_index);
}

// Handle horizontal pinches and adjust timebase.
void tb_pinchCB(EventType ev, int indx, void *param, int dx, int dy, float sx, float sy)
{
  float tdiv;
  float min_diff = 1000000.0f;
  int min_indx;

  if (ev & EV_RELEASED)   // The pinch has been lifted off.
  {
    pinching = false;
    return;
  }
  
  if (!pinching)
  {
    orig_tdiv = tb[x_index].t_div;
    pinching = true;
  }

  // Find the nearest t/div. Use sx squared to increase sensitivity
  tdiv = orig_tdiv / (sx * sx);
  for (int i = 0; i < TB_MAX; i++)
  {
    float dt = fabsf(tdiv - tb[i].t_div);
    if (dt < min_diff)
    {
      min_diff = dt;
      min_indx = i;
    }
  }
  x_index = min_indx;
  adc.stop();
  adc.begin(ADC_RESOLUTION, tb[x_index].sps, 1024, 32);
  check_tb_menu(x_index);
}

void ch_dragCB(EventType ev, int indx, void *param, int x, int y, int dx, int dy)
{
  int ch = (int)param;    // Channel number we are dragging

  if (ev & EV_RELEASED)   // The drag has been lifted off.
  {
    dragging = false;
    return;
  }

  if (!dragging)
  {
    orig_yoffset = chan[ch].y_offset;
    dragging = true;
  }

  chan[ch].y_offset = orig_yoffset + dy;
}

// Trigger menu selection.
void trigCB(EventType ev, int indx, void *param, int x, int y)
{
  if ((indx & 0xFF) == 0xFF)
    return;
  trig = indx & 0xFF;   // menu item # in the low byte
  switch(trig)
  {
  case 0:
    mb_trig.setText("Off");
    trig_level = rising_level;
    break;
  case 1:
    mb_trig.setText((char)2);
    trig_level = rising_level;
    break;
  case 2:
    mb_trig.setText((char)4);
    trig_level = falling_level;
    break;
  }
  m_trig.checkMenuItem(0, false);
  m_trig.checkMenuItem(1, false);
  m_trig.checkMenuItem(2, false);
  m_trig.checkMenuItem(trig, true);
}

void setup() 
{
  int i;
  char str[9];

  Serial.begin(9600);
  while(!Serial) {}  // TODO - remove this when going standalone

  // Resolution, sample rate, number of samples per channel, queue depth.
  // 1 million SPS seems to be the limit (tested at 8 bit and 10 bit)
  // Changing sample time to 2_5 makes no difference.

  if (!adc.begin(ADC_RESOLUTION, tb[x_index].sps, 1024, 32)) 
  {
    Serial.println("Failed to start analog acquisition!");
    while (1);
  }

  tft.begin();
  if (detector.begin()) {
    Serial.println("Touch controller init - OK");
  } else {
    Serial.println("Touch controller init - FAILED");
    while(1) ;
  }

  // Set landscape mode. these must occur togerther.
  tft.setRotation(1);
  detector.setRotation(1);
  
  // Set up the buttons and menus across the top of the screen.
  // Timebase
  int pri = 1;
  tb_tdiv_str(x_index, str);
  mb_tb.initButtonUL(30, 5, 90, 40, WHITE, GREY, WHITE, str, pri++);
  m_tb.initMenu(&mb_tb, WHITE, DKGREY, GREY, WHITE, tb_menuCB, pri++, NULL);
  for (i = 0; i < TB_MAX; i++)
  {
    tb_tdiv_str(i,str);
    m_tb.setMenuItem(i, str);
  }
  m_tb.checkMenuItem(x_index, true);

  // Set up the channel structs
  chan[0].b = &b_ch0;
  chan[0].mb = &mb_ch0;
  chan[0].m = &m_ch0;
  chan[1].b = &b_ch1;
  chan[1].mb = &mb_ch1;
  chan[1].m = &m_ch1;

  // Channel 0
  if (chan[0].shown)
    b_ch0.initButtonUL(250, 5, 90, 40, YELLOW, YELLOW, BLACK, "CH0", 1, ch_tapCB, pri++, (void *)0);
  else
    b_ch0.initButtonUL(250, 5, 90, 40, YELLOW, BLACK, WHITE, "CH0", 1, ch_tapCB, pri++, (void *)0);

  ch_vdiv_str(chan[0].y_index, str);
  mb_ch0.initButtonUL(350, 5, 90, 40, WHITE, GREY, WHITE, str, 1);
  m_ch0.initMenu(&mb_ch0, WHITE, DKGREY, GREY, WHITE, ch_menuCB, pri++, (void *)0);
  for (i = 0; i < VOLTS_MAX; i++)
  {
    ch_vdiv_str(i, str);
    m_ch0.setMenuItem(i, str);
  }
  m_ch0.checkMenuItem(chan[0].y_index, true);

  // Channel 1
  if (chan[1].shown)
    b_ch1.initButtonUL(450, 5, 90, 40, YELLOW, YELLOW, BLACK, "CH1", 1, ch_tapCB, pri++, (void *)1);
  else
    b_ch1.initButtonUL(450, 5, 90, 40, YELLOW, BLACK, WHITE, "CH1", 1, ch_tapCB, pri++, (void *)1);

  ch_vdiv_str(chan[1].y_index, str);
  mb_ch1.initButtonUL(550, 5, 90, 40, WHITE, GREY, WHITE, str, 1);
  m_ch1.initMenu(&mb_ch1, WHITE, DKGREY, GREY, WHITE, ch_menuCB, pri++, (void *)1);
  for (i = 0; i < VOLTS_MAX; i++)
  {
    ch_vdiv_str(i, str);
    m_ch1.setMenuItem(i, str);
  }
  m_ch1.checkMenuItem(chan[1].y_index, true);

  // Trigger settings
  mb_trig.initButtonUL(670, 5, 90, 40, WHITE, GREY, WHITE, "Off", 1);
  m_trig.initMenu(&mb_trig, WHITE, DKGREY, GREY, WHITE, trigCB, pri++, NULL);
  m_trig.setMenuItem(0, "Off");
  m_trig.setMenuItem(1, (char)2);   // rising edge
  m_trig.setMenuItem(2, (char)4);   // falling edge
  m_trig.checkMenuItem(0, true);    // default is OFF

  // Set up horizontal pinch for timebase zooming. Vertical drags and pinches
  // will wait till some trace(s) are drawn. Save the priority of the
  // next event to be used then.
  detector.onPinch(0, 0, 0, 0, tb_pinchCB, pri++, NULL, false, CO_HORIZ, 2);
  trace_pri = pri;

  // 1kHz square wave output for testing
  pinMode(2, OUTPUT);
  tone(2, 1000);
}

void loop() 
{
    int i, p, s, samples;

    // Poll for gesture input
    detector.poll();

    // Acquire and display a trace
    if (adc.available()) 
    {
      int prev_x, prev_y, x, y;
      int bufsize;
      SampleBuffer buf = adc.read();
#if 0
      Serial.print("Channels: ");
      Serial.print(buf.channels());
      Serial.print(" Interleaved: ");
      Serial.print(buf.getflags(DMA_BUFFER_INTRLVD));
      Serial.print(" Size: ");
      Serial.println(buf.size());
#endif
      bufsize = buf.size();
      samples = bufsize / buf.channels();
      
      // Process the buffer. Interleaved samples.
      // Make sure this doesn't get displayed when a menu is active
      if ((millis() - last_millis) > 100  && !m_tb.isAnyMenuDisplayed()) 
      {
        // Draw the graticule.
        tft.startBuffering();
        tft.fillScreen(0);
        for (i = 0; i < tft.height(); i += PIX_DIV)
          tft.drawLine(0, i, tft.width(), i, GREY);
        for (i = 0; i < tft.width(); i += PIX_DIV)
          tft.drawLine(i, 0, i, tft.height(), GREY);

        // Debugging printout
#if 0
        for (i = 0, p = 0; i < samples; i++)
        {
          Serial.print(buf[p]);
          p++;
          Serial.print(" ");
          Serial.println(buf[p]);
          p++;
        }
#endif
        // Draw the furniture at the top. 
        // Custom fonts are drawn from the bottom left corner
        draw_tb();

        // Find the trigger point set set x_offset accordingly.
        find_trigger(buf);

        // Draw the traces for each channel. Adapt the drag/pinch event(s) to
        // the envelopes of the trace(s).
        for (int ch = 0; ch < 2; ch++)
        {
          if (chan[ch].shown)
          {
            draw_trace(buf, ch);
            detector.onDrag(0, chan[ch].y_min, tft.width(), chan[ch].y_max - chan[ch].y_min, 
                            ch_dragCB, trace_pri + ch, (void *)ch, CO_VERT, 2);
          }
          else
          {
            // Channel 0 not shown, make sure that any drag event is turned off.
            detector.cancelEvent(trace_pri + ch);
          }
        }

        // Draw the trigger level indicator on the left.
        draw_trig();

        last_millis = millis();
        tft.endBuffering();
      }
      // Release the buffer to return it to the pool.
      buf.release();
    }
}
