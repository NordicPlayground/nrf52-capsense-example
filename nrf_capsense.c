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

#include <stdbool.h>
#include <stdint.h>
#include "nrf.h"
#include "nrf_capsense.h"
#include "nrf_capsense_cfg.h"


typedef struct
{
    uint32_t cal_val_min;
    uint32_t cal_val_max;
    uint32_t cal_average;
} calibration_data_t;


static calibration_data_t m_calibration_data[CAPSENSE_NUM_BUTTONS];
static uint32_t m_current_pin_index = 0;
static nrf_capsense_cfg_t *m_cfg = 0;
static uint32_t m_pressed_mask = 0;
static bool m_calibration_active = false;
static uint32_t m_calibration_run = 0;
static uint32_t m_debounced_pin_mask = 0;
static uint32_t m_debounce_pressed_confidence_level[CAPSENSE_NUM_BUTTONS] = {0};
static uint32_t m_debounce_released_confidence_level[CAPSENSE_NUM_BUTTONS] = {0};


static void post_sampling_cleanup()
{
#if CAPSENSE_ALWAYS_CONSTANT_LATENCY == 0
    NRF_POWER->TASKS_CONSTLAT = 0;
#endif
}


static void sample_initiate()
{
    // Clear timer
    CAPSENSE_TIMER->TASKS_CLEAR = 1;

    // Set COMP pin and enable the COMP
    NRF_COMP->PSEL = m_cfg->analog_pins[m_current_pin_index];
    NRF_COMP->ENABLE = (COMP_ENABLE_ENABLE_Enabled << COMP_ENABLE_ENABLE_Pos);
    NRF_COMP->TASKS_START = 1;
}


// Return true if button is pressed
static bool analyze_sample(uint32_t sample)
{
    if (sample > (m_calibration_data[m_current_pin_index].cal_average + CAPSENSE_CALIBRATION_FILTER_MARGIN))
    {
        return true;
    }
    else
    {
        return false;
    }
}


static void debounce(uint32_t pin_mask)
{
    uint32_t prev_debounced_pin_mask = m_debounced_pin_mask;

    for (unsigned int i = 0; i < CAPSENSE_NUM_BUTTONS; i++)
    {
        bool pressed = (pin_mask & (1 << i)) > 0 ? true : false;

        if (pressed)
        {
            m_debounce_pressed_confidence_level[i]++;
            m_debounce_released_confidence_level[i] = 0;
            if (m_debounce_pressed_confidence_level[i] > CAPSENSE_DEBOUNCE_CONFIDENCE_THRESHOLD)
            {
                m_debounced_pin_mask |= (1 << i); // Tag button as pressed
                m_debounce_pressed_confidence_level[i] = 0;
            }
        }
        else
        {
            m_debounce_released_confidence_level[i]++;
            m_debounce_pressed_confidence_level[i] = 0;
            if (m_debounce_released_confidence_level[i] > CAPSENSE_DEBOUNCE_CONFIDENCE_THRESHOLD)
            {
                m_debounced_pin_mask &= ~(1 << i); // Tag button as released
                m_debounce_released_confidence_level[i] = 0;
            }
        }
    }

    if (prev_debounced_pin_mask != m_debounced_pin_mask)
    {
        // Change in button press state. Callback.
        post_sampling_cleanup();
        m_cfg->callback(CAPSENSE_BUTTON_EVENT, m_debounced_pin_mask);
    }
}


static void sample_finalize()
{
    uint32_t sample = CAPSENSE_TIMER->CC[0];
    bool pressed = analyze_sample(sample);

    if (pressed)
    {
        m_pressed_mask |= 1 << m_current_pin_index;
    }

    if (m_current_pin_index < (CAPSENSE_NUM_BUTTONS - 1))
    {
        // More pins to do...
        m_current_pin_index++;
        sample_initiate();
    }
    else
    {
        // This was the last pin. Time to debounce....
        post_sampling_cleanup();
        debounce(m_pressed_mask);
    }
}


static void config_comparator(void)
{
    // Configure the comparator (COMP). Pin number is not configured at
    // this stage, as it will anyway be set whenever sampling a pin.
    // The comparator is not enabled at this stage, as it will be done
    // whenever sampling a pin.
    NRF_COMP->REFSEL = (COMP_REFSEL_REFSEL_VDD << COMP_REFSEL_REFSEL_Pos);
    NRF_COMP->TH = (5 << COMP_TH_THDOWN_Pos) | (60 << COMP_TH_THUP_Pos);
    NRF_COMP->MODE = (COMP_MODE_MAIN_SE << COMP_MODE_MAIN_Pos) | (COMP_MODE_SP_High << COMP_MODE_SP_Pos);
    NRF_COMP->ISOURCE = (COMP_ISOURCE_ISOURCE_Ien10mA << COMP_ISOURCE_ISOURCE_Pos);
    // Trigger interrupt on EVENTS_DOWN
    NRF_COMP->INTENSET = COMP_INTEN_DOWN_Msk;
    // Shortcut between events_down and task_stop
    NRF_COMP->SHORTS = COMP_SHORTS_DOWN_STOP_Msk;
}


static void config_timer(void)
{
    // Use CC[0] for timing the period of the oscilator (will be set
    // by PPI). Use CC[1] as a timeout that triggers a interrupt.
    // 16 bit timer
    CAPSENSE_TIMER->PRESCALER = 0;
    CAPSENSE_TIMER->BITMODE = TIMER_BITMODE_BITMODE_16Bit << TIMER_BITMODE_BITMODE_Pos;
    // Configure timeout
    CAPSENSE_TIMER->CC[1] = 1000*16;
    CAPSENSE_TIMER->SHORTS = TIMER_SHORTS_COMPARE1_CLEAR_Msk | TIMER_SHORTS_COMPARE1_STOP_Msk;
    CAPSENSE_TIMER->INTENSET = TIMER_INTENSET_COMPARE1_Msk;
    // Clear timer
    CAPSENSE_TIMER->TASKS_CLEAR = 1;
}


static void config_ppi(void)
{
    // Use PPI to start timer at upward crossing
    NRF_PPI->CH[CAPSENSE_PPI_CH0].EEP = (uint32_t)&NRF_COMP->EVENTS_UP;
    NRF_PPI->CH[CAPSENSE_PPI_CH0].TEP = (uint32_t)&CAPSENSE_TIMER->TASKS_START;
    NRF_PPI->CHENSET = 1 << CAPSENSE_PPI_CH0;

    // Use PPI to capture timer at downward crossing to CC[0] and stop
    // the timer
    NRF_PPI->CH[CAPSENSE_PPI_CH1].EEP = (uint32_t)&NRF_COMP->EVENTS_DOWN;
    NRF_PPI->CH[CAPSENSE_PPI_CH1].TEP = (uint32_t)&CAPSENSE_TIMER->TASKS_CAPTURE[0];
    NRF_PPI->FORK[CAPSENSE_PPI_CH1].TEP = (uint32_t)&CAPSENSE_TIMER->TASKS_STOP;
    NRF_PPI->CHENSET = 1 << CAPSENSE_PPI_CH1;
}


static void enable_interrupts(void)
{
    NVIC_SetPriority(CAPSENSE_TIMER_IRQ, 3);
    NVIC_EnableIRQ(CAPSENSE_TIMER_IRQ);
    NVIC_SetPriority(LPCOMP_IRQn, 3);
    NVIC_EnableIRQ(LPCOMP_IRQn);
}


static void calibration_sample_finalize()
{
    uint32_t sample = CAPSENSE_TIMER->CC[0];

    if ((sample > m_calibration_data[m_current_pin_index].cal_val_max) ||
        (sample < m_calibration_data[m_current_pin_index].cal_val_min))
    {
        if (sample > m_calibration_data[m_current_pin_index].cal_val_max)
        {
            m_calibration_data[m_current_pin_index].cal_val_max = sample;
        }
        if (sample < m_calibration_data[m_current_pin_index].cal_val_min)
        {
            m_calibration_data[m_current_pin_index].cal_val_min = sample;
        }

        m_calibration_data[m_current_pin_index].cal_average =
            (m_calibration_data[m_current_pin_index].cal_val_max
             + m_calibration_data[m_current_pin_index].cal_val_min) / 2;
    }

    if (m_current_pin_index < (CAPSENSE_NUM_BUTTONS - 1))
    {
        // More pins to do...
        m_current_pin_index++;
        sample_initiate();
    }
    else
    {
        // This was the last pin of this run
        if (m_calibration_run < (CAPSENSE_CALIBRATION_RUNS - 1))
        {
            // More runs to do
            m_calibration_run++;
            m_current_pin_index = 0;
            sample_initiate();
        }
        else
        {
            // This was the last run
            m_calibration_active = false;
            m_cfg->callback(CAPSENSE_CALIBRATION_EVENT, 0);
        }
    }
}


void COMP_LPCOMP_IRQHandler(void)
{
    // This interrupt is triggered when a sample has been
    // collected. The "sample" is the half period of the oscillator
    // (which is depandent of the capacitance of the sensor).

    if (NRF_COMP->EVENTS_DOWN)
    {
        NRF_COMP->EVENTS_DOWN = 0;
        if (m_calibration_active)
        {
            calibration_sample_finalize();
        }
        else
        {
            sample_finalize();
        }
    }
}


void CAPSENSE_TIMER_IRQHandler(void)
{
    // This interrupt is only triggered when a timeout has occured.

    if (CAPSENSE_TIMER->EVENTS_COMPARE[1])
    {
        CAPSENSE_TIMER->EVENTS_COMPARE[1] = 0;
        CAPSENSE_TIMER->TASKS_STOP = 1;
        post_sampling_cleanup();
        m_cfg->callback(CAPSENSE_TIMEOUT_EVENT, 0);
    }
}


void nrf_capsense_init(nrf_capsense_cfg_t *cfg)
{
    m_cfg = cfg;

    for (unsigned int i = 0; i < CAPSENSE_NUM_BUTTONS; i++)
    {
        m_calibration_data[i].cal_val_min = ~0;
        m_calibration_data[i].cal_val_max = 0;
    }

    config_comparator();
    config_timer();
    config_ppi();
    enable_interrupts();
}


static void prepare_for_sampling()
{
    // Set constant latency mode to force the clock active. It will be
    // disabled again once sampling is completed.
#if CAPSENSE_ALWAYS_CONSTANT_LATENCY == 0
    NRF_POWER->TASKS_CONSTLAT = 1;
#endif
    // Initate first sample
    sample_initiate();
}


void nrf_capsense_sample(void)
{
    m_current_pin_index = 0;
    m_pressed_mask = 0;
    prepare_for_sampling();
}


void nrf_capsense_calibrate(void)
{
    m_calibration_active = true;
    m_current_pin_index = 0;
    m_calibration_run = 0;
    prepare_for_sampling();
}
