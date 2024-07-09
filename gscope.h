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
  { 1000,   1000000 * PIX_DIV / 2000, 1           },
  { 1000,   1000000 * PIX_DIV / 5000, 1           },
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

// Voltage at highest count
#define V_MAX         3.3f

// Number of voltage ranges
#define VOLTS_MAX     6

// Voltage range entries for each channel
typedef struct Voltage
{
  float v_div;      // volts/div
  int   sign_offset; // 0 - zero is at bottom (otherwise ADC_RANGE/2 - in middle)
  float pix_count;  // Pixels per ADC count
} Voltage;

Voltage voltage[VOLTS_MAX] =
{
  { 0.1f, ADC_RANGE / 2, (V_MAX * PIX_DIV) / (ADC_RANGE * 0.1f)},
  { 0.2f, ADC_RANGE / 2, (V_MAX * PIX_DIV) / (ADC_RANGE * 0.2f)},
  { 0.5f, ADC_RANGE / 2, (V_MAX * PIX_DIV) / (ADC_RANGE * 0.5f)},
  { 1.0f, ADC_RANGE / 2, (V_MAX * PIX_DIV) / (ADC_RANGE * 1.0f)},
  { 2.0f, ADC_RANGE / 2, (V_MAX * PIX_DIV) / (ADC_RANGE * 2.0f)},
  { 5.0f, ADC_RANGE / 2, (V_MAX * PIX_DIV) / (ADC_RANGE * 5.0f)}
};

// The index into the voltage range table (one for each channel)
int y_index[2] = { 3, 3 };

// The pixel position of the zero voltage point per channel.
int y_offset[2] = { 240, 480 };

// Set to show channel 1 (channel 0 is always shown)
bool show_ch1 = false;

// Trigger level on ch0, and whether rising or falling


