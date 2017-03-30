/*
 * COPYRIGHT
 * 
 *   Copyright (c) 2017 Markus Laire
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

#include "app_controller.h"

#include <inttypes.h>

using namespace mono::geo;

const char * AppController::LOG_FILE = "/sd/battery_logger.txt";

// ------------=============----
AppController::AppController() :
// ------------=============----
  sd_fs(SD_SPI_MOSI, SD_SPI_MISO, SD_SPI_CLK, SD_SPI_CS, "sd"),
  label(Rect(0,100,176,20), "Starting ..."),
  timer(DISPLAY_UPDATE_INTERVAL_MS),
  logger(LOG_FILE, LOG_INTERVAL_MS)
{
  label.setAlignment(TextLabelView::ALIGN_CENTER);
  label.setTextColor(display::TurquoiseColor);
}

// -----------------=================----
void AppController::monoWakeFromReset() {
// -----------------=================----
  logger.appendToLog("--- RESET ---\n");
  label.show();
  timer.setCallback<AppController>(this, &AppController::updateDisplay);
  timer.Start();
}

// -----------------=================----
void AppController::monoWakeFromSleep() {
// -----------------=================----
  IApplicationContext::SoftwareResetToApplication();
}

// -----------------=================----
void AppController::monoWillGotoSleep() {}
// -----------------=================----

// -----------------=============----
void AppController::updateDisplay() {
// -----------------=============----
  uint16_t mV = BatteryLogger::readVoltage();
  
  char buf[99];
  sprintf(buf, "%s %"PRIu16" mV", DateTime::now().toTimeString()(), mV);  
  label.setText(buf);
}
