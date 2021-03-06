# ESP32 Command Station Feature/Bug Tracking list
This document tracks features and bug fixes that are planned.

## v1.3.0
The primary focus for this will be improving the LCC integration and solve the crash caused by the DCC signal generation code.

### Updates from v1.2.3

#### JMRI Interface

- [x] Replaced WiFiServer code with socket_listener from OpenMRNLite.

#### DCC System

- [x] fix signal generation so it doesn't crash up when spi_flash disables cache.
- [x] adjust motor board powerOn/powerOff to enable/disable DCC signal instead of using global power-on/off.
- [x] OPS RailCom support (not enableable yet).
- [x] remove hardware timer legacy code.

#### LCC Integration

- [x] Integrate the WiFiConfiguration CDI element.

#### Nextion Interface

- [x] fix screen type detection.
- [x] lock to title screen until WiFi connects.
- [x] switch to timer based speed increment/decrement on the nextion mcu side.

#### Web Interface

- [x] Add space in footer for clock so date/time are not smashed together.
- [x] Fixed S88 section hiding.
- [x] Fixed S88 sensor bus creation/edit json payload parameters.
- [x] Reintroduce websocket usage for pushing commands back to the CS (improves performance).
- [x] auto-refresh of status pages
- [x] add busy/wait spinner for when data is loading (or being refreshed) in the web interface.
- [x] Adding SPIFFSEditor for easy access to view/edit configuration data.
- [x] investigate tcp/ip hang (fixed in AsyncTCP 1.1.0 and ESP Async Webserver 1.2.2)

#### S88 Sensors

- [x] Add delay in s88SensorTask so that it gives time for other tasks to execute between updates.
- [x] restore custom web rendering of sensor data.

#### Configuration

- [x] rename of WIFI_SSID to SSID_NAME and WIFI_PASSWORD to SSID_PASSWORD to be in sync with upcoming changes.
- [x] Split up monolithic json files to instead be individual files.

#### General

- [x] Remove usage of log_X macros in favor of LOG.
- [x] Status LED output for WiFi, OPS and PROG.
- [x] Enable WDT to prevent hangs.

## Future planning:
The entries below are not tracked to a specific release or in any particular priority order.

### General (misc)

- [ ] CMake and VisualGDB support (https://github.com/atanisoft/ESP32CommandStation/pull/22)

### S88 Sensors

- [ ] Add S88 sensor data to InfoScreen status line, 16 sensor output rotation.

### DCC System

- [ ] continue sending eStop packet until eStop is cleared.
- [ ] rework DCC Prog Track interface so it supports multiple requests (serialized) and async response to web.
- [ ] refactor signal generation for better RailCom/LCC integration (dcc::Packet, UpdateLoop, RailcomHub, ProgrammingTrackBackend, LocalTrackIf).
- [ ] allow adjustment of the DCC preamble bit count, default is 16 (OPS) and 22 (PROG). The OPS value is constrained between 11 and 20 and PROG between 22 and 50.
- [ ] test and expose OPS RailCom configuration.
- [ ] consider moving to rmt_translator_init

### Config

- [ ] Dynamic WiFi configuration support (multiple SSID, strength based selection of SSID, etc)
- [ ] Dynamic command station feature support (S88, OLED, LCD, LocoNet, Nextion, LCC, etc)
- [ ] SoftAP support for initial config and "non-home" network. (https://github.com/atanisoft/ESP32CommandStation/issues/4)

### Web Interface

- [ ] add dialog for failed CS requests.
- [ ] WiThrottle support (https://github.com/atanisoft/ESP32CommandStation/issues/15)
- [ ] Expose Loco Consist creation.
- [ ] Add strict validation of input parameter data.
- [ ] Rework web prog req to be async rather than blocking (can cause WDT failure with retries 5+)

### LCC Integration

- [ ] Traction proxy impl.
- [ ] Broadcast events for turnout state change.
- [ ] Discard turnout events when turnout already in expected state (drop duplicate events).
- [ ] adjust InfoScreen LCC details so they are actually useful, right now it is a placeholder.
- [ ] implement CV memory space.
- [ ] migrate to Esp32WiFiManager instead of Arduino WiFi library.

### Nextion Interface

- [ ] auto turn on of track power from Nextion when interacting with loco/turnouts.
- [ ] add notification of turnout state change when changed external to the nextion code.
- [ ] replace Routes page with a Setup page which will include route creation.
- [ ] implement automatic resolver for component id during page initialization so we can drop component IDs from the argument list.
- [ ] move screen detection code into NeoNextion lib.
- [ ] add support for Nextion Upload via OTA in NeoNextion.
- [ ] correct screen size detection for 5" and 7" displays which send the same screen type prefix.

### OTA

- [ ] OTA support via JMRI / LCC.
- [ ] return to normal mode on Nextion when OTA fails.

### InfoScreen

- [ ] Investigate multi-thread aware I2C library if/when available.

### Documentation
No tasks have been added yet.
