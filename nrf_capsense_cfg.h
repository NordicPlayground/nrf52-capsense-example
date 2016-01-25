// Number of sensors used for the Capsense library. The maximum number
// is 8, limited by the number of analog input pins on the nRF52.
#define CAPSENSE_NUM_BUTTONS                      2

// The number of consecutive samples indicating the same state
// (pressed / not pressed) that is required before a action is
// considered real and teh callback function is called.
#define CAPSENSE_DEBOUNCE_CONFIDENCE_THRESHOLD    5

// Define the timer that is used by the capsense library.
#define CAPSENSE_TIMER                            NRF_TIMER1
#define CAPSENSE_TIMER_IRQ                        TIMER1_IRQn
#define CAPSENSE_TIMER_IRQHandler                 TIMER1_IRQHandler

// PPI channel assignment for the capsense library.
#define CAPSENSE_PPI_CH0                          0
#define CAPSENSE_PPI_CH1                          1

// Calibration filter configuration.
#define CAPSENSE_CALIBRATION_FILTER_MARGIN        3
#define CAPSENSE_CALIBRATION_RUNS                 25

// Always use constant latency mode. The library will use constant
// latency mode while sampling. Normall it will be disabled after
// sampling, but if this define is set to non-null, keep constant
// latency mode.
#define CAPSENSE_ALWAYS_CONSTANT_LATENCY          0
