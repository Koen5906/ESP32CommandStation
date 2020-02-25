/**********************************************************************
ESP32 COMMAND STATION

COPYRIGHT (c) 2017-2020 Mike Dunston

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

/**********************************************************************

The ESP32 Command Station supports Sensor inputs that can be connected to any
unused ESP32 pin. Sensors can be of any type (infrared, magentic, mechanical...).
The only requirement is that when "activated" the Sensor must force the
specified pin LOW (i.e. to ground), and when not activated, this pin should
remain HIGH (e.g. 3.3V), or be allowed to float HIGH if use of the pin's
internal pull-up resistor is specified.  In addition to this type of sensor
the command station also supports S88-n connected sensors.

To ensure proper voltage levels, some part of the Sensor circuitry
MUST be tied back to the same ground as used by the ESP32.

To have the command station monitor one or more GPIO pins for sensor triggers, first
define/edit/delete sensor definitions using the following variation of the "S"
command:

  <S ID PIN PULLUP>:    creates a new sensor ID, with specified PIN and PULLUP
                        if sensor ID already exists, it is updated with
                        specificed PIN and PULLUP.
        returns: <O> if successful and <X> if unsuccessful (e.g. out of memory)

  <S ID>:               deletes definition of sensor ID.
        returns: <O> if successful and <X> if unsuccessful (e.g. ID does not exist)

  <S>:                  lists all defined sensors.
        returns: <Q ID PIN PULLUP> for each defined sensor or <X> if no sensors
        defined

where

  ID:     the numeric ID (0-32767) of the sensor
  PIN:    the pin number the sensor is connected to
  PULLUP: 1=use internal pull-up resistor for PIN, 0=don't use internal pull-up
          resistor for PIN

Once all sensors have been properly defined, use the <E> command to store their
definitions to the ESP32. If you later make edits/additions/deletions to the
sensor definitions, you must invoke the <E> command if you want those new
definitions updated on the ESP32. You can also clear everything stored on the
ESP32 by invoking the <e> command.

All sensors defined as per above are repeatedly and sequentially checked within
the main loop of this sketch. If a Sensor Pin is found to have transitioned from
one state to another, one of the following serial messages are generated:

  <Q ID>     - for transition of Sensor ID from HIGH state to LOW state
               (i.e. the sensor is triggered)
  <q ID>     - for transition of Sensor ID from LOW state to HIGH state
               (i.e. the sensor is no longer triggered)

Depending on whether the physical sensor is acting as an "event-trigger" or a
"detection-sensor," you may decide to ignore the <q ID> return and only react to
<Q ID> triggers.

**********************************************************************/

#if CONFIG_ENABLE_SENSORS
vector<unique_ptr<Sensor>> sensors;

#include <json.hpp>

TaskHandle_t SensorManager::_taskHandle;
OSMutex SensorManager::_lock;
static constexpr UBaseType_t SENSOR_TASK_PRIORITY = 1;
static constexpr uint32_t SENSOR_TASK_STACK_SIZE = 2048;

static constexpr const char * SENSORS_JSON_FILE = "sensors.json";

void SensorManager::init()
{
  LOG(INFO, "[Sensors] Initializing sensors");
  nlohmann::json root = nlohmann::json::parse(Singleton<ConfigurationManager>::instance()->load(SENSORS_JSON_FILE));
  if(root.contains(JSON_COUNT_NODE))
  {
    uint16_t sensorCount = root[JSON_COUNT_NODE].get<uint16_t>();
    Singleton<StatusDisplay>::instance()->status("Found %02d Sensors", sensorCount);
    for(auto sensor : root[JSON_SENSORS_NODE])
    {
      string data = sensor.dump();
      sensors.push_back(std::make_unique<Sensor>(data));
    }
  }
  LOG(INFO, "[Sensors] Loaded %d sensors", sensors.size());
  xTaskCreate(sensorTask, "SensorManager", SENSOR_TASK_STACK_SIZE, NULL, SENSOR_TASK_PRIORITY, &_taskHandle);
}

void SensorManager::clear()
{
  sensors.clear();
}

uint16_t SensorManager::store()
{
  nlohmann::json root;
  uint16_t sensorStoredCount = 0;
  for (const auto& sensor : sensors)
  {
    if(sensor->getPin() != NON_STORED_SENSOR_PIN)
    {
      root[JSON_SENSORS_NODE].push_back(sensor->toJson());
      sensorStoredCount++;
    }
  }
  root[JSON_COUNT_NODE] = sensorStoredCount;
  Singleton<ConfigurationManager>::instance()->store(SENSORS_JSON_FILE, root.dump());
  return sensorStoredCount;
}

void SensorManager::sensorTask(void *param)
{
  while(true)
  {
    {
      OSMutexLock l(&_lock);
      for (const auto& sensor : sensors)
      {
        sensor->check();
      }
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

string SensorManager::getStateAsJson()
{
  string status;
  for (const auto& sensor : sensors)
  {
    status += sensor->toJson(true);
  }
  return status;
}

Sensor *SensorManager::getSensor(uint16_t id)
{
  OSMutexLock l(&_lock);
  const auto & ent = std::find_if(sensors.begin(), sensors.end(),
  [id](unique_ptr<Sensor> & sensor) -> bool
  {
    return sensor->getID() == id;
  });
  if (ent != sensors.end())
  {
    return ent->get();
  }
  return nullptr;
}

bool SensorManager::createOrUpdate(const uint16_t id, const uint8_t pin, const bool pullUp)
{
  if(is_restricted_pin(pin))
  {
    return false;
  }
  auto sens = getSensor(id);
  if (sens)
  {
    OSMutexLock l(&_lock);
    sens->update(pin, pullUp);
    return true;
  }
  // add the new sensor
  {
    OSMutexLock l(&_lock);
    sensors.push_back(std::make_unique<Sensor>(id, pin, pullUp));
  }
  return true;
}

bool SensorManager::remove(const uint16_t id)
{
  OSMutexLock l(&_lock);
  const auto & ent = std::find_if(sensors.begin(), sensors.end(),
  [id](unique_ptr<Sensor> & sensor) -> bool
  {
    return sensor->getID() == id;
  });
  if (ent != sensors.end())
  {
    LOG(INFO, "[Sensors] Removing Sensor(%d)", (*ent)->getID());
    sensors.erase(ent);
    return true;
  }
  return false;
}

int8_t SensorManager::getSensorPin(const uint16_t id)
{
  auto sens = getSensor(id);
  if (sens)
  {
    return sens->getPin();
  }
  return -1;
}

string SensorManager::get_state_for_dccpp()
{
  string res;
  for (const auto &sensor : sensors)
  {
    res += sensor->get_state_for_dccpp();
  }
  return res;
}

Sensor::Sensor(uint16_t sensorID, int8_t pin, bool pullUp, bool announce) : _sensorID(sensorID), _pin(pin), _pullUp(pullUp), _lastState(false)
{
  if(announce)
  {
    LOG(VERBOSE, "[Sensors] Sensor(%d) on pin %d created, pullup %s", _sensorID, _pin, _pullUp ? "Enabled" : "Disabled");
    gpio_pad_select_gpio((gpio_num_t)_pin);
    ESP_ERROR_CHECK(gpio_set_direction((gpio_num_t)_pin, GPIO_MODE_INPUT));
    if (pullUp)
    {
      ESP_ERROR_CHECK(gpio_pullup_en((gpio_num_t)_pin));
    }
  }
}

Sensor::Sensor(string &data) : _lastState(false)
{
  nlohmann::json object = nlohmann::json::parse(data);
  _sensorID = object[JSON_ID_NODE];
  _pin = object[JSON_PIN_NODE];
  _pullUp = object[JSON_PULLUP_NODE];
  LOG(VERBOSE, "[Sensors] Sensor(%d) on pin %d loaded, pullup %s", _sensorID, _pin, _pullUp ? "Enabled" : "Disabled");
  gpio_pad_select_gpio((gpio_num_t)_pin);
  ESP_ERROR_CHECK(gpio_set_direction((gpio_num_t)_pin, GPIO_MODE_INPUT));
  if (_pullUp)
  {
    ESP_ERROR_CHECK(gpio_pullup_en((gpio_num_t)_pin));
  }
}

string Sensor::toJson(bool includeState)
{
  nlohmann::json object =
  {
    { JSON_ID_NODE, _sensorID },
    { JSON_PIN_NODE, _pin },
    { JSON_PULLUP_NODE, _pullUp },
  };
  if(includeState)
  {
    object[JSON_STATE_NODE] = _lastState;
  }
  return object.dump();
}

void Sensor::update(uint8_t pin, bool pullUp)
{
  _pin = pin;
  _pullUp = pullUp;
  LOG(VERBOSE, "[Sensors] Sensor(%d) on pin %d updated, pullup %s", _sensorID, _pin, _pullUp ? "Enabled" : "Disabled");
  gpio_pad_select_gpio((gpio_num_t)_pin);
  ESP_ERROR_CHECK(gpio_set_direction((gpio_num_t)_pin, GPIO_MODE_INPUT));
  if (_pullUp)
  {
    ESP_ERROR_CHECK(gpio_pullup_en((gpio_num_t)_pin));
  }
}

void Sensor::check()
{
  set(gpio_get_level((gpio_num_t)_pin) == 1);
}

string Sensor::get_state_for_dccpp()
{
  return StringPrintf("<Q %d %d %d>", _sensorID, _pin, _pullUp);
}

string Sensor::set(bool state)
{
  if(_lastState != state)
  {
    _lastState = state;
    LOG(INFO, "Sensor: %d :: %s", _sensorID, _lastState ? "ACTIVE" : "INACTIVE");
    return StringPrintf("<%c %d>", state ? 'Q' : 'q', _sensorID);
  }
  return COMMAND_NO_RESPONSE;
}

#endif // CONFIG_ENABLE_SENSORS