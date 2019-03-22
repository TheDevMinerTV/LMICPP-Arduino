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

#ifndef _radio_h_
#define _radio_h_

#include "lorabase.h"
#include "osticks.h"
#include <stdint.h>

class Radio {

public:
  virtual void init(void) = 0;
  virtual void rst() const = 0;
  virtual void tx(uint32_t freq, rps_t rps, int8_t txpow,
                  uint8_t const *framePtr, uint8_t frameLength) = 0;
  virtual void rx(uint32_t freq, rps_t rps, uint8_t rxsyms, OsTime rxtime) = 0;

  virtual void init_random(uint8_t randbuf[16]) = 0;
  virtual uint8_t handle_end_rx(uint8_t *framePtr) = 0;
  virtual void handle_end_tx() const = 0;

  virtual bool io_check() const = 0;
  virtual uint8_t rssi() const = 0;
  virtual int16_t get_last_packet_rssi() const = 0;
  virtual int8_t get_last_packet_snr_x4() const = 0;
};

#endif