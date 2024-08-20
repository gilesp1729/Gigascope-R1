# Oscilloscope for the Arduino Giga R1 and the Giga Display
This board boasts impressive ADC statistics with two independent 1Msps ADC's and a configurable op-amp,
making it possible to have a competent scope with few or no additional components. 

The display is 800x480, which is just about enough for a small scope (I'd love a 7" smartphone sized display,
can we have one soon please?). UI clutter is kept to a minimum by using touch screen gestures made possible
by the multi-touch hardware, but there are also menus to select options more conventionally.

Library dependencies:
- Arduino_AdvancedAnalog for trace acquisition
- GU_Elements for UI elements (gilesp1729/GU_Elements)
- GestureDetector for gesture interaction (gilesp1729/GestureDetector)
- FontCollection (gilesp1729/FontCollection)
- Arduino_GigaDisplay_GFX for screen display. This requires the gilesp1729 fork for the moment; a PR is pending but
I'm not holding my breath. To use the original GFX, simply comment out the startBuffering() and endBuffering() calls.
You will notice a little more screen flicker.
