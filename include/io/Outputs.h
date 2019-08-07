/**********************************************************************
ESP32 COMMAND STATION

COPYRIGHT (c) 2017-2019 Mike Dunston

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

#ifndef OUTPUTS_H_
#define OUTPUTS_H_

#include "interfaces/DCCppProtocol.h"

const uint8_t OUTPUT_IFLAG_INVERT = BIT0;
const uint8_t OUTPUT_IFLAG_RESTORE_STATE = BIT1;
const uint8_t OUTPUT_IFLAG_FORCE_STATE = BIT2;

class Output
{
public:
  Output(uint16_t, uint8_t, uint8_t);
  Output(std::string &);
  void set(bool=false, bool=true);
  void update(uint8_t, uint8_t);
  std::string toJson(bool=false);
  uint16_t getID()
  {
    return _id;
  }
  uint8_t getPin()
  {
    return _pin;
  }
  uint8_t getFlags()
  {
    return _flags;
  }
  bool isActive()
  {
    return _active;
  }
  void showStatus();
  std::string getFlagsAsString()
  {
    std::string flags = "";
    if((_flags & OUTPUT_IFLAG_INVERT) == OUTPUT_IFLAG_INVERT)
    {
      flags += "activeLow";
    }
    else
    {
      flags += "activeHigh";
    }
    if((_flags & OUTPUT_IFLAG_RESTORE_STATE) == OUTPUT_IFLAG_RESTORE_STATE)
    {
      if((_flags & OUTPUT_IFLAG_FORCE_STATE) == OUTPUT_IFLAG_FORCE_STATE)
      {
        flags += ",force(on)";
      }
      else
      {
        flags += ",force(off)";
      }
    }
    else
    {
      flags += ",restoreState";
    }
    return flags;
  }
private:
  uint16_t _id;
  uint8_t _pin;
  uint8_t _flags;
  bool _active;
};

class OutputManager
{
  public:
    static void init();
    static void clear();
    static uint16_t store();
    static bool set(uint16_t, bool=false);
    static Output *getOutput(uint16_t);
    static bool toggle(uint16_t);
    static std::string getStateAsJson();
    static void showStatus();
    static bool createOrUpdate(const uint16_t, const uint8_t, const uint8_t);
    static bool remove(const uint16_t);
};

DECLARE_DCC_PROTOCOL_COMMAND_CLASS(OutputCommandAdapter, "Z")

DECLARE_DCC_PROTOCOL_COMMAND_CLASS(OutputExCommandAdapter, "Zex")

#endif // OUTPUTS_H_