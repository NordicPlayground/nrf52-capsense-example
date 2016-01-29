/* Copyright (c) 2016 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

#include <stdint.h>
#include "nrf_capsense_cfg.h"

// Capsense event.
enum capsense_event_t {CAPSENSE_BUTTON_EVENT, CAPSENSE_CALIBRATION_EVENT, CAPSENSE_TIMEOUT_EVENT};


// Call back event handler implemented by the application. The event
// will always be valid. However, the pin_mask will only be valid when
// the event is CAPSENSE_BUTTON_EVENT.
typedef void (*capsense_callback_t)(enum capsense_event_t event, uint32_t pin_mask);


// Configuration struct. This holds the general configuration of the
// library.
typedef struct
{
    uint32_t analog_pins[CAPSENSE_NUM_BUTTONS];   // Analog input pins
    capsense_callback_t callback;                 // Callback function pointer
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
// changes. The calibration algorithm must be significantly improved
// before used in an end product. (A proper calibration mechanism must
// be able to properly handle changes in the environment, but not
// mistake e.g. a very long touch as a change in the environment.)
void nrf_capsense_calibrate(void);
