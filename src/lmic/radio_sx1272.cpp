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
#if 0
#include "radio.h"
#include "../hal/print_debug.h"

#include "../aes/lmic_aes.h"
#include "lmic_table.h"

// ----------------------------------------
// Registers Mapping
const uint8_t RegFifo = 0x00;     // common
const uint8_t RegOpMode = 0x01;   // common
const uint8_t RegFrfMsb = 0x06;   // common
const uint8_t RegFrfMid = 0x07;   // common
const uint8_t RegFrfLsb = 0x08;   // common
const uint8_t RegPaConfig = 0x09; // common
const uint8_t RegPaRamp = 0x0A;   // common
const uint8_t RegOcp = 0x0B;      // common
const uint8_t RegLna = 0x0C;      // common
const uint8_t LORARegFifoAddrPtr = 0x0D;
const uint8_t LORARegFifoTxBaseAddr = 0x0E;
const uint8_t LORARegFifoRxBaseAddr = 0x0F;
const uint8_t LORARegFifoRxCurrentAddr = 0x10;
const uint8_t LORARegIrqFlagsMask = 0x11;
const uint8_t LORARegIrqFlags = 0x12;
const uint8_t LORARegRxNbBytes = 0x13;
const uint8_t LORARegRxHeaderCntValueMsb = 0x14;
const uint8_t LORARegRxHeaderCntValueLsb = 0x15;
const uint8_t LORARegRxPacketCntValueMsb = 0x16;
const uint8_t LORARegRxpacketCntValueLsb = 0x17;
const uint8_t LORARegModemStat = 0x18;
const uint8_t LORARegPktSnrValue = 0x19;
const uint8_t LORARegPktRssiValue = 0x1A;
const uint8_t LORARegRssiValue = 0x1B;
const uint8_t LORARegHopChannel = 0x1C;
const uint8_t LORARegModemConfig1 = 0x1D;
const uint8_t LORARegModemConfig2 = 0x1E;
const uint8_t LORARegSymbTimeoutLsb = 0x1F;
const uint8_t LORARegPreambleMsb = 0x20;
const uint8_t LORARegPreambleLsb = 0x21;
const uint8_t LORARegPayloadLength = 0x22;
const uint8_t LORARegPayloadMaxLength = 0x23;
const uint8_t LORARegHopPeriod = 0x24;
const uint8_t LORARegFifoRxByteAddr = 0x25;
const uint8_t LORARegModemConfig3 = 0x26;
const uint8_t LORARegFeiMsb = 0x28;
const uint8_t LORAFeiMib = 0x29;
const uint8_t LORARegFeiLsb = 0x2A;
const uint8_t LORARegRssiWideband = 0x2C;
const uint8_t LORARegDetectOptimize = 0x31;
const uint8_t LORARegInvertIQ = 0x33;
const uint8_t LORARegDetectionThreshold = 0x37;
const uint8_t LORARegSyncWord = 0x39;
const uint8_t RegDioMapping1 = 0x40; // common
const uint8_t RegDioMapping2 = 0x41; // common
const uint8_t RegVersion = 0x42;     // common
const uint8_t RegPaDac = 0x4D;       // common

// ----------------------------------------
// spread factors and mode for RegModemConfig2
const uint8_t SX1272_MC2_FSK = 0x00;
const uint8_t SX1272_MC2_SF7 = 0x70;
const uint8_t SX1272_MC2_SF8 = 0x80;
const uint8_t SX1272_MC2_SF9 = 0x90;
const uint8_t SX1272_MC2_SF10 = 0xA0;
const uint8_t SX1272_MC2_SF11 = 0xB0;
const uint8_t SX1272_MC2_SF12 = 0xC0;
// bandwidth for RegModemConfig1
const uint8_t SX1272_MC1_BW_125 = 0x00;
const uint8_t SX1272_MC1_BW_250 = 0x40;
const uint8_t SX1272_MC1_BW_500 = 0x80;
// coding rate for RegModemConfig1
const uint8_t SX1272_MC1_CR_4_5 = 0x08;
const uint8_t SX1272_MC1_CR_4_6 = 0x10;
const uint8_t SX1272_MC1_CR_4_7 = 0x18;
const uint8_t SX1272_MC1_CR_4_8 = 0x20;
const uint8_t SX1272_MC1_IMPLICIT_HEADER_MODE_ON = 0x04; // required for receive
const uint8_t SX1272_MC1_RX_PAYLOAD_CRCON = 0x02;
const uint8_t SX1272_MC1_LOW_DATA_RATE_OPTIMIZE =
    0x01; // mandated for SF11 and SF12
// transmit power configuration for RegPaConfig
const uint8_t SX1272_PAC_PA_SELECT_PA_BOOST = 0x80;
const uint8_t SX1272_PAC_PA_SELECT_RFIO_PIN = 0x00;

// sx1276 RegModemConfig1
const uint8_t SX1276_MC1_BW_125 = 0x70;
const uint8_t SX1276_MC1_BW_250 = 0x80;
const uint8_t SX1276_MC1_BW_500 = 0x90;
const uint8_t SX1276_MC1_CR_4_5 = 0x02;
const uint8_t SX1276_MC1_CR_4_6 = 0x04;
const uint8_t SX1276_MC1_CR_4_7 = 0x06;
const uint8_t SX1276_MC1_CR_4_8 = 0x08;

const uint8_t SX1276_MC1_IMPLICIT_HEADER_MODE_ON = 0x01;

// sx1276 RegModemConfig2
const uint8_t SX1276_MC2_RX_PAYLOAD_CRCON = 0x04;

// sx1276 RegModemConfig3
const uint8_t SX1276_MC3_LOW_DATA_RATE_OPTIMIZE = 0x08;
const uint8_t SX1276_MC3_AGCAUTO = 0x04;

// preamble for lora networks (nibbles swapped)
const uint8_t LORA_MAC_PREAMBLE = 0x34;

const uint8_t RXLORA_RXMODE_RSSI_REG_MODEM_CONFIG1 = 0x0A;
#ifdef CFG_sx1276_radio
const uint8_t RXLORA_RXMODE_RSSI_REG_MODEM_CONFIG2 = 0x70;
#elif CFG_sx1272_radio
const uint8_t RXLORA_RXMODE_RSSI_REG_MODEM_CONFIG2 = 0x74;
#endif

// ----------------------------------------
// Constants for radio registers
const uint8_t OPMODE_LORA = 0x80;
const uint8_t OPMODE_MASK = 0x07;
const uint8_t OPMODE_SLEEP = 0x00;
const uint8_t OPMODE_STANDBY = 0x01;
const uint8_t OPMODE_FSTX = 0x02;
const uint8_t OPMODE_TX = 0x03;
const uint8_t OPMODE_FSRX = 0x04;
const uint8_t OPMODE_RX = 0x05;
const uint8_t OPMODE_RX_SINGLE = 0x06;
const uint8_t OPMODE_CAD = 0x07;

// ----------------------------------------
// Bits masking the corresponding IRQs from the radio
const uint8_t IRQ_LORA_RXTOUT_MASK = 0x80;
const uint8_t IRQ_LORA_RXDONE_MASK = 0x40;
const uint8_t IRQ_LORA_CRCERR_MASK = 0x20;
const uint8_t IRQ_LORA_HEADER_MASK = 0x10;
const uint8_t IRQ_LORA_TXDONE_MASK = 0x08;
const uint8_t IRQ_LORA_CDDONE_MASK = 0x04;
const uint8_t IRQ_LORA_FHSSCH_MASK = 0x02;
const uint8_t IRQ_LORA_CDDETD_MASK = 0x01;

// ----------------------------------------
// DIO function mappings                D0D1D2D3
const uint8_t MAP_DIO0_LORA_RXDONE = 0x00; // 00------
const uint8_t MAP_DIO0_LORA_TXDONE = 0x40; // 01------
const uint8_t MAP_DIO0_LORA_NOP = 0xC0;    // 11------
const uint8_t MAP_DIO1_LORA_RXTOUT = 0x00; // --00----
const uint8_t MAP_DIO1_LORA_NOP = 0x30;    // --11----
const uint8_t MAP_DIO2_LORA_NOP = 0x0C;    // ----11--

const uint8_t LNA_RX_GAIN = (0x20 | 0x03);

void Radio::opmode(uint8_t const mode) const {
  hal.write_reg(RegOpMode, (hal.read_reg(RegOpMode) & ~OPMODE_MASK) | mode);
}

void Radio::opmodeLora() const {
  uint8_t u = OPMODE_LORA;
#ifdef CFG_sx1276_radio
  u |= 0x8; // TBD: sx1276 high freq
#endif
  hal.write_reg(RegOpMode, u);
}

// configure LoRa modem (cfg1, cfg2)
void Radio::configLoraModem(rps_t rps) {
  sf_t const sf = rps.sf;

#ifdef CFG_sx1276_radio
  uint8_t mc1 = 0;

  switch (rps.getBw()) {
  case BandWidth::BW125:
    mc1 |= SX1276_MC1_BW_125;
    break;
  case BandWidth::BW250:
    mc1 |= SX1276_MC1_BW_250;
    break;
  case BandWidth::BW500:
    mc1 |= SX1276_MC1_BW_500;
    break;
  default:
    ASSERT(0);
  }
  switch (rps.getCr()) {
  case CodingRate::CR_4_5:
    mc1 |= SX1276_MC1_CR_4_5;
    break;
  case CodingRate::CR_4_6:
    mc1 |= SX1276_MC1_CR_4_6;
    break;
  case CodingRate::CR_4_7:
    mc1 |= SX1276_MC1_CR_4_7;
    break;
  case CodingRate::CR_4_8:
    mc1 |= SX1276_MC1_CR_4_8;
    break;
  default:
    ASSERT(0);
  }

  if (rps.ih) {
    mc1 |= SX1276_MC1_IMPLICIT_HEADER_MODE_ON;
    hal.write_reg(LORARegPayloadLength, rps.ih); // required length
  }
  // set ModemConfig1
  hal.write_reg(LORARegModemConfig1, mc1);

  uint8_t mc2 = (SX1272_MC2_SF7 + ((sf - 1) << 4));
  if (!rps.nocrc) {
    mc2 |= SX1276_MC2_RX_PAYLOAD_CRCON;
  }
  hal.write_reg(LORARegModemConfig2, mc2);

  uint8_t mc3 = SX1276_MC3_AGCAUTO;
  if ((sf == SF11 || sf == SF12) && rps.getBw() == BandWidth::BW125) {
    mc3 |= SX1276_MC3_LOW_DATA_RATE_OPTIMIZE;
  }
  hal.write_reg(LORARegModemConfig3, mc3);
#elif CFG_sx1272_radio
  uint8_t mc1 = (rps.bw << 6);

  switch (rps.cr) {
  case CR_4_5:
    mc1 |= SX1272_MC1_CR_4_5;
    break;
  case CR_4_6:
    mc1 |= SX1272_MC1_CR_4_6;
    break;
  case CR_4_7:
    mc1 |= SX1272_MC1_CR_4_7;
    break;
  case CR_4_8:
    mc1 |= SX1272_MC1_CR_4_8;
    break;
  }

  if ((sf == SF11 || sf == SF12) && rps.bw == BW125) {
    mc1 |= SX1272_MC1_LOW_DATA_RATE_OPTIMIZE;
  }

  if (rps.nocrc == 0) {
    mc1 |= SX1272_MC1_RX_PAYLOAD_CRCON;
  }

  if (rps.ih) {
    mc1 |= SX1272_MC1_IMPLICIT_HEADER_MODE_ON;
    hal.write_reg(LORARegPayloadLength, rps.ih); // required length
  }
  // set ModemConfig1
  hal.write_reg(LORARegModemConfig1, mc1);

  // set ModemConfig2 (sf, AgcAutoOn=1 SymbTimeoutHi=00)
  hal.write_reg(LORARegModemConfig2, (SX1272_MC2_SF7 + ((sf - 1) << 4)) | 0x04);
#else
#error Missing CFG_sx1272_radio/CFG_sx1276_radio
#endif /* CFG_sx1272_radio */
}

void Radio::configChannel(uint32_t const freq) const {
  // set frequency: FQ = (FRF * 32 Mhz) / (2 ^ 19)
  uint64_t const frf = ((uint64_t)freq << 19) / 32000000;
  hal.write_reg(RegFrfMsb, (uint8_t)(frf >> 16));
  hal.write_reg(RegFrfMid, (uint8_t)(frf >> 8));
  hal.write_reg(RegFrfLsb, (uint8_t)(frf >> 0));
}

#define PA_BOOST_PIN 1

void Radio::configPower(int8_t pw) const {
#ifdef CFG_sx1276_radio

#if PA_BOOST_PIN
  // no boost +20dB used for now
  if (pw > 17) {
    pw = 17;
  } else if (pw < 2) {
    pw = 2;
  }
  PRINT_DEBUG(1, F("Config power to %i on PA_BOOST"), pw);

  pw -= 2;
  // check board type for output pin
  // output on PA_BOOST for RFM95W
  hal.write_reg(RegPaConfig, (uint8_t)(0x80 | pw));
  // no boost +20dB
  hal.write_reg(RegPaDac, (hal.read_reg(RegPaDac) & 0xF8) | 0x4);

#else
  // output on rfo pin
  // Bit 6-4 Select max output power: Pmax=10.8+0.6*MaxPower [dBm]
  // Bit 0-3 Pout=Pmax-(15-OutputPower) if PaSelect = 0 (RFO pin)

  if (pw > 15) {
    pw = 15;
  } else if (pw < -4) {
    pw = -4;
  }

  PRINT_DEBUG(1, F("Config power to %i on RFO"), pw);

  uint8_t pa = 0;
  if (pw >= 0) {
    pa = 7 << 4;
    pa += pw;
  } else {
    pa = 0 << 4;
    // take 11 instead of 10.8
    pa += 15 - 11 + pw;
  }

  hal.write_reg(RegPaConfig, pa);
  // no boost +20dB
  hal.write_reg(RegPaDac, (hal.read_reg(RegPaDac) & 0xF8) | 0x4);
#endif
#elif CFG_sx1272_radio
  // set PA config (2-17 dBm using PA_BOOST)
  if (pw > 17) {
    pw = 17;
  } else if (pw < 2) {
    pw = 2;
  }
  hal.write_reg(RegPaConfig, (uint8_t)(0x80 | (pw - 2)));
#else
#error Missing CFG_sx1272_radio/CFG_sx1276_radio
#endif /* CFG_sx1272_radio */
}

uint8_t crForLog(rps_t const rps) {
  switch (rps.getCr()) {
  case CodingRate::CR_4_5:
    return 5;
  case CodingRate::CR_4_6:
    return 6;
  case CodingRate::CR_4_7:
    return 7;
  case CodingRate::CR_4_8:
    return 8;
  }
  return 0;
}

uint16_t bwForLog(rps_t const rps) {
  switch (rps.getBw()) {
  case BandWidth::BW125:
    return 125;
  case BandWidth::BW250:
    return 250;
  case BandWidth::BW500:
    return 500;
  default:
    return 0;
  }
}

enum { RXMODE_SINGLE, RXMODE_SCAN };

// start LoRa receiver
void Radio::rxrssi() const {
  // select LoRa modem (from sleep mode)
  opmodeLora();
  // enter standby mode (warm up))
  opmode(OPMODE_STANDBY);
  // don't use MAC settings at startup
  // use fixed settings for rssi scan
  hal.write_reg(LORARegModemConfig1, RXLORA_RXMODE_RSSI_REG_MODEM_CONFIG1);
  hal.write_reg(LORARegModemConfig2, RXLORA_RXMODE_RSSI_REG_MODEM_CONFIG2);
  // set LNA gain
  hal.write_reg(RegLna, LNA_RX_GAIN);
  // clear all radio IRQ flags
  hal.write_reg(LORARegIrqFlags, 0xFF);
  // mask all irq
  hal.write_reg(LORARegIrqFlagsMask, ~0x00);
  // enable antenna switch for RX
  hal.pin_rxtx(0);
  // now instruct the radio to receive
  // continous rx
  opmode(OPMODE_RX);
  PRINT_DEBUG(1, F("RXMODE_RSSI"));
}

// start LoRa receiver
void Radio::rxlora(uint8_t const rxmode, uint32_t const freq, rps_t const rps,
                   uint8_t const rxsyms, OsTime const rxtime) {
  // select LoRa modem (from sleep mode)
  opmodeLora();
  ASSERT((hal.read_reg(RegOpMode) & OPMODE_LORA) != 0);
  // enter standby mode (warm up))
  opmode(OPMODE_STANDBY);
  // don't use MAC settings at startup
  // configure LoRa modem (cfg1, cfg2)
  configLoraModem(rps);
  // configure frequency
  configChannel(freq);

  // set LNA gain
  hal.write_reg(RegLna, LNA_RX_GAIN);
  // set max payload size
  hal.write_reg(LORARegPayloadMaxLength, 64);
#if !defined(DISABLE_INVERT_IQ_ON_RX)
  // use inverted I/Q signal (prevent mote-to-mote communication)
  hal.write_reg(LORARegInvertIQ, hal.read_reg(LORARegInvertIQ) | (1 << 6));
#endif
  // set symbol timeout (for single rx)
  hal.write_reg(LORARegSymbTimeoutLsb, rxsyms);
  // set sync word
  hal.write_reg(LORARegSyncWord, LORA_MAC_PREAMBLE);

  // configure DIO mapping DIO0=RxDone DIO1=RxTout DIO2=NOP
  hal.write_reg(RegDioMapping1,
           MAP_DIO0_LORA_RXDONE | MAP_DIO1_LORA_RXTOUT | MAP_DIO2_LORA_NOP);
  // clear all radio IRQ flags
  hal.write_reg(LORARegIrqFlags, 0xFF);

  // enable antenna switch for RX
  hal.pin_rxtx(0);

  // now instruct the radio to receive
  if (rxmode == RXMODE_SINGLE) {
    // single rx
    // enable required radio IRQs
    hal.write_reg(LORARegIrqFlagsMask,
             (uint8_t) ~(IRQ_LORA_RXDONE_MASK | IRQ_LORA_RXTOUT_MASK));
    hal_waitUntil(rxtime); // busy wait until exact rx time
    opmode(OPMODE_RX_SINGLE);
  } else {
    // continous rx (scan)
    // enable required radio IRQs
    hal.write_reg(LORARegIrqFlagsMask, ~IRQ_LORA_RXDONE_MASK);
    opmode(OPMODE_RX);
  }

  uint8_t const sf = rps.sf + 6; // 1 == SF7
  PRINT_DEBUG(1, F("%s, freq=%" PRIu32 ", SF=%d, BW=%d, CR=4/%d, IH=%d"),
              rxmode == RXMODE_SINGLE
                  ? "RXMODE_SINGLE"
                  : (rxmode == RXMODE_SCAN ? "RXMODE_SCAN" : "UNKNOWN_RX"),
              freq, sf, bwForLog(rps), crForLog(rps), rps.ih);
}

void Radio::init() {
  DisableIRQsGard irqguard;
  hal.init();
  // manually reset radio
#ifdef CFG_sx1276_radio
  hal.pin_rst(0); // drive RST pin low
#else
  hal.pin_rst(1); // drive RST pin high
#endif
  // wait >100us for SX127x to detect reset
  hal_wait(OsDeltaTime::from_ms(1));
  hal.pin_rst(2); // configure RST pin floating!
  // wait 5ms after reset
  hal_wait(OsDeltaTime::from_ms(5));

  // some sanity checks, e.g., read version number
  uint8_t const v = hal.read_reg(RegVersion);
  PRINT_DEBUG(1, F("Chip version : %i"), v);

#ifdef CFG_sx1276_radio
  ASSERT(v == 0x12);
#elif CFG_sx1272_radio
  ASSERT(v == 0x22);
#else
#error Missing CFG_sx1272_radio/CFG_sx1276_radio
#endif

  /* TODO add a parameter
  // Configure max curent
  // limit current to 45mA
  constexpr uint8_t limit = 0;
  hal.write_reg(RegOcp, 0x20 | limit);
  */
  opmode(OPMODE_SLEEP);
}

// get random seed from wideband noise rssi
void Radio::init_random(uint8_t randbuf[16]) {
  DisableIRQsGard irqguard;

  // seed 15-byte randomness via noise rssi
  rxrssi();
  while ((hal.read_reg(RegOpMode) & OPMODE_MASK) != OPMODE_RX)
    ; // continuous rx
  for (uint8_t i = 1; i < 16; i++) {
    for (uint8_t j = 0; j < 8; j++) {
      uint8_t b; // wait for two non-identical subsequent least-significant bits
      while ((b = hal.read_reg(LORARegRssiWideband) & 0x01) ==
             (hal.read_reg(LORARegRssiWideband) & 0x01))
        ;
      randbuf[i] = (randbuf[i] << 1) | b;
    }
  }

  // stop RX
  opmode(OPMODE_SLEEP);
}

uint8_t Radio::rssi() const {
  DisableIRQsGard irqguard;
  uint8_t const r = hal.read_reg(LORARegRssiValue);
  return r;
}

// called by hal ext IRQ handler
// (radio goes to stanby mode after tx/rx operations)
uint8_t Radio::handle_end_rx(uint8_t *const framePtr) {

  uint8_t const flags = hal.read_reg(LORARegIrqFlags);
  PRINT_DEBUG(2, F("irq: flags: 0x%x\n"), flags);

  uint8_t length = 0;
  if (flags & IRQ_LORA_RXDONE_MASK) {
    // read the PDU and inform the MAC that we received something
    // TODO correct SX1272_MC1_IMPLICIT_HEADER_MODE_ON is not for SX1276
    length = (hal.read_reg(LORARegModemConfig1) & SX1272_MC1_IMPLICIT_HEADER_MODE_ON)
                 ? hal.read_reg(LORARegPayloadLength)
                 : hal.read_reg(LORARegRxNbBytes);

    // for security clamp length of data
    length = length < MAX_LEN_FRAME ? length : MAX_LEN_FRAME;

    // set FIFO read address pointer
    hal.write_reg(LORARegFifoAddrPtr, hal.read_reg(LORARegFifoRxCurrentAddr));
    // now read the FIFO
    hal.read_buffer(RegFifo, framePtr, length);

    // read rx quality parameters
    // SNR [dB] * 4
    last_packet_snr_reg = static_cast<int8_t>(hal.read_reg(LORARegPktSnrValue));
    // RSSI [dBm]  - 139
    last_packet_rssi_reg = hal.read_reg(LORARegPktRssiValue);
  } else if (flags & IRQ_LORA_RXTOUT_MASK) {
    // indicate timeout
    PRINT_DEBUG(1, F("RX timeout"));
  }
  // mask all radio IRQs
  hal.write_reg(LORARegIrqFlagsMask, 0xFF);
  // clear radio IRQ flags
  hal.write_reg(LORARegIrqFlags, 0xFF);
  // go from stanby to sleep
  opmode(OPMODE_SLEEP);

  // 0 in case of timeout.
  return length;
}

void Radio::handle_end_tx() const {

  // mask all radio IRQs
  hal.write_reg(LORARegIrqFlagsMask, 0xFF);
  // clear radio IRQ flags
  hal.write_reg(LORARegIrqFlags, 0xFF);
  // go from stanby to sleep
  opmode(OPMODE_SLEEP);
}

void Radio::rst() const {
  DisableIRQsGard irqguard;
  // put radio to sleep
  opmode(OPMODE_SLEEP);
}

void Radio::tx(uint32_t const freq, rps_t const rps, int8_t const txpow,
               uint8_t const *const framePtr, uint8_t const frameLength) {
  DisableIRQsGard irqguard;
  // select LoRa modem (from sleep mode)
  // hal.write_reg(RegOpMode, OPMODE_LORA);
  opmodeLora();
  // enter standby mode (required for FIFO loading))
  opmode(OPMODE_STANDBY);
  // configure LoRa modem (cfg1, cfg2)
  configLoraModem(rps);
  // configure frequency
  configChannel(freq);
  // configure output power
  // set PA ramp-up time 50 uSec
  hal.write_reg(RegPaRamp, (hal.read_reg(RegPaRamp) & 0xF0) | 0x08);
  configPower(txpow);
  // set sync word
  hal.write_reg(LORARegSyncWord, LORA_MAC_PREAMBLE);

  // set the IRQ mapping DIO0=TxDone DIO1=NOP DIO2=NOP
  hal.write_reg(RegDioMapping1,
           MAP_DIO0_LORA_TXDONE | MAP_DIO1_LORA_NOP | MAP_DIO2_LORA_NOP);
  // clear all radio IRQ flags
  hal.write_reg(LORARegIrqFlags, 0xFF);
  // mask all IRQs but TxDone
  hal.write_reg(LORARegIrqFlagsMask, ~IRQ_LORA_TXDONE_MASK);

  // initialize the payload size and address pointers
  hal.write_reg(LORARegFifoTxBaseAddr, 0x00);
  hal.write_reg(LORARegFifoAddrPtr, 0x00);
  hal.write_reg(LORARegPayloadLength, frameLength);

  // download buffer to the radio FIFO
  hal.write_buffer(RegFifo, framePtr, frameLength);

  // enable antenna switch for TX
  hal.pin_rxtx(1);

  // now we actually start the transmission
  opmode(OPMODE_TX);

  uint8_t const sf = rps.sf + 6; // 1 == SF7
  PRINT_DEBUG(
      1, F("TXMODE, freq=%" PRIu32 ", len=%d, SF=%d, BW=%d, CR=4/%d, IH=%d"),
      freq, frameLength, sf, bwForLog(rps), crForLog(rps), rps.ih);
  // the radio will go back to STANDBY mode as soon as the TX is finished
  // the corresponding IRQ will inform us about completion.
}

void Radio::rx(uint32_t const freq, rps_t const rps, uint8_t const rxsyms,
               OsTime const rxtime) {
  DisableIRQsGard irqguard;
  // receive frame now (exactly at rxtime)
  rxlora(RXMODE_SINGLE, freq, rps, rxsyms, rxtime);
  // the radio will go back to STANDBY mode as soon as the RX is finished
  // or timed out, and the corresponding IRQ will inform us about completion.
}


/**
 * Check the IO pin.
 * Return true if the radio has finish it's operation
 */
bool Radio::io_check() const {
  auto const pinInInt = hal.io_check();
  if (pinInInt < NUM_DIO) {
    return true;
  }
  return false;
}

Radio::Radio(lmic_pinmap const &pins) : hal(pins) {}
#endif