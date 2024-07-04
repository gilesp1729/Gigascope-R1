#include <Arduino_AdvancedAnalog.h>
#include <GestureDetector.h>
#include <Arduino_GigaDisplay_GFX.h>
#include "gscope.h"

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

void setup() {
    Serial.begin(9600);

    // Resolution, sample rate, number of samples per channel, queue depth.
    // 1 million SPS seems to be the limit (tested at 8 bit and 10 bit)
    // Changing sample time to 2_5 makes no difference.
    if (!adc.begin(AN_RESOLUTION_8, 1000000, 1024, 32)) {
        Serial.println("Failed to start analog acquisition!");
        while (1);
    }

    // 1kHz square wave output for testing
    pinMode(2, OUTPUT);
    tone(2, 1000);
}

void loop() {
    int i, p, s, samples;

    // Poll for gesture input




    // Acquire and display a trace
    if (adc.available()) {
        SampleBuffer buf = adc.read();
#if 0
        Serial.print("Channels: ");
        Serial.print(buf.channels());
        Serial.print(" Interleaved: ");
        Serial.print(buf.getflags(DMA_BUFFER_INTRLVD));
        Serial.print(" Size: ");
        Serial.println(buf.size());
#endif
        samples = buf.size() / buf.channels();
        
        // Process the buffer. Interleaved samples.
        if ((millis() - last_millis) > 20) 
        {
          for (i = 0, p = 0; i < samples; i++)
          {
            Serial.print(buf[p]);
            p++;
            Serial.print(" ");
            Serial.println(buf[p]);
            p++;
          }
          last_millis = millis();
        }
        // Release the buffer to return it to the pool.
        buf.release();
    }
}
