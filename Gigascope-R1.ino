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
// While here, accumulate the min and max Y pixel extent (for drag events)
// and the min and max voltage (for display)
void draw_trace(SampleBuffer buf, int start_pos)
{
  int i, p, x, y;
  float v;
  int y_off = chan[start_pos].y_offset;
  int y_ind = chan[start_pos].y_index;

  int prev_x = x_offset + start_pos * tb[x_index].p_sam;
  int prev_y = y_off - (buf[0] - voltage[y_ind].sign_offset) * voltage[y_ind].pix_count;
  chan[start_pos].y_min = 9999;
  chan[start_pos].y_max = 0;
  chan[start_pos].v_min = 99999.0f;
  chan[start_pos].v_max = -99999.0f;
  for (i = 0, p = start_pos + 2; p < buf.size(); i++, p += 2) 
  {
    x = x_offset + i * tb[x_index].p_sam;
    y = y_off - (buf[p] - voltage[y_ind].sign_offset) * voltage[y_ind].pix_count;
    v = (buf[p] - voltage[y_ind].sign_offset) * voltage[y_ind].pix_count / PIX_DIV; 
    tft.drawLine(prev_x, prev_y, x, y, chan[start_pos].color);
    prev_x = x;
    prev_y = y;

    // Accumulate the trace's screen Y extent and voltage
    // (the Y's are inverted wrt the voltage - beware of sign)
    if (y < chan[start_pos].y_min)
    {
      chan[start_pos].y_min = y;
      chan[start_pos].v_max = v;
    }
    if (y > chan[start_pos].y_max)
    {
      chan[start_pos].y_max = y;
      chan[start_pos].v_min = v;
    }
  }
  // Draw the zero point triangle on the far left side. The offset of 12 pixels
  // puts the point of the "play" symbol on the line.
  fc.drawText((char)8, 0, chan[start_pos].y_offset + 12, chan[start_pos].color);
}

// Draw the trigger level indicator on the left.
void draw_trig_level(int ch)
{
  if (trig != 0)
  {
    int y_ind = chan[ch].y_index;
    int adc_count = ADC_RANGE * (trig_level / V_MAX);
    int y_trig = chan[ch].y_offset - (adc_count - voltage[y_ind].sign_offset) * voltage[y_ind].pix_count;
    fc.drawText((char)'T', 0, y_trig + 12, chan[ch].color);
  }
}

// Find the next trigger sample in the buffer, starting at the given
// sample position (assuming interleaved 2-channel data). 
// Start_pos starts at 0 for channel 0, and 1 for channel 2.
// Return 0 if no trigger was found.
int find_next_trigger(SampleBuffer buf, int start_pos)
{
  int adc_count = ADC_RANGE * (trig_level / V_MAX);
  int i, trig_pos;

  trig_pos = 0;
  switch (trig)
  {
  case 0:     // trigger off (still need trigger points for freq display)
  case 1:     // rising
    // Look for the buffer readings crossing adc_count.
    // If signal is high, wait till it goes low then high again.
    i = start_pos;
    while (i < buf.size() && buf[i] > adc_count)
      i += 2;
    if (i >= buf.size())
      return 0;     // no trigger found      

    while (i < buf.size() && buf[i] < adc_count)
      i += 2;
    if (i >= buf.size())
      return 0; 
    break;

  case 2:     // falling
    i = start_pos;
    while (i < buf.size() && buf[i] < adc_count)
      i += 2;
    if (i >= buf.size())
      return 0;      

    while (i < buf.size() && buf[i] > adc_count)
      i += 2;
    if (i >= buf.size())
      return 0; 
    break;
  }
  return i;
}

// Find a set of trigger points and calculate the frequency of the input
// from them. Store the first trigger point with the channel.
// Start_pos starts at 0 for channel 0, and 1 for channel 2.
void find_trig_and_freq(SampleBuffer buf, int start_pos)
{
  int trig_pos;
  int prev_trig = find_next_trigger(buf, start_pos);
  int n_trig = 0;
  long spacing = 0;

  chan[start_pos].trig_pt = 0;
  chan[start_pos].freq = 0;
  if (prev_trig == 0)
    return;
  chan[start_pos].trig_pt = prev_trig;

// TODO this has been seen to loop. INvestigate.
  while ((trig_pos = find_next_trigger(buf, prev_trig)) != 0)
  {
#if 0    
    Serial.print(prev_trig);
    Serial.print(" ");
    Serial.println(trig_pos);
#endif
    // Accumulate the trigger spacing.
    spacing += (trig_pos - prev_trig) / 2;
    n_trig++;
    prev_trig = trig_pos;
  }
  if (n_trig == 0)
    return;
  chan[start_pos].freq = tb[x_index].sps / (spacing / n_trig);    
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
    sprintf(str, "(%dMsps)", tb[indx].sps / 1000000);
  else
    sprintf(str, "(%dksps)", tb[indx].sps / 1000);
}

void ch_vdiv_str(int ch, char *str)
{
  if (voltage[ch].v_div < 1.0f)
    sprintf(str, "%.1fV", voltage[ch].v_div);
  else
    sprintf(str, "%.0fV",voltage[ch].v_div);
}

void ch_freq_str(int ch, char *str)
{
  float freq = chan[ch].freq;

  if (freq < 1)
    str[0] = '\0';  // blank display if no frequency
  else if (freq < 1000.0f)
    sprintf(str, "%.0fHz", freq);
  else if (freq < 10000.0f)
    sprintf(str, "%.2fkHz", freq / 1000);
  else
    sprintf(str, "%.0fkHz", freq / 1000);
}

void ch_volt_str(int ch, char *str)
{
  float v = chan[ch].v_max - chan[ch].v_min;
  if (voltage[chan[ch].y_index].sign_offset == 0)   // unsigned
    sprintf(str, "%.2fVpk", v);
  else
    sprintf(str, "%.2fVp-p", v);
}

// Draw furniture at top of screen. Timebase, 2 channels, and trigger settings.
void draw_furniture()
{
  char str[16];

  b_ch0.drawButton();
  b_ch1.drawButton();
  mb_tb.drawButton();
  mb_ch0.drawButton();
  mb_ch1.drawButton();
  mb_trig.drawButton();
  tb_sps_str(x_index, str);
  fc.drawText(str, 150, 36, WHITE);
}

// Draw the voltage and frequency readouts at the bottom of screen.
// TODO: Some of them may be empty strings.
void draw_freqs_voltages(int ch)
{
  char str[16];

  ch_freq_str(ch, str);
  fc.drawText(str, 30, tft.height() - 20, chan[ch].color);
  ch_volt_str(ch, str);
  fc.drawText(str);
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
    chan[ch].b->setColor(chan[ch].color, BLACK, WHITE); 
  }
  else
  {
    chan[ch].shown = true;
    chan[ch].b->setColor(chan[ch].color, chan[ch].color, BLACK);
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
  mb_tb.initButtonUL(30, 5, 110, 40, WHITE, GREY, WHITE, str, pri++);
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

  // Channel buttons and menus
  int x = 320;
  for (int ch = 0; ch < 2; ch++)
  {
    sprintf(str, "CH%d", ch);
    if (chan[ch].shown)
      chan[ch].b->initButtonUL(x, 5, 90, 40, chan[ch].color, chan[ch].color, BLACK, str, 1, ch_tapCB, pri++, (void *)ch);
    else
      chan[ch].b->initButtonUL(x, 5, 90, 40, chan[ch].color, BLACK, WHITE, str, 1, ch_tapCB, pri++, (void *)ch);

    ch_vdiv_str(chan[ch].y_index, str);
    chan[ch].mb->initButtonUL(x + 100, 5, 90, 40, WHITE, GREY, WHITE, str, 1);
    chan[ch].m->initMenu(chan[ch].mb, WHITE, DKGREY, GREY, WHITE, ch_menuCB, pri++, (void *)ch);
    for (i = 0; i < VOLTS_MAX; i++)
    {
      ch_vdiv_str(i, str);
      chan[ch].m->setMenuItem(i, str);
    }
    chan[ch].m->checkMenuItem(chan[ch].y_index, true);
    x = 550;
  }

  // Trigger settings
  mb_trig.initButtonUL(650, tft.height() - 50, 90, 40, WHITE, GREY, WHITE, "Off", 1);
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
    // Poll for gesture input
    detector.poll();

    // Acquire and display a trace
    if (adc.available()) 
    {
      SampleBuffer buf = adc.read();
#if 0
      Serial.print("Channels: ");
      Serial.print(buf.channels());
      Serial.print(" Interleaved: ");
      Serial.print(buf.getflags(DMA_BUFFER_INTRLVD));
      Serial.print(" Size: ");
      Serial.println(buf.size());
#endif
      
      // Process the buffer. Interleaved samples.
      // Make sure this doesn't get displayed when a menu is active
      if ((millis() - last_millis) > 100  && !m_tb.isAnyMenuDisplayed()) 
      {
        int i;

        // Draw the graticule.
        tft.startBuffering();
        tft.fillScreen(0);
        for (i = 0; i < tft.height(); i += PIX_DIV)
          tft.drawLine(0, i, tft.width(), i, GREY);
        for (i = 0; i < tft.width(); i += PIX_DIV)
          tft.drawLine(i, 0, i, tft.height(), GREY);

#if 0
        // Debugging printout
        int samples = buf.size() / buf.channels();
        int p;
        for (i = 0, p = 0; i < samples; i++)
        {
          Serial.print(buf[p]);
          p++;
          Serial.print(" ");
          Serial.println(buf[p]);
          p++;
        }
#endif
        // Find the frequency and trigger point in channel 0 and set set x_offset accordingly.
        find_trig_and_freq(buf, 0);
        if (trig != 0)
          x_offset = -(chan[0].trig_pt / 2) * tb[x_index].p_sam;
        else
          x_offset = 0;

        // Draw the buttons and other stuff on the screen
        draw_furniture();

        // Draw the traces for each channel. Adapt the drag/pinch event(s) to
        // the envelopes of the trace(s).
        for (int ch = 0; ch < 2; ch++)
        {
          if (chan[ch].shown)
          {
            draw_trace(buf, ch);

            // If the envelope is very thin (e.g. a flat trace) make it a bit wider so it is draggable.
            int trace_y = chan[ch].y_min;
            int trace_height = chan[ch].y_max - chan[ch].y_min;
            if (trace_height < 50)
            {
              trace_y -= 25;
              trace_height = 50;
            }
            detector.onDrag(0, trace_y, tft.width(), trace_height, 
                            ch_dragCB, trace_pri + ch, (void *)ch, CO_VERT, 2);
          }
          else
          {
            // Channel 0 not shown, make sure that any drag event is turned off.
            detector.cancelEvent(trace_pri + ch);
          }
        }

        // Draw the trigger level indicator on the left.
        draw_trig_level(0);

        // Draw freq and voltage display.
        draw_freqs_voltages(0);

        last_millis = millis();
        tft.endBuffering();
      }
      // Release the buffer to return it to the pool.
      buf.release();
    }
}
