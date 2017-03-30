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

#ifndef battery_logger_h
#define battery_logger_h

#include <mono.h>
#include <wireless/module_communication.h>
#include <wireless/redpine_module.h>
#include <stdint.h>

using namespace mono;

class BatteryLogger {
  Timer        autolog_timer;
  const char * filename;

public:
  static const uint32_t RAW_ADC_MAX            = 0xFFF;
  static const uint32_t REFERENCE_VOLTAGE      = 0x400;
  static const uint32_t CORRECTION_FACTOR      = 1588;
  static const uint32_t CORRECTION_DENOMINATOR = 1000;
  static const uint32_t CORRECTION_OFFSET      = 440;
  static const uint32_t CORRECTION_SCALE =
      RAW_ADC_MAX * CORRECTION_FACTOR
    / CORRECTION_DENOMINATOR * REFERENCE_VOLTAGE;

  BatteryLogger(const char * filename, uint32_t autolog_interval_ms);

  void            appendToLog(const char *line);
  void            logVoltage();
  static uint16_t readVoltage();    
};

#endif /* battery_logger_h */
