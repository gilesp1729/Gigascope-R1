// Pixels per div in both X and Y
#define PIX_DIV     80

// Max number of timebase settings
#define TB_MAX      10

// Timebase entries for each time/div setting
typedef struct Timebase
{
  float t_div;      // time in us per div
  long  sps;        // samples per second
  int   p_sam;      // X pixels per sample
} Timebase;

// The SPS cannot exceed 1 million (you get aliasing) so for
// short t/div we stretch out the pixels per sample instead.
// Choose carefully so as not to get integer division surprises
// (the first three will work for a PIX_DIV of 60 or 80)
Timebase tb[TB_MAX] = 
{
//  t_div   sps                       p_sam
  { 10,     1000000,                  PIX_DIV / 10},
  { 20,     1000000,                  PIX_DIV / 20},
  { 50,     1000000 * PIX_DIV / 100,  2           },
  { 100,    1000000 * PIX_DIV / 100,  1           },
  { 200,    1000000 * PIX_DIV / 200,  1           },
  { 500,    1000000 * PIX_DIV / 500,  1           },
  { 1000,   1000000 * PIX_DIV / 1000, 1           },
  { 2000,   1000000 * PIX_DIV / 2000, 1           },
  { 5000,   1000000 * PIX_DIV / 5000, 1           },
  { 10000,  1000000 * PIX_DIV / 10000,1           }
};

// The index into the timebase table above.
int x_index = 6;

// The pixel position of the start of the trace. May be negative.
int x_offset = 0;

// Bit resolution of the ADC's
#define ADC_RESOLUTION AN_RESOLUTION_10
#define ADC_BITS      10
#define ADC_RANGE     (1 << ADC_BITS)

// Number of samples taken in trace
#define N_SAMPLES     2048

// This will depend on the AFE connected to the input, and may not
// be the same for all voltage ranges. 

// Min and max voltages for the first range of the Fscope-500k AFE.
//#define V_MAX           5.917f
//#define V_MIN          -5.872f
//#define SIGN_OFFSET     (-(V_MIN) / (V_MAX - V_MIN)) * ADC_RANGE

// Min and max voltages for the 4 ranges of the Fscope-500k AFE.
// The sign offset for each range is calculated using the formula above.
typedef struct Vrange
{
  float     v_min;          // Min and max voltages
  float     v_max;
  int       sign_offset;    // Sign offset in ADC counts (the ADC count represeting zero volts)
  float     rising_level;   // Rising and falling trigger levels (in volts) for this range
  float     falling_level;
  float     level_hyst;     // Hysteresis band to mminimise false triggers
} Vrange;

// Voltage ranges of the Fscope-500k AFE board. The index (0-3) is fed to 
// the voltage select pins (2 for each channel)
Vrange range[4] =
{
  // v_min   v_max   sign_off  rising  falling  hyst
  { -5.872f, 5.917f, 509,       2.5f,   0.8f,   0.15f },
  { -2.152f, 2.497f, 473,       1.25f,  0.4f,   0.075f },
  { -1.121f, 0.949f, 553,       0.5f,   0.2f,   0.03f },
  { -0.404f, 0.595f, 413,       0.25f,  0.1f,   0.02f }
};

#define V_MAX(r)        range[r].v_max
#define V_MIN(r)        range[r].v_min
#define V_RANGE(r)      (range[r].v_max - range[r].v_min)
#define SIGN_OFFSET(r)  range[r].sign_offset

// Number of volts/div ranges
#define VOLTS_MAX     5

// Volts/div range entries for each channel
typedef struct Voltage
{
  float v_div;      // volts/div
  float pix_count;  // Pixels per ADC count
  int   range_idx;  // Voltage range index. 
} Voltage;

Voltage voltage[VOLTS_MAX] =
{
//  v_div   pix_count                                   range_idx
  { 0.1f,  (V_RANGE(3) * PIX_DIV) / (ADC_RANGE * 0.1f), 3},
  { 0.2f,  (V_RANGE(2) * PIX_DIV) / (ADC_RANGE * 0.2f), 2},
  { 0.5f,  (V_RANGE(1) * PIX_DIV) / (ADC_RANGE * 0.5f), 1}, 
  { 1.0f,  (V_RANGE(0) * PIX_DIV) / (ADC_RANGE * 1.0f), 0},
  { 2.0f,  (V_RANGE(0) * PIX_DIV) / (ADC_RANGE * 2.0f), 0}
};

typedef struct Channel
{
  int       y_index;    // The index into the voltage range table
  int       y_offset;   // The pixel position of the zero voltage point
  bool      shown;      // Set to show trace for channel
  int       y_min;      // Min and max Y extent on screen
  int       y_max;
  float     v_min;      // Min and max voltages on input
  float     v_max;
  int       trig_pt;    // Trigger point (a sample number)
  float     freq;       // Frequency of signal (Hz)
  uint16_t  color;      // Color of traces and channel buttons
  GU_Button *b;         // Channel toggle button
  GU_Button *mb;        // Channel menu button
  GU_Menu   *m;         // Channel menu
  int       pin0;       // Pins to use to set voltage range to AFE board
  int       pin1;
} Channel;

Channel chan[2] =
{
  {3, 360, true,  0, 0, 0, 0, 0, 0, YELLOW, NULL, NULL, NULL, 3, 4 },
  {3, 400, false, 0, 0, 0, 0, 0, 0, CYAN,   NULL, NULL, NULL, 5, 6 }
};

// Trigger levels, and whether rising or falling.
// Default levels are:
// - rising, logic HIGH (2.5V = 75% of +3.3V logic supply)
// - falling, logic LOW (0.8V = 25%)
// There is a hysteresis band of 0.15V (= 5%). When a voltage range is changed, the trigger
// level and hyst band are readjusted to bring them back into the new range.
float trig_level = range[0].rising_level;
int trig = 0;     // 0 = Off, 1 = rising, 2 = falling
int trig_ch = 0;  // Channel number to trigger on (and to display frequency etc)
int trig_x = 20;  // X position of trigger point. Initially at or near left edge (0)
