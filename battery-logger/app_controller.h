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

#ifndef app_controller_h
#define app_controller_h

#include "battery_logger.h"

#include <mono.h>
#include <SDFileSystem.h>

using namespace mono;
using namespace mono::ui;

class AppController : public IApplication {
  SDFileSystem  sd_fs;
  TextLabelView label;
  TextLabelView percentageLabel;
  Timer         timer;
  BatteryLogger logger;

public:
  static const int    DISPLAY_UPDATE_INTERVAL_MS = 1000;
  static const int    LOG_INTERVAL_MS            = 60 * 1000;
  static const char * LOG_FILE;

  AppController();

  void monoWakeFromReset();
  void monoWakeFromSleep();
  void monoWillGotoSleep();
  void updateDisplay();
};

#endif /* app_controller_h */
