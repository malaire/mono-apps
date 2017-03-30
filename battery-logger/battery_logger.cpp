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

// -----------------==========----
void BatteryLogger::logVoltage() {
// -----------------==========----
  uint16_t mV = readVoltage();

  char buf[99];
  sprintf(buf, "%s %"PRIu16" mV\n", DateTime::now().toTimeString()(), mV);
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
