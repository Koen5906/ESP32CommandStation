/**********************************************************************
ESP32 COMMAND STATION

COPYRIGHT (c) 2018-2019 Mike Dunston

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

#ifndef CONFIG_MGR_H_
#define CONFIG_MGR_H_

#include <openlcb/Defs.hxx>
#include "cdi/CSConfigDescriptor.h"
#include "stateflows/FreeRTOSTaskMonitor.h"
#include "stateflows/HC12Radio.h"
#include "stateflows/InfoScreen.h"
#include "stateflows/OTAMonitor.h"
#include "stateflows/StatusLED.h"

// Class definition for the Configuration Management system in ESP32 Command Station
class ConfigurationManager
{
public:
  ConfigurationManager();
  virtual ~ConfigurationManager();
  void init();
  void clear();

  bool exists(const std::string &);
  void remove(const std::string &);
  std::string load(const std::string &);
  void store(const char *, const std::string &);
  void factory_reset_lcc();
  openlcb::NodeID getNodeId();
  void setNodeID(std::string);
  void configureLCC(OpenMRN *, const esp32cs::Esp32ConfigDef &);
  void configureEnabledModules(openlcb::SimpleCanStack *);
  std::string getCSConfig();
  std::string getCSFeatures();
  std::string getSSID()
  {
    return wifiSSID_;
  }
  bool isAPEnabled()
  {
    return wifiMode_ == WIFI_MODE_AP || wifiMode_ == WIFI_MODE_APSTA;
  }
  bool isStationEnabled()
  {
    return wifiMode_ == WIFI_MODE_APSTA || wifiMode_ == WIFI_MODE_APSTA;
  }
private:
  std::string getFilePath(const std::string &, bool=false);
  bool validateWiFiConfig();
  bool validateLCCConfig();
  void parseWiFiConfig();

  int configFd_{-1};

  std::string wifiSSID_{SSID_NAME};
  std::string wifiPassword_{SSID_PASSWORD};
  wifi_mode_t wifiMode_{WIFI_MODE_STA};
  std::unique_ptr<tcpip_adapter_ip_info_t> stationStaticIP_{nullptr};
  ip_addr_t stationDNSServer_{ip_addr_any};

  uninitialized<esp32cs::HC12Radio> hc12_;
  uninitialized<OTAMonitorFlow> ota_;
  uninitialized<InfoScreen> infoScreen_;
  uninitialized<StatusLED> statusLED_;
  uninitialized<FreeRTOSTaskMonitor> taskMon_;
  std::unique_ptr<AutoSyncFileFlow> configAutoSync_;
};

extern std::unique_ptr<ConfigurationManager> configStore;
extern std::unique_ptr<Esp32WiFiManager> wifiManager;

#endif // CONFIG_MGR_H_