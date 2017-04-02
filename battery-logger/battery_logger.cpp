/*
 * COPYRIGHT
 * 
 *   This code is based on OpenMono (http://developer.openmono.com)
 * 
 *   Copyright (c) 2017 Monolit ApS                    ; original code
 *   Copyright (c) 2017 Markus Laire                   ; additions and changes
 * 
 * LICENSE
 * 
 *   Permission is hereby granted, free of charge, to any person obtaining a
 *   copy of this software and associated documentation files (the "Software"),
 *   to deal in the Software without restriction, including without limitation
 *   the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *   and/or sell copies of the Software, and to permit persons to whom the
 *   Software is furnished to do so, subject to the following conditions:
 *
 *   The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *   DEALINGS IN THE SOFTWARE.
 */

#include "battery_logger.h"

#include <mbed.h>
#include <stdio.h>
#include <inttypes.h>

/* BEGIN AUTO-GENERATED CODE */
// mV at 99%, 98%, ..., 1%
const uint16_t BatteryLogger::battery_percent_mV[] = {
  3957,3948,3941,3935,3928,3922,3915,3899,3890,3884,3878,3872,3866,3859,3854,
  3847,3841,3835,3829,3824,3818,3812,3807,3793,3767,3759,3754,3748,3743,3738,
  3733,3727,3722,3717,3712,3707,3702,3697,3696,3696,3694,3689,3686,3686,3686,
  3684,3680,3677,3674,3671,3668,3665,3662,3660,3657,3654,3652,3649,3648,3646,
  3644,3641,3639,3637,3636,3634,3632,3631,3629,3628,3626,3624,3622,3620,3618,
  3616,3614,3604,3604,3588,3585,3585,3573,3562,3558,3553,3547,3543,3538,3535,
  3532,3528,3524,3519,3511,3493,3473,3445,3379,0
};
/* END AUTO-GENERATED CODE */

// ------------=============-------------------------------------------------------
BatteryLogger::BatteryLogger(const char * filename, uint32_t autolog_interval_ms) :
// ------------=============-------------------------------------------------------
  autolog_timer(autolog_interval_ms),
  filename(filename)
{
  if (autolog_interval_ms != 0) {
    autolog_timer.setCallback<BatteryLogger>(this, &BatteryLogger::logVoltage);
    autolog_timer.Start();
  }
}

// -----------------===========--------------------
void BatteryLogger::appendToLog(const char *line) {
// -----------------===========--------------------
  FILE *fh = fopen(filename, "a");
  if (fh) {
    fwrite(line, 1, strlen(line), fh);
    fclose(fh);
  }
}

// --------------------===================---------------
uint8_t BatteryLogger::calculatePercentage(uint16_t mV) {
// --------------------===================---------------
  // While charging (high mV) or just after reset (zero mV),
  // battery percentage can't be determined -- just return 100
  // - note that normal algorithm below never returns 100 since it 'rounds down'
  if (mV > 4200 || mV == 0) return 100;

  // find first value less than mV
  int n = 0;
  while (battery_percent_mV[n] >= mV) {
    n++;
  }
  // if previous value equals mV, select it
  if (n > 0 && battery_percent_mV[n - 1] == mV) n--;

  return 100 - PERCENT_STEP_SIZE * (n + 1);
}

// -----------------==========----
void BatteryLogger::logVoltage() {
// -----------------==========----
  uint16_t mV = readVoltage();
  uint16_t percentage = calculatePercentage(mV);

  char buf[99];
  sprintf(buf, "%s %"PRIu16" mV (%2d %%)\n",
    DateTime::now().toTimeString()(), mV, percentage);
  appendToLog(buf);
}

// ---------------------===========----
uint16_t BatteryLogger::readVoltage() {
// ---------------------===========----
  static bool isStarted = false;
  if (! isStarted) {
    ADC_SAR_1_Start();
    isStarted = true;
  }

  // Disconnect AMUXBUSL ; Connect AG5 / CMP2 to AG5 / vref to CMP2
  CY_SET_REG8(CYDEV_ANAIF_RT_SAR0_SW3, CY_GET_REG8(CYDEV_ANAIF_RT_SAR0_SW3) & ~0x01);
  CY_SET_REG8(CYDEV_ANAIF_RT_SAR0_SW0, CY_GET_REG8(CYDEV_ANAIF_RT_SAR0_SW0) | 0x20);
  CY_SET_REG8(CYDEV_ANAIF_RT_CMP2_SW4, CY_GET_REG8(CYDEV_ANAIF_RT_CMP2_SW4) | 0x20);
  CY_SET_REG8(CYDEV_ANAIF_RT_CMP2_SW3, CY_GET_REG8(CYDEV_ANAIF_RT_CMP2_SW3) | 0x20);

  // wait for voltage level to settle
  wait_us(50);

  const uint32_t count_log = 6;
  int got_zero = false;
  uint32_t sum = 0;
  for (uint32_t n = 0; n < (uint32_t)1 << count_log; n++) {
    ADC_SAR_1_StartConvert();
    ADC_SAR_1_IsEndConversion(ADC_SAR_1_WAIT_FOR_RESULT);
    uint16_t reading = ADC_SAR_1_GetResult16();
    
    // After reset value seems to stay at 0 for some time.
    // In that case don't try to calculate average, just return 0.
    if (reading == 0) {
      got_zero = true;
      break;
    }
    
    sum += reading;
    wait_us(10);
  }
  uint32_t avg = sum >> count_log;

  // Disconnect CMP2 from vref and AG5 / AG5 from ADC ; Connect ADC to AMUXBUSL
  CY_SET_REG8(CYDEV_ANAIF_RT_CMP2_SW4, CY_GET_REG8(CYDEV_ANAIF_RT_CMP2_SW4) & ~0x20);
  CY_SET_REG8(CYDEV_ANAIF_RT_CMP2_SW3, CY_GET_REG8(CYDEV_ANAIF_RT_CMP2_SW3) & ~0x20);
  CY_SET_REG8(CYDEV_ANAIF_RT_SAR0_SW0, CY_GET_REG8(CYDEV_ANAIF_RT_SAR0_SW0) & ~0x20);
  CY_SET_REG8(CYDEV_ANAIF_RT_SAR0_SW3, CY_GET_REG8(CYDEV_ANAIF_RT_SAR0_SW3) | 0x01);

  if (got_zero) {
    return 0;
  } else {
    // scale from 12 bit ADC to mV
    return CORRECTION_SCALE / avg + CORRECTION_OFFSET;
  }
}
