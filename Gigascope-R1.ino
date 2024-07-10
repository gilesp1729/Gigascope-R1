#include <Arduino_AdvancedAnalog.h>
#include <Arduino_GigaDisplay_GFX.h>
#include <GestureDetector.h>
#include "gscope.h"
#include "GU_Elements.h"
#include <fonts/FreeSans18pt7b.h>

// Colours in RGB565.
#define CYAN    (uint16_t)0x07FF
#define RED     (uint16_t)0xf800
#define BLUE    (uint16_t)0x001F
#define GREEN   (uint16_t)0x07E0
#define MAGENTA (uint16_t)0xF81F
#define WHITE   (uint16_t)0xffff
#define BLACK   (uint16_t)0x0000
#define YELLOW  (uint16_t)0xFFE0
#define GREY (uint16_t)tft.color565(0x7F, 0x7F, 0x7F)
#define DKGREY (uint16_t)tft.color565(0x3F, 0x3F, 0x3F)

// Oscilloscope for the Giga R1 and Giga Display.
// Dual channel with two independent 10bit, 1Msps ADC's.

// Uses libraries:
// GestureDetector for screen interaction
// GU_Elements for UI elements (included here for the moment)
// Arduino_GigaDisplay_GFX for screen display
// Arduino_AdvancedAnalog for trace acquisition
// (and all their dependencies)

AdvancedADC adc(A0, A1);
uint64_t last_millis = 0;

GestureDetector detector;
GigaDisplay_GFX tft;
GU_Button b_ch0, b_ch1, b_trig;

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

// Draw furniture at top of screen. Timebase, 2 channels, and trigger settings.
void draw_tb()
{
  char str[32];

  tft.setCursor(0, 30);
  tft.setTextColor(WHITE);
  if (tb[x_index].t_div >= 1000)
    sprintf(str, "%.0fms", tb[x_index].t_div / 1000);
  else
    sprintf(str, "%.0fus", tb[x_index].t_div);
  tft.print(str);

  if (tb[x_index].sps >= 1000000)
    sprintf(str, " (%dMsps)", tb[x_index].sps / 1000000);
  else
    sprintf(str, " (%dksps)", tb[x_index].sps / 1000);
  tft.print(str);
}

void setup() {
    Serial.begin(9600);

    // TODO: Move this code to a callback triggered by X pinch.

    // Resolution, sample rate, number of samples per channel, queue depth.
    // 1 million SPS seems to be the limit (tested at 8 bit and 10 bit)
    // Changing sample time to 2_5 makes no difference.
    if (!adc.begin(ADC_RESOLUTION, tb[x_index].sps, 1024, 32)) {
        Serial.println("Failed to start analog acquisition!");
        while (1);
    }

    tft.setRotation(1);
    detector.setRotation(1);
    
    tft.setFont(&FreeSans18pt7b);
    //tft.setTextSize(3);
    
    b_ch0.initButtonUL(&tft, 240, 2, 100, 40, BLACK, YELLOW, BLACK, "CH0", 1, 1);
    b_ch1.initButtonUL(&tft, 480, 2, 100, 40, BLACK, YELLOW, BLACK, "CH1", 1, 1);

    // 1kHz square wave output for testing
    pinMode(2, OUTPUT);
    tone(2, 1000);
}

void loop() {
    int i, p, s, samples;

    // Poll for gesture input




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
      if ((millis() - last_millis) > 2000) 
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
        b_ch0.drawButton();
        b_ch1.drawButton();

        // Draw the traces for each channel.
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
