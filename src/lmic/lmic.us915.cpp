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
#include "../hal/print_debug.h"

#include "lmic.us915.h"
#include "bufferpack.h"
#include "lmic_table.h"
#include <algorithm>

namespace {

// Default frequency plan for US 915MHz

constexpr uint32_t US915_125kHz_UPFBASE = 902300000;
constexpr uint32_t US915_125kHz_UPFSTEP = 200000;
constexpr uint32_t US915_500kHz_UPFBASE = 903000000;
constexpr uint32_t US915_500kHz_UPFSTEP = 1600000;
constexpr uint32_t US915_500kHz_DNFBASE = 923300000;
constexpr uint32_t US915_500kHz_DNFSTEP = 600000;

constexpr uint32_t US915_FREQ_MIN = 902000000;
constexpr uint32_t US915_FREQ_MAX = 928000000;
const OsDeltaTime DNW2_SAFETY_ZONE = OsDeltaTime::from_ms(750);
constexpr uint8_t CHNL_DNW2 = 0;
constexpr uint32_t FREQ_DNW2 =
    US915_500kHz_DNFBASE + CHNL_DNW2 * US915_500kHz_DNFSTEP;
constexpr dr_t DR_DNW2 = LmicUs915::SF12CR;
} // namespace

#define maxFrameLen(dr)                                                        \
  ((dr) <= DR_SF11CR ? TABLE_GET_U1(maxFrameLens, (dr)) : 0xFF)
CONST_TABLE(uint8_t, maxFrameLens)
[] = {24, 66, 142, 255, 255, 255, 255, 255, 66, 142};

namespace {
constexpr uint8_t rps_DR0 =
    rps_t{SF10, BandWidth::BW125, CodingRate::CR_4_5, false}.rawValue();
constexpr uint8_t rps_DR1 =
    rps_t{SF9, BandWidth::BW125, CodingRate::CR_4_5, false}.rawValue();
constexpr uint8_t rps_DR2 =
    rps_t{SF8, BandWidth::BW125, CodingRate::CR_4_5, false}.rawValue();
constexpr uint8_t rps_DR3 =
    rps_t{SF7, BandWidth::BW125, CodingRate::CR_4_5, false}.rawValue();
constexpr uint8_t rps_DR4 =
    rps_t{SF8, BandWidth::BW500, CodingRate::CR_4_5, false}.rawValue();

constexpr uint8_t rps_DR8 =
    rps_t{SF12, BandWidth::BW500, CodingRate::CR_4_5, false}.rawValue();
constexpr uint8_t rps_DR9 =
    rps_t{SF11, BandWidth::BW500, CodingRate::CR_4_5, false}.rawValue();
constexpr uint8_t rps_DR10 =
    rps_t{SF10, BandWidth::BW500, CodingRate::CR_4_5, false}.rawValue();
constexpr uint8_t rps_DR11 =
    rps_t{SF9, BandWidth::BW500, CodingRate::CR_4_5, false}.rawValue();
constexpr uint8_t rps_DR12 =
    rps_t{SF8, BandWidth::BW500, CodingRate::CR_4_5, false}.rawValue();
constexpr uint8_t rps_DR13 =
    rps_t{SF7, BandWidth::BW500, CodingRate::CR_4_5, false}.rawValue();

} // namespace

CONST_TABLE(uint8_t, _DR2RPS_CRC)
[] = {ILLEGAL_RPS, rps_DR0,     rps_DR1,     rps_DR2,    rps_DR3, rps_DR4,
      ILLEGAL_RPS, ILLEGAL_RPS, ILLEGAL_RPS, rps_DR8,    rps_DR9, rps_DR10,
      rps_DR11,    rps_DR12,    rps_DR13,    ILLEGAL_RPS};

uint8_t LmicUs915::getRawRps(dr_t dr) const {
  return TABLE_GET_U1(_DR2RPS_CRC, dr + 1);
}

int8_t LmicUs915::pow2dBm(uint8_t powerIndex) const {
  if (powerIndex >= 15) {
    return InvalidPower;
  }
  return 30 - (powerIndex * 2);
}

OsDeltaTime LmicUs915::getDwn2SafetyZone() const { return DNW2_SAFETY_ZONE; }

// Table below defines the size of one symbol as
//   symtime = 256us * 2^T(sf,bw)
// 256us is called one symunit.
//                 SF:
//      BW:      |__7___8___9__10__11__12
//      125kHz   |  2   3   4   5   6   7
//      250kHz   |  1   2   3   4   5   6
//      500kHz   |  0   1   2   3   4   5
//
// Times for half symbol per DR
// Per DR table to minimize rounding errors
static CONST_TABLE(int32_t, DR2HSYM)[] = {
    OsDeltaTime::from_us_round(128 << 5).tick(), // DR_SF10   DR_SF12CR
    OsDeltaTime::from_us_round(128 << 4).tick(), // DR_SF9    DR_SF11CR
    OsDeltaTime::from_us_round(128 << 3).tick(), // DR_SF8    DR_SF10CR
    OsDeltaTime::from_us_round(128 << 2).tick(), // DR_SF7    DR_SF9CR
    OsDeltaTime::from_us_round(128 << 1).tick(), // DR_SF8C   DR_SF8CR
    OsDeltaTime::from_us_round(128 << 0).tick()  // ------    DR_SF7CR
};

// map DR_SFnCR -> 0-6
OsDeltaTime LmicUs915::dr2hsym(dr_t dr) const {
  return OsDeltaTime(TABLE_GET_S4(DR2HSYM, dr & 7));
}

bool LmicUs915::validRx1DrOffset(uint8_t drOffset) const {
  return drOffset < 4;
}

void LmicUs915::initDefaultChannels() {
  for (uint8_t i = 0; i < 4; i++)
    channelMap[i] = 0xFFFF;
  channelMap[4] = 0x00FF;
}

uint32_t LmicUs915::convFreq(const uint8_t *ptr) const {
  uint32_t newfreq = rlsbf3(ptr) * 100;
  if (newfreq < US915_FREQ_MIN || newfreq > US915_FREQ_MAX)
    newfreq = 0;
  return newfreq;
}

void LmicUs915::handleCFList(const uint8_t *ptr) {
  // Check CFList type
  if (ptr[15] != 1) {
    PRINT_DEBUG(2, F("Wrong cflist type %d"), ptr[15]);
    return;
  }
  for (uint8_t chMaskCntl = 0; chMaskCntl < 5; chMaskCntl++, ptr += 2) {
    uint16_t chMask = rlsbf2(ptr);
    mapChannels(chMaskCntl, chMask);
    PRINT_DEBUG(2, F("Setup mask %d to %d"), chMaskCntl, chMask);
  }
}

bool LmicUs915::setupChannel(uint8_t, uint32_t, uint16_t) {
  // channels 0..71 are hardwired
  return false;
}

void LmicUs915::disableChannel(uint8_t channel) {
  if (channel < 72)
    channelMap[channel >> 4] &= ~(1 << (channel & 0xF));
}

void LmicUs915::enableChannel(uint8_t channel) {
  if (channel < 72)
    channelMap[channel >> 4] |= (1 << (channel & 0xF));
}

void LmicUs915::enableSubBand(uint8_t band) {
  ASSERT(band < 8);
  uint8_t start = band * 8;
  uint8_t end = start + 8;
  for (int channel = start; channel < end; ++channel)
    enableChannel(channel);
}
void LmicUs915::disableSubBand(uint8_t band) {
  ASSERT(band < 8);
  uint8_t start = band * 8;
  uint8_t end = start + 8;
  for (int channel = start; channel < end; ++channel)
    disableChannel(channel);
}
void LmicUs915::selectSubBand(uint8_t band) {
  ASSERT(band < 8);
  for (int b = 0; b < 8; ++b) {
    if (band == b)
      enableSubBand(b);
    else
      disableSubBand(b);
  }
}

// special channel page enable, bits applied to 64..71
constexpr uint8_t MCMD_LADR_CHP_125ON = 0x06;
//  ditto
constexpr uint8_t MCMD_LADR_CHP_125OFF = 0x07;

bool LmicUs915::validMapChannels(uint8_t const chMaskCntl, uint16_t const) {
  if (chMaskCntl == MCMD_LADR_CHP_125ON || chMaskCntl == MCMD_LADR_CHP_125OFF)
    return true;
  if (chMaskCntl < 5)
    return true;

  // TODO handle chMaskCntl = 5

  return false;
}

void LmicUs915::mapChannels(uint8_t chMaskCntl, uint16_t chMask) {
  if (chMaskCntl == MCMD_LADR_CHP_125ON || chMaskCntl == MCMD_LADR_CHP_125OFF) {
    uint16_t en125 = chMaskCntl == MCMD_LADR_CHP_125ON ? 0xFFFF : 0x0000;
    for (uint8_t u = 0; u < 4; u++)
      channelMap[u] = en125;
    channelMap[64 / 16] = chMask;
  } else if (chMaskCntl < 5) {
    channelMap[chMaskCntl] = chMask;
  }
  // TODO handle chMaskCntl = 5
}

uint32_t LmicUs915::getTxFrequency() const {
  uint8_t chnl = txChnl;
  ASSERT(chnl < 64 + 8);

  if (chnl < 64) {
    return US915_125kHz_UPFBASE + chnl * US915_125kHz_UPFSTEP;
  }
  return US915_500kHz_UPFBASE + (chnl - 64) * US915_500kHz_UPFSTEP;
}

int8_t LmicUs915::getTxPower() const {
  if (txChnl < 64) {
    return 30;
  }
  return 26;
};

void LmicUs915::updateTxTimes(OsDeltaTime) {}

// US does not have duty cycling - return now as earliest TX time
OsTime LmicUs915::nextTx(OsTime now) {
  if (chRnd == 0)
    chRnd = rand.uint8() & 0x3F;
  if (datarate >= SF8C) { // 500kHz
    uint8_t map = channelMap[64 / 16] & 0xFF;
    for (uint8_t i = 0; i < 8; i++) {
      if ((map & (1 << (++chRnd & 7))) != 0) {
        txChnl = 64 + (chRnd & 7);
        return now;
      }
    }
  } else { // 125kHz
    for (uint8_t i = 0; i < 64; i++) {
      uint8_t chnl = ++chRnd & 0x3F;
      if ((channelMap[(chnl >> 4)] & (1 << (chnl & 0xF))) != 0) {
        txChnl = chnl;
        return now;
      }
    }
  }
  // No feasible channel  found! Keep old one.
  return now;
}

uint32_t LmicUs915::getRx1Frequency() const {

  return US915_500kHz_DNFBASE + (txChnl & 0x7) * US915_500kHz_DNFSTEP;
}

dr_t LmicUs915::getRx1Dr() const {
  // |Upstream data rate  | Downstream data rate      |
  // |RX1DROffset         |  0   |  1   |  2   |  3   |
  // |--------------------| ---- | ---- | ---- | ---- |
  // |        DR0         | DR10 | DR9  | DR8  | DR8  |
  // |        DR1         | DR11 | DR10 | DR9  | DR8  |
  // |        DR2         | DR12 | DR11 | DR10 | DR9  |
  // |        DR3         | DR13 | DR12 | DR11 | DR10 |
  // |        DR4         | DR13 | DR13 | DR12 | DR11 |

  if (datarate < 4) {
    auto const dr_offset0 = datarate + 10;
    return lowerDR(dr_offset0, rx1DrOffset);
  } else if (datarate == 4) {
    if (rx1DrOffset == 0) {
      return 13;
    } else {
      return lowerDR(13, rx1DrOffset - 1);
    }
  }
  return datarate;
}

FrequencyAndRate LmicUs915::getRx1Parameter() const {
  return {getRx1Frequency(), getRx1Dr()};
}

void LmicUs915::initJoinLoop() {
  chRnd = 0;
  txChnl = 0;
  adrTxPow = 20;
  txend = os_getTime() + OsDeltaTime::rnd_delay(rand, 8);
  setDrJoin(SF7);
}

bool LmicUs915::nextJoinState() {
  // Try the following:
  //   SF7/8/9/10  on a random channel 0..63
  //   SF8C        on a random channel 64..71
  //
  bool failed = false;
  if (datarate != SF8C) {
    txChnl = 64 + (txChnl & 7);
    datarate = SF8C;
  } else {
    txChnl = rand.uint8() & 0x3F;
    int8_t dr = SF7 - ++txCnt;
    if (dr < SF10) {
      dr = SF10;
      failed = true; // All DR exhausted - signal failed
    }
    datarate = dr;
  }
  txend = os_getTime();

  return !failed;
}

FrequencyAndRate LmicUs915::defaultRX2Parameter() const {
  return {FREQ_DNW2, DR_DNW2};
}

LmicUs915::LmicUs915(Radio &aradio)
    : Lmic(aradio) {}
