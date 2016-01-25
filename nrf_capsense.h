#include <stdint.h>


// Capsense evens.
enum capsense_event_t {CAPSENSE_BUTTON_EVENT, CAPSENSE_CALIBRATION_EVENT, CAPSENSE_TIMEOUT_EVENT};


// Call back event handler implemented by the application. The event
// will always be valid. However, the pin_mask will only be valid when
// the event is CAPSENSE_BUTTON_EVENT.
typedef void (*capsense_callback_t)(enum capsense_event_t event, uint32_t pin_mask);


// Capsense pin configuration struct. This holds configuration for
// specific to a single pin.
typedef struct
{
    uint32_t pin;
} nrf_capsense_pin_cfg_t;


// Configuration struct. This holds the general configuration
// of the library, and pointers to the specific configuration of each
// pin.
typedef struct
{
    nrf_capsense_pin_cfg_t *pins_cfg;
    uint32_t num_pins;
    capsense_callback_t callback;
} nrf_capsense_cfg_t;


// Function to initialize the capsense library. The supplied
// configuration array must be valid for as long as capsense is used
// and shall not be changed outside the library after the call to this
// function.
void nrf_capsense_init(nrf_capsense_cfg_t *cfg);


// Function to initiate sampling of all the registered capsense
// channels. Once sampling of all channels is complete the callback
// will be called if any change was registered after debouncing.
//
// Call regularly from the application in order to sample buttons (use
// apptimer library or RTC directly).
void nrf_capsense_sample(void);


// Function to calibrate the capacitive sensors. This simple
// calibration is based on the naive assumption that buttons are never
// pressed when calibration is run and that the environment never
// changes.
void nrf_capsense_calibrate(void);
