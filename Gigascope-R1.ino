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

// Draw a trace from a 2-channel interleaved sample buffer, starting
// at start_pos (0 or 1)
// TODO: don't draw off the screen, do triggering, etc. etc.
void draw_trace(SampleBuffer buf, int start_pos)
{
  int i, p, x, y;
  int y_off = y_offset[start_pos];
  int y_ind = y_index[start_pos];

  int prev_x = x_offset + start_pos * tb[x_index].p_sam;
  int prev_y = y_off - (buf[0] - voltage[y_ind].sign_offset) * voltage[y_ind].pix_count;

  for (i = 0, p = start_pos + 2; p < buf.size(); i++, p += 2) 
  {
    x = x_offset + i * tb[x_index].p_sam;
    y = y_off - (buf[p] - voltage[y_ind].sign_offset) * voltage[y_ind].pix_count;
    tft.drawLine(prev_x, prev_y, x, y, WHITE);
    prev_x = x;
    prev_y = y;
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
void tapCB(EventType ev, int indx, void *param, int x, int y)
{
  int ch = (int)param;

  if ((ev & EV_RELEASED) == 0)
    return;   // we only act on the releases

  switch (ch)
  {
  case 0:
    if (show_ch0)
    {
      show_ch0 = false;
      b_ch0.setColor(YELLOW, BLACK, WHITE); 
    }
    else
    {
      show_ch0 = true;
      b_ch0.setColor(YELLOW, YELLOW, BLACK);
    }
    break;

  case 1:
    if (show_ch1)
    {
      show_ch1 = false;
      b_ch1.setColor(YELLOW, BLACK, WHITE); 
    }
    else
    {
      show_ch1 = true;
      b_ch1.setColor(YELLOW, YELLOW, BLACK);
    }
    break;
  }
}

// When x_index is updated, check mark the right menu item in the TB menu,
// and set the button text to reflect the new setting
void check_tb_menu(int indx)
{
  char str[9];

  for (int i = 0; i < TB_MAX; i++)
    m_tb.checkMenuItem(i, false);
  m_tb.checkMenuItem(x_index, true);
  tb_tdiv_str(x_index, str);
  mb_tb.setText(str);
}

// When channel y_index[0] is updated, check mark the right menu item in its menu,
// and set the button text to reflect the new setting. Similar for channel 1.
void check_ch0_menu(int indx)
{
  char str[9];

  for (int i = 0; i < VOLTS_MAX; i++)
    m_ch0.checkMenuItem(i, false);
  m_ch0.checkMenuItem(y_index[0], true);
  ch_vdiv_str(y_index[0], str);
  mb_ch0.setText(str);
}

void check_ch1_menu(int indx)
{
  char str[9];

  for (int i = 0; i < VOLTS_MAX; i++)
    m_ch1.checkMenuItem(i, false);
  m_ch1.checkMenuItem(y_index[1], true);
  ch_vdiv_str(y_index[1], str);
  mb_ch1.setText(str);
}

// Callbacks are called whenever a menu item is selected.
// The timebase menu
void tb_menuCB(EventType ev, int indx, void *param, int x, int y)
{
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

  y_index[ch] = indx & 0xFF;
  if (ch == 0)
    check_ch0_menu(y_index[0]);
  else
    check_ch1_menu(y_index[1]);
}

void setup() 
{
  int i;
  char str[9];

  Serial.begin(9600);
  while(!Serial) {}  // TODO - remove this when going standalone

  // TODO: Move this code to a callback triggered by X pinch or tb menu selection..

  // Resolution, sample rate, number of samples per channel, queue depth.
  // 1 million SPS seems to be the limit (tested at 8 bit and 10 bit)
  // Changing sample time to 2_5 makes no difference.

  if (!adc.begin(ADC_RESOLUTION, tb[x_index].sps, 1024, 32)) {

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
  tb_tdiv_str(x_index, str);
  mb_tb.initButtonUL(30, 5, 90, 40, WHITE, GREY, WHITE, str, 1);
  m_tb.initMenu(&mb_tb, WHITE, DKGREY, GREY, WHITE, tb_menuCB, 2, NULL);
  for (i = 0; i < TB_MAX; i++)
  {
    tb_tdiv_str(i,str);
    m_tb.setMenuItem(i, str);
  }
  m_tb.checkMenuItem(x_index, true);

  // Channel 0
  if (show_ch0)
    b_ch0.initButtonUL(250, 5, 90, 40, YELLOW, YELLOW, BLACK, "CH0", 1, tapCB, 3, (void *)0);
  else
    b_ch0.initButtonUL(250, 5, 90, 40, YELLOW, BLACK, WHITE, "CH0", 1, tapCB, 3, (void *)0);

  ch_vdiv_str(y_index[0], str);
  mb_ch0.initButtonUL(350, 5, 90, 40, WHITE, GREY, WHITE, str, 1);
  m_ch0.initMenu(&mb_ch0, WHITE, DKGREY, GREY, WHITE, ch_menuCB, 4, (void *)0);
  for (i = 0; i < VOLTS_MAX; i++)
  {
    ch_vdiv_str(i, str);
    m_ch0.setMenuItem(i, str);
  }
  m_ch0.checkMenuItem(y_index[0], true);

  // Channel 1
  if (show_ch1)
    b_ch1.initButtonUL(450, 5, 90, 40, YELLOW, YELLOW, BLACK, "CH1", 1, tapCB, 5, (void *)1);
  else
    b_ch1.initButtonUL(450, 5, 90, 40, YELLOW, BLACK, WHITE, "CH1", 1, tapCB, 5, (void *)1);

  ch_vdiv_str(y_index[1], str);
  mb_ch1.initButtonUL(550, 5, 90, 40, WHITE, GREY, WHITE, str, 1);
  m_ch1.initMenu(&mb_ch1, WHITE, DKGREY, GREY, WHITE, ch_menuCB, 6, (void *)1);
  for (i = 0; i < VOLTS_MAX; i++)
  {
    ch_vdiv_str(i, str);
    m_ch1.setMenuItem(i, str);
  }
  m_ch1.checkMenuItem(y_index[1], true);

  // Trigger settings
  // TODO the default trigger setting here
  mb_trig.initButtonUL(670, 5, 90, 40, WHITE, GREY, WHITE, "OFF", 1);

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
      if ((millis() - last_millis) > 2000  && !m_tb.isAnyMenuDisplayed()) 
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

        // Draw the traces for each channel.
        if (show_ch0)
          draw_trace(buf, 0);
        if (show_ch1)
          draw_trace(buf, 1);

        last_millis = millis();
        tft.endBuffering();
      }
      // Release the buffer to return it to the pool.
      buf.release();
    }
}
