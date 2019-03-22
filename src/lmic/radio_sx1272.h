/*******************************************************************************
 * Copyright (c) 2014-2015 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *    IBM Zurich Research Lab - initial API, implementation and documentation
 *    Nicolas Graziano - cpp style.
 *******************************************************************************/
/*
#ifndef _radio_sx1272_h_
#define _radio_sx1272_h_

#include "../hal/hal_io.h"
#include "lorabase.h"
#include "osticks.h"
#include <stdint.h>

class Radio {

public:
  explicit Radio(lmic_pinmap const &pins);
  void init(void);
  void rst() const;
  void tx(uint32_t freq, rps_t rps, int8_t txpow, uint8_t const *framePtr,
          uint8_t frameLength);
  void rx(uint32_t freq, rps_t rps, uint8_t rxsyms, OsTime rxtime);

  void init_random(uint8_t randbuf[16]);
  uint8_t handle_end_rx(uint8_t *framePtr);
  void handle_end_tx() const;

  bool io_check() const;
  uint8_t rssi() const;

private:
  HalIo hal;

  void opmode(uint8_t mode) const;
  void opmodeLora() const;
  void configLoraModem(rps_t rps);
  void configChannel(uint32_t freq) const;
  void configPower(int8_t pw) const;
  void rxlora(uint8_t rxmode, uint32_t freq, rps_t rps, uint8_t rxsyms,
              OsTime rxtime);
  void rxrssi() const;
};

#endif
*/