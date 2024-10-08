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

// TODO! This will depend on the AFE connected to the input, and may not
// be the same for all voltage ranges. The below code assumes no AFE 
// (all inputs are 0-3.3V unsigned)

// Voltage at an unsigned input resulting in the highest count (ADC_RANGE)
// assuming 0V is zero (if signed, it's half of that)
#define V_MAX         3.3f

// Number of voltage ranges
#define VOLTS_MAX     6

// Voltage range entries for each channel
typedef struct Voltage
{
  float v_div;      // volts/div
  float pix_count;  // Pixels per ADC count
} Voltage;

Voltage voltage[VOLTS_MAX] =
{
  { 0.1f,  (V_MAX * PIX_DIV) / (ADC_RANGE * 0.1f)},
  { 0.2f,  (V_MAX * PIX_DIV) / (ADC_RANGE * 0.2f)},
  { 0.5f,  (V_MAX * PIX_DIV) / (ADC_RANGE * 0.5f)},
  { 1.0f,  (V_MAX * PIX_DIV) / (ADC_RANGE * 1.0f)},
  { 2.0f,  (V_MAX * PIX_DIV) / (ADC_RANGE * 2.0f)},
  { 5.0f,  (V_MAX * PIX_DIV) / (ADC_RANGE * 5.0f)}
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
  bool      signed_v;   // If reading is signed, use the sign_offset (ADC_RANGE / 2), otherwise zero 
  GU_Button *b;         // Channel toggle button
  GU_Button *mb;        // Channel menu button
  GU_Menu   *m;         // Channel menu
} Channel;

Channel chan[2] =
{
  {3, 360, true,  0, 0, 0, 0, 0, 0, YELLOW, false, NULL, NULL, NULL },
  {3, 400, false, 0, 0, 0, 0, 0, 0, CYAN,   false, NULL, NULL, NULL }
};

// Trigger level on ch0, and whether rising or falling.
// Default levels are:
// - rising, logic HIGH (2.5V)
// - falling, logic LOW (0.5V)
// There is a hysteresis band of 0.15V.
float rising_level = 2.5f;        // TODO - this is assuming VMAV is 3.3V. Should change with v/div.
float falling_level = 0.5f;
float level_hyst = 0.15f;
float trig_level = rising_level;
int trig = 0;     // 0 = Off, 1 = rising, 2 = falling
int trig_ch = 0;  // Channel number to trigger (and to display frequency etc)
int trig_x = 0;   // X position of trigger point. Initially at left edge (0)
