/**********************************************************************
ESP32 COMMAND STATION

COPYRIGHT (c) 2019 Mike Dunston

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see http://www.gnu.org/licenses
**********************************************************************/

#include "ESP32CommandStation.h"
#if 0
#include "DCCSignalGenerator.h"

// number of samples to take when monitoring current after a CV verify
// (bit or byte) has been sent
static constexpr DRAM_ATTR uint8_t CVSampleCount = 150;

// number of attempts the programming track will make to read/write a CV
static constexpr uint8_t PROG_TRACK_CV_ATTEMPTS = 3;

// flag for when programming track is actively being used
bool progTrackBusy = false;

bool enterProgrammingMode() {
  const uint16_t milliAmpStartupLimit = (4096 * 100 / get_hbridge_max_amps(PROG_HBRIDGE_NAME));

  // check if the programming track is already in use
  if(progTrackBusy) {
    return false;
  }

  // flag that we are currently using the programming track
  progTrackBusy = true;

  // energize the programming track
  enable_named_hbridge(PROG_HBRIDGE_NAME);
  dccSignal[DCC_SIGNAL_PROGRAMMING]->waitForQueueEmpty();
  // give decoder time to start up and stabilize to under 100mA draw
  LOG(VERBOSE, "[PROG] waiting for power draw to stabilize");
  vTaskDelay(pdMS_TO_TICKS(100));

  // check that the current is under 100mA limit, this will take ~50ms
  if(get_hbridge_sample(PROG_HBRIDGE_NAME) > milliAmpStartupLimit) {
    LOG_ERROR("[PROG] current draw is over 100mA, aborting");
    statusLED->setStatusLED(StatusLED::LED::PROG_TRACK, StatusLED::COLOR::RED);
    leaveProgrammingMode();
    return false;
  }

  // delay for a short bit before entering programming mode
  vTaskDelay(pdMS_TO_TICKS(40));

  return true;
}

void leaveProgrammingMode() {
  if(!progTrackBusy) {
    return;
  }

  // deenergize the programming track
  disable_named_hbridge(PROG_HBRIDGE_NAME);

  // reset flag to indicate the programming track is free
  progTrackBusy = false;
}

int16_t readCV(const uint16_t cv) {
  const uint16_t milliAmpAck = (4096 * 60 / get_hbridge_max_amps(PROG_HBRIDGE_NAME));
  uint8_t readCVBitPacket[4] = { (uint8_t)(0x78 + (highByte(cv - 1) & 0x03)), lowByte(cv - 1), 0x00, 0x00};
  uint8_t verifyCVPacket[4] = { (uint8_t)(0x74 + (highByte(cv - 1) & 0x03)), lowByte(cv - 1), 0x00, 0x00};
  int16_t cvValue = -1;
  auto& signalGenerator = dccSignal[DCC_SIGNAL_PROGRAMMING];

  for(int attempt = 0; attempt < PROG_TRACK_CV_ATTEMPTS && cvValue == -1; attempt++) {
    LOG(INFO, "[PROG %d/%d] Attempting to read CV %d", attempt+1, PROG_TRACK_CV_ATTEMPTS, cv);
    if(attempt) {
      LOG(VERBOSE, "[PROG] Resetting DCC Decoder");
      signalGenerator->loadBytePacket(resetPacket, 2, 25);
      signalGenerator->waitForQueueEmpty();
    }

    // reset cvValue to all bits OFF
    cvValue = 0;
    for(uint8_t bit = 0; bit < 8; bit++) {
      LOG(VERBOSE, "[PROG] CV %d, bit [%d/7]", cv, bit);
      readCVBitPacket[2] = 0xE8 + bit;
      signalGenerator->loadBytePacket(resetPacket, 2, 3);
      signalGenerator->loadBytePacket(readCVBitPacket, 3, 5);
      signalGenerator->waitForQueueEmpty();
      if(get_hbridge_sample(PROG_HBRIDGE_NAME) > milliAmpAck) {
        LOG(VERBOSE, "[PROG] CV %d, bit [%d/7] ON", cv, bit);
        bitWrite(cvValue, bit, 1);
      } else {
        LOG(VERBOSE, "[PROG] CV %d, bit [%d/7] OFF", cv, bit);
      }
    }

    // verify the byte we received
    verifyCVPacket[2] = cvValue & 0xFF;
    LOG(INFO, "[PROG %d/%d] Attempting to verify read of CV %d as %d", attempt+1, PROG_TRACK_CV_ATTEMPTS, cv, cvValue);
    signalGenerator->loadBytePacket(resetPacket, 2, 3);
    signalGenerator->loadBytePacket(verifyCVPacket, 3, 5);
    signalGenerator->waitForQueueEmpty();
    if(get_hbridge_sample(PROG_HBRIDGE_NAME) > milliAmpAck) {
      LOG(INFO, "[PROG] CV %d, verified as %d", cv, cvValue);
    } else {
      LOG(WARNING, "[PROG] CV %d, could not be verified", cv);
      cvValue = -1;
    }
  }
  LOG(INFO, "[PROG] CV %d value is %d", cv, cvValue);
  return cvValue;
}

bool writeProgCVByte(const uint16_t cv, const uint8_t cvValue) {
  const uint16_t milliAmpAck = (4096 * 60 / get_hbridge_max_amps(PROG_HBRIDGE_NAME));
  uint8_t writeCVBytePacket[4] = { (uint8_t)(0x7C + (highByte(cv - 1) & 0x03)), lowByte(cv - 1), cvValue, 0x00};
  uint8_t verifyCVBytePacket[4] = { (uint8_t)(0x74 + (highByte(cv - 1) & 0x03)), lowByte(cv - 1), cvValue, 0x00};
  bool writeVerified = false;
  auto& signalGenerator = dccSignal[DCC_SIGNAL_PROGRAMMING];

  for(uint8_t attempt = 1; attempt <= PROG_TRACK_CV_ATTEMPTS && !writeVerified; attempt++) {
    LOG(INFO, "[PROG %d/%d] Attempting to write CV %d as %d", attempt, PROG_TRACK_CV_ATTEMPTS, cv, cvValue);
    if(attempt) {
      LOG(VERBOSE, "[PROG] Resetting DCC Decoder");
      signalGenerator->loadBytePacket(resetPacket, 2, 25);
    }
    signalGenerator->loadBytePacket(resetPacket, 2, 3);
    signalGenerator->loadBytePacket(writeCVBytePacket, 3, 4);
    signalGenerator->waitForQueueEmpty();

    // verify that the decoder received the write byte packet and sent an ACK
    if(get_hbridge_sample(PROG_HBRIDGE_NAME) > milliAmpAck) {
      signalGenerator->loadBytePacket(verifyCVBytePacket, 3, 5);
      signalGenerator->waitForQueueEmpty();
      // check that decoder sends an ACK for the verify operation
      if(get_hbridge_sample(PROG_HBRIDGE_NAME)> milliAmpAck) {
        writeVerified = true;
        LOG(INFO, "[PROG] CV %d write value %d verified.", cv, cvValue);
      }
    } else {
      LOG(WARNING, "[PROG] CV %d write value %d could not be verified.", cv, cvValue);
    }
  }
  return writeVerified;
}

bool writeProgCVBit(const uint16_t cv, const uint8_t bit, const bool value) {
  const uint16_t milliAmpAck = (4096 * 60 / get_hbridge_max_amps(PROG_HBRIDGE_NAME));
  uint8_t writeCVBitPacket[4] = { (uint8_t)(0x78 + (highByte(cv - 1) & 0x03)), lowByte(cv - 1), (uint8_t)(0xF0 + bit + value * 8), 0x00};
  uint8_t verifyCVBitPacket[4] = { (uint8_t)(0x74 + (highByte(cv - 1) & 0x03)), lowByte(cv - 1), (uint8_t)(0xB0 + bit + value * 8), 0x00};
  bool writeVerified = false;
  auto& signalGenerator = dccSignal[DCC_SIGNAL_PROGRAMMING];

  for(uint8_t attempt = 1; attempt <= PROG_TRACK_CV_ATTEMPTS && !writeVerified; attempt++) {
    LOG(INFO, "[PROG %d/%d] Attempting to write CV %d bit %d as %d", attempt, PROG_TRACK_CV_ATTEMPTS, cv, bit, value);
    if(attempt) {
      LOG(VERBOSE, "[PROG] Resetting DCC Decoder");
      signalGenerator->loadBytePacket(resetPacket, 2, 3);
    }
    signalGenerator->loadBytePacket(writeCVBitPacket, 3, 4);
    signalGenerator->waitForQueueEmpty();

    // verify that the decoder received the write byte packet and sent an ACK
    if(get_hbridge_sample(PROG_HBRIDGE_NAME) > milliAmpAck) {
      signalGenerator->loadBytePacket(resetPacket, 2, 3);
      signalGenerator->loadBytePacket(verifyCVBitPacket, 3, 5);
      signalGenerator->waitForQueueEmpty();
      // check that decoder sends an ACK for the verify operation
      if(get_hbridge_sample(PROG_HBRIDGE_NAME) > milliAmpAck) {
        writeVerified = true;
        LOG(INFO, "[PROG %d/%d] CV %d write bit %d verified.", attempt, PROG_TRACK_CV_ATTEMPTS, cv, bit);
      }
    } else {
      LOG(WARNING, "[PROG %d/%d] CV %d write bit %d could not be verified.", attempt, PROG_TRACK_CV_ATTEMPTS, cv, bit);
    }
  }
  return writeVerified;
}
#endif

void writeOpsCVByte(const uint16_t locoAddress, const uint16_t cv, const uint8_t cvValue) {
  LOG(VERBOSE, "[OPS] Updating CV %d to %d for loco %d", cv, cvValue, locoAddress);
  auto *b = trackInterface->alloc();
  b->data()->start_dcc_packet();
  if(locoAddress > 127)
  {
    b->data()->add_dcc_address(dcc::DccLongAddress(locoAddress));
  }
  else
  {
    b->data()->add_dcc_address(dcc::DccShortAddress(locoAddress));
  }
  b->data()->add_dcc_pom_write1(cv, cvValue);
  b->data()->packet_header.rept_count = 3;
  trackInterface->send(b);
}

void writeOpsCVBit(const uint16_t locoAddress, const uint16_t cv, const uint8_t bit, const bool value) {
  LOG(VERBOSE, "[OPS] Updating CV %d bit %d to %d for loco %d", cv, bit, value, locoAddress);
  auto *b = trackInterface->alloc();
  b->data()->start_dcc_packet();
  if(locoAddress > 127)
  {
    b->data()->add_dcc_address(dcc::DccLongAddress(locoAddress));
  }
  else
  {
    b->data()->add_dcc_address(dcc::DccShortAddress(locoAddress));
  }
  // TODO add_dcc_pom_write_bit(cv, bit, value)
  b->data()->add_dcc_prog_command(0xe8, cv - 1, (uint8_t)(0xF0 + bit + value * 8));
  b->data()->packet_header.rept_count = 3;
  trackInterface->send(b);
}
