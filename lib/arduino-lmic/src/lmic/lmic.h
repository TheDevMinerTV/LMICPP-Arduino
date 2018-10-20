/*******************************************************************************
 * Copyright (c) 2014-2015 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *    IBM Zurich Research Lab - initial API, implementation and documentation
 *******************************************************************************/

//! @file
//! @brief LMIC API

#ifndef _lmic_h_
#define _lmic_h_

#include "../aes/aes.h"
#include "lmicrand.h"
#include "lorabase.h"
#include "oslmic.h"
#include "radio.h"

// LMIC version
#define LMIC_VERSION_MAJOR 1
#define LMIC_VERSION_MINOR 5


enum { MAX_FRAME_LEN = 64 };  //!< Library cap on max frame length
enum { TXCONF_ATTEMPTS = 8 }; //!< Transmit attempts for confirmed frames

enum {
  RETRY_PERIOD_secs = 3
}; // secs - random period for retrying a confirmed send

enum { KEEP_TXPOW = -128 };

// Netid values /  lmic_t.netid
enum { NETID_NONE = (int)~0U, NETID_MASK = (int)0xFFFFFF };
// MAC operation modes (lmic_t.opmode).
enum {
  OP_NONE = 0x0000,
  OP_SCAN = 0x0001,    // radio scan to find a beacon
  OP_TRACK = 0x0002,   // track my networks beacon (netid)
  OP_JOINING = 0x0004, // device joining in progress (blocks other activities)
  OP_TXDATA = 0x0008,  // TX user data (buffered in pendTxData)
  OP_POLL =
      0x0010, // send empty UP frame to ACK confirmed DN/fetch more DN data
  OP_REJOIN = 0x0020,   // occasionally send JOIN REQUEST
  OP_SHUTDOWN = 0x0040, // prevent MAC from doing anything
  OP_TXRXPEND = 0x0080, // TX/RX transaction pending
  OP_RNDTX = 0x0100,    // prevent TX lining up after a beacon
  OP_PINGINI = 0x0200,  // pingable is initialized and scheduling active
  OP_PINGABLE = 0x0400, // we're pingable
  OP_NEXTCHNL = 0x0800, // find a new channel
  OP_LINKDEAD = 0x1000, // link was reported as dead
  OP_TESTMODE = 0x2000, // developer test mode
};
// TX-RX transaction flags - report back to user
enum {
  TXRX_ACK = 0x80,  // confirmed UP frame was acked
  TXRX_NACK = 0x40, // confirmed UP frame was not acked
  TXRX_NOPORT =
      0x20, // set if a frame with a port was RXed, clr if no frame/no port
  TXRX_PORT = 0x10, // set if a frame with a port was RXed,
                    // LMIC.frame[LMIC.dataBeg-1] => port
  TXRX_DNW1 = 0x01, // received in 1st DN slot
  TXRX_DNW2 = 0x02, // received in 2dn DN slot
  TXRX_PING = 0x04
}; // received in a scheduled RX slot
// Event types for event callback
enum _ev_t {
  EV_SCAN_TIMEOUT = 1,
  EV_JOINING,
  EV_JOINED,
  EV_JOIN_FAILED,
  EV_REJOIN_FAILED,
  EV_TXCOMPLETE,
  EV_RESET,
  EV_LINK_DEAD,
  EV_LINK_ALIVE
};
typedef enum _ev_t ev_t;

using eventCallback_t = void (*)(ev_t);
using keyCallback_t = void (*)(uint8_t *);
enum {
  // This value represents 100% error in LMIC.clockError
  MAX_CLOCK_ERROR = 256,
};

#if defined(CFG_eu868) // EU868 spectrum

enum { ADR_ACK_DELAY = 32, ADR_ACK_LIMIT = 64 };

#elif defined(CFG_us915) // US915 spectrum

enum { ADR_ACK_DELAY = 32, ADR_ACK_LIMIT = 64 };

#endif

enum {
  // continue with this after reported dead link
  LINK_CHECK_CONT = 0,
  // after this UP frames and no response from NWK assume link is dead
  LINK_CHECK_DEAD = ADR_ACK_DELAY,
  // UP frame count until we ask for ADRACKReq
  LINK_CHECK_INIT = -ADR_ACK_LIMIT,
  LINK_CHECK_OFF = -128
}; // link check disabled

class Lmic {
public:
  Radio radio;

private:
  OsJobType<Lmic> osjob{*this, OSS};
  // Radio settings TX/RX (also accessed by HAL)

  OsTime rxtime;

  int8_t rssi = 0;
  int8_t snr = 0;
  // radio parameters set
  rps_t rps;
  uint8_t rxsyms = 0;

  eventCallback_t eventCallBack = nullptr;
  keyCallback_t devEuiCallBack = nullptr;
  keyCallback_t artEuiCallBack = nullptr;

protected:
  OsTime txend;
  uint8_t dndr = 0;
  uint32_t freq = 0;
  uint8_t txChnl = 0;         // channel for next TX
  uint8_t globalDutyRate = 0; // max rate: 1/2^k
  OsTime globalDutyAvail;     // time device can send again

  // ADR adjusted TX power, limit power to this value.
  int8_t adrTxPow;
  int8_t txpow = 0; // dBm

  dr_t datarate = 0; // current data rate
private:
  uint32_t netid; // current network id (~0 - none)
  // curent opmode set at init
  uint16_t opmode;
  // configured up repeat for unconfirmed message, reset after join.
  // Not handle properly  cf: LoRaWAN™ Specification §5.2
  uint8_t upRepeat;

  // adjustment for rejoin datarate
  uint8_t rejoinCnt;

  uint8_t clockError = 0; // Inaccuracy in the clock. CLOCK_ERROR_MAX
                          // represents +/-100% error

  // pending data length
  uint8_t pendTxLen = 0;
  // pending data ask for confirmation
  bool pendTxConf;
  // pending data port
  uint8_t pendTxPort;
  // pending data
  uint8_t pendTxData[MAX_LEN_PAYLOAD];

  // last generated nonce
  // set at random value at reset.
  uint16_t devNonce;

  // device address, set at 0 at reset.
  devaddr_t devaddr;
  // device level down stream seqno, reset after join.
  uint32_t seqnoDn;
  // device level up stream seqno, reset after join.
  uint32_t seqnoUp;
  // dn frame confirm pending: LORA::FCT_ACK or 0, reset after join
  uint8_t dnConf;
  // counter until we reset data rate (-128=off), reset after join
  // ask for confirmation if > 0
  // lower data rate if > LINK_CHECK_DEAD
  int8_t adrAckReq;

  // // Rx delay after TX, init at reset
  OsDeltaTime rxDelay;

  uint8_t margin = 0;
  // link adr adapt answer pending, init after join
  // use bit 15 as flag, other as value for acq
  uint8_t ladrAns;
  // device status answer pending, init after join
  bool devsAns;
  // RX timing setup answer pending, init after join
  bool rxTimingSetupAns;
  // adr Mode, init at reset
  uint8_t adrEnabled;
#if !defined(DISABLE_MCMD_DCAP_REQ)
  // have to ACK duty cycle settings, init after join
  bool dutyCapAns;
#endif
#if !defined(DISABLE_MCMD_SNCH_REQ)
  // answer set new channel, init afet join.
  uint8_t snchAns;
#endif
protected:
  // 1 RX window DR offset
  uint8_t rx1DrOffset;

private:
  // 2nd RX window (after up stream), init at reset
  dr_t dn2Dr;
  uint32_t dn2Freq;
#if !defined(DISABLE_MCMD_DN2P_SET)
  // 0=no answer pend, 0x80+ACKs, init after join
  uint8_t dn2Ans;
#endif

public:
  // Public part of MAC state
  uint8_t txCnt = 0;
  uint8_t txrxFlags = 0; // transaction flags (TX-RX combo)
  uint8_t dataBeg = 0;   // 0 or start of data (dataBeg-1 is port)
  uint8_t dataLen = 0;   // 0 no data or zero length data, >0 byte count of data
  uint8_t frame[MAX_LEN_FRAME];

  Aes aes;

protected:
  LmicRand rand;

private:
  // callbacks
  void processRx1DnData();
  void setupRx1();
  void setupRx2();
  void schedRx12(OsDeltaTime const &delay, uint8_t dr);

  void txDone(OsDeltaTime const &delay);

  void runReset();
  void runEngineUpdate();

#if !defined(DISABLE_JOIN)
  void onJoinFailed();
  bool processJoinAcceptNoJoinFrame();
  bool processJoinAccept();
  void processRx1Jacc();
  void processRx2Jacc();
  void setupRx1Jacc();
  void setupRx2Jacc();
  void jreqDone();
  void startJoiningCallBack();

  void buildJoinRequest(uint8_t ftype);

#endif

  void processRx2DnData();

  void setupRx1DnData();
  void setupRx2DnData();

  void updataDone();

  void stateJustJoined();

  void reportEvent(ev_t ev);

  void buildDataFrame();
  void engineUpdate();
  void parseMacCommands(const uint8_t *opts, uint8_t olen);
  bool decodeFrame();
  bool processDnData();

  void txDelay(OsTime const &reftime, uint8_t secSpan);

public:
  void setDrJoin(dr_t dr);
  // set default/start DR/txpow
  void setDrTxpow(uint8_t dr, int8_t pow);
  void setLinkCheckMode(bool enabled);
  void setSession(uint32_t netid, devaddr_t devaddr, uint8_t *nwkSKey,
                  uint8_t *artKey);

  // set ADR mode (if mobile turn off)
  void setAdrMode(bool enabled);

#if !defined(DISABLE_JOIN)
  bool startJoining();
  void tryRejoin();

#endif

  void init();
  void shutdown();
  void reset();

  void clrTxData();
  void setTxData();
  int8_t setTxData2(uint8_t port, uint8_t *data, uint8_t dlen, bool confirmed);
  void sendAlive();
  void setClockError(uint8_t error);

  uint16_t getOpMode() { return opmode; };

  void setEventCallBack(eventCallback_t callback) { eventCallBack = callback; };
  void setDevEuiCallback(keyCallback_t callback) { devEuiCallBack = callback; };
  void setArtEuiCallback(keyCallback_t callback) { artEuiCallBack = callback; };

  // for radio to wakeup processing.
  void irq_handler(uint8_t dio, OsTime const &trigger);

protected:
  virtual uint8_t getRawRps(dr_t dr) const = 0;

  virtual int8_t pow2dBm(uint8_t mcmd_ladr_p1) const = 0;
  virtual OsDeltaTime getDwn2SafetyZone() const = 0;
  virtual OsDeltaTime dr2hsym(dr_t dr) const = 0;
  virtual uint32_t convFreq(const uint8_t *ptr) const = 0;
  virtual bool validRx1DrOffset(uint8_t drOffset) const = 0;

  virtual void initDefaultChannels(bool join) = 0;
  virtual bool setupChannel(uint8_t channel, uint32_t newfreq, uint16_t drmap,
                            int8_t band) = 0;
  virtual void disableChannel(uint8_t channel) = 0;
  virtual void handleCFList(const uint8_t *ptr) = 0;

  virtual uint8_t mapChannels(uint8_t chpage, uint16_t chmap) = 0;
  virtual void updateTx(OsTime const &txbeg, OsDeltaTime const &airtime) = 0;
  virtual OsTime nextTx(OsTime const &now) = 0;
  virtual void setRx1Params() = 0;
#if !defined(DISABLE_JOIN)
  virtual void initJoinLoop() = 0;
  virtual bool nextJoinState() = 0;
#endif
  virtual dr_t defaultRX2Dr() const = 0;
  virtual uint32_t defaultRX2Freq() const = 0;

  rps_t updr2rps(dr_t dr) const;
  rps_t dndr2rps(dr_t dr) const;
  bool isFasterDR(dr_t dr1, dr_t dr2) const;
  bool isSlowerDR(dr_t dr1, dr_t dr2) const;
  // increase data rate
  dr_t incDR(dr_t dr) const;
  // decrease data rate
  dr_t decDR(dr_t dr) const;
  // in range
  bool validDR(dr_t dr) const;
  // decrease data rate by n steps
  dr_t lowerDR(dr_t dr, uint8_t n) const;

public:
  Lmic();
};

//! Construct a bit map of allowed datarates from drlo to drhi (both included).
#define DR_RANGE_MAP(drlo, drhi)                                               \
  (((uint16_t)0xFFFF << static_cast<uint8_t>(drlo)) & ((uint16_t)0xFFFF >> (15 - static_cast<uint8_t>(drhi))))

#endif // _lmic_h_
