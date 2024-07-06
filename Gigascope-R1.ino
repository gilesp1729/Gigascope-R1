#include <Arduino_AdvancedAnalog.h>
#include <Arduino_GigaDisplay_GFX.h>
#include <GestureDetector.h>
#include "gscope.h"

// Colours in RGB565.
#define GC9A01A_CYAN    (uint16_t)0x07FF
#define GC9A01A_RED     (uint16_t)0xf800
#define GC9A01A_BLUE    (uint16_t)0x001F
#define GC9A01A_GREEN   (uint16_t)0x07E0
#define GC9A01A_MAGENTA (uint16_t)0xF81F
#define GC9A01A_WHITE   (uint16_t)0xffff
#define GC9A01A_BLACK   (uint16_t)0x0000
#define GC9A01A_YELLOW  (uint16_t)0xFFE0

#define GREY (uint16_t)tft.color565(0x7F, 0x7F, 0x7F)
#define DKGREY (uint16_t)tft.color565(0x3F, 0x3F, 0x3F)

// Oscilloscope for the Giga R1 and Giga Display.
// Dual channel with two independent 10bit, 1Msps ADC's.

// Uses libraries:
// GestureDetector for screen interaction
// Arduino_GigaDisplay_GFX for screen display
// Arduino_AdvancedAnalog for trace acquisition
// (and all their dependencies)

AdvancedADC adc(A0, A1);
uint64_t last_millis = 0;

GestureDetector detector;
GigaDisplay_GFX tft;

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
      if ((millis() - last_millis) > 20) 
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

        // Draw the traces for each channel. TODO: put this in a subroutine
        prev_x = x_offset;
        prev_y = y_offset0 - (buf[0] - voltage[y_index0].sign_offset) * voltage[y_index0].pix_count;
        for (i = 2; i < samples; i += 2)
        {
          x = x_offset + i * tb[x_index].p_sam;
          y = y_offset0 - (buf[i] - voltage[y_index0].sign_offset) * voltage[y_index0].pix_count;
          tft.drawLine(prev_x, prev_y, x, y, 0xFFFF);
          prev_x = x;
          prev_y = y;
        }
        // Back for channel 1
        if (buf.channels() > 1 && show_ch1)
        {
          prev_x = x_offset + tb[x_index].p_sam;
          prev_y = y_offset1 - (buf[1] - voltage[y_index1].sign_offset) * voltage[y_index1].pix_count;
          for (i = 3; i < samples; i += 2)
          {
            x = x_offset + i * tb[x_index].p_sam;
            y = y_offset1 - (buf[i] - voltage[y_index1].sign_offset) * voltage[y_index1].pix_count;
            tft.drawLine(prev_x, prev_y, x, y, 0xFFFF);
            prev_x = x;
            prev_y = y;
          }
        }

        last_millis = millis();
        tft.endBuffering();
      }
      // Release the buffer to return it to the pool.
      buf.release();
    }
}
