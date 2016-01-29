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
#include "nrf.h"
#include "nrf_log.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "app_error.h"
#include "boards.h"
#include "app_timer.h"
#include "nrf_drv_clock.h"
#include "nrf_capsense.h"
#include "nrf_capsense_cfg.h"


// Capsense configuration
#define CAPSENSE_INTERVAL_MS            10

// General application timer settings.
#define APP_TIMER_PRESCALER             16    // RTC PRESCALER register value.
#define APP_TIMER_OP_QUEUE_SIZE         2     // Size of timer operation queues.

APP_TIMER_DEF(m_capsense_timer);


static void nrf_log_init(void)
{
    // Initialize logging library. 
    uint32_t err_code = NRF_LOG_INIT();
    APP_ERROR_CHECK(err_code);
}


static void lfclk_request(void)
{
    uint32_t err_code = nrf_drv_clock_init();
    APP_ERROR_CHECK(err_code);
    nrf_drv_clock_lfclk_request(NULL);
}


static void apply_errata_workarounds(void)
{
    // [12] COMP: Reference ladder is not correctly calibrated
    //
    // This anomaly applies to Engineering A, B and C revisions.
    *(volatile uint32_t *)0x40013540 = (*(volatile uint32_t *)0x10000324 & 0x00001F00) >> 8;

    // [70] COMP: Not able to wake CPU from System ON IDLE.
    //
    // This anomaly applies to Engineering A and B revisions. Fixed in
    // Engineering C. This workaround will lead to increased power
    // consumption. Thus in order to get correct power meassurements
    // this has to be tested on a Engineering C or later *with this
    // workaround removed*.
    while (*(volatile uint32_t*)0x4006EC00 != 1)
    {
        *(volatile uint32_t*)0x4006EC00 = 0x9375;
    }
    if (*(volatile uint32_t*)0x4006EC00 != 0)
    {
        *(volatile uint32_t*)0x4006EC14 = 0x00C0;
    }
}


static void update_leds(uint32_t pin_mask)
{
    nrf_gpio_pin_write(LED_1, ((pin_mask & 0x01) ? 0 : 1));
    nrf_gpio_pin_write(LED_2, ((pin_mask & 0x02) ? 0 : 1));
}


static void capsense_button_event_handler(enum capsense_event_t event, uint32_t pin_mask)
{
    switch (event)
    {
    case CAPSENSE_BUTTON_EVENT:
        NRF_LOG_PRINTF("Capsense button mask update: %u\r\n", pin_mask);
        update_leds(pin_mask);
        break;

    case CAPSENSE_CALIBRATION_EVENT:
        NRF_LOG("Capsense calibration done\r\n");
        break;

    case CAPSENSE_TIMEOUT_EVENT:
        NRF_LOG_ERROR("Capsense timeout\r\n");
        break;
    }
}


static void init_capsense()
{
    static nrf_capsense_cfg_t cfg = {
        {2, 3},                         // Analog input pins (AIN).
        capsense_button_event_handler   // Callback function
    };

    nrf_capsense_init(&cfg);
}


static void capsense_timer_event_handler(void * p_context)
{
    nrf_capsense_sample();
}


static void init_leds()
{
    nrf_gpio_range_cfg_output(LED_START, LED_STOP);
    nrf_gpio_pins_set(LEDS_MASK);
}


static void init_timer()
{
    APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, false);

    uint32_t err_code = app_timer_create(&m_capsense_timer,
                                         APP_TIMER_MODE_REPEATED,
                                         capsense_timer_event_handler);
    APP_ERROR_CHECK(err_code);
}


static void timer_start()
{
    uint32_t err_code = app_timer_start(m_capsense_timer,
                                        APP_TIMER_TICKS(CAPSENSE_INTERVAL_MS, APP_TIMER_PRESCALER),
                                        NULL);
    APP_ERROR_CHECK(err_code);
}


static void power_down()
{
    // Make sure any pending events are cleared
    __SEV();
    __WFE();
    // Enter System ON sleep mode
    __WFE();
}


int main(void)
{
    apply_errata_workarounds();
    lfclk_request();
    nrf_log_init();
    init_timer();
    init_leds();
    init_capsense();
    nrf_capsense_calibrate();
    timer_start();

    while (true)
    {
        power_down();
    }
}
