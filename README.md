* Oscilloscope for the Arduino Giga R1 and the Giga Display
This board boasts impressive ADC statistics with two independent 1Msps ADC's and a configurable op-amp,
making it possible to have a competent scope with few or no additional components. 

The display is 800x480, which is just about enough for a small scope (I'd love a 7" smartphone sized display,
can we have one soon please?). UI clutter is kept to a minimum by using touch screen gestures made pissible
by the multi-touch hardware.

Library dependencies:
- GestureDetector for screen interaction (gilesp7647/GestureDetector for the moment)
- Arduino_GigaDisplay_GFX for screen display (requires gilesp7647 fork for the moment; PR pending)
- Arduino_AdvancedAnalog for trace acquisition
