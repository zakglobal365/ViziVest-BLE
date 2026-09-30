#include "wled.h"
#include "NimBLEDevice.h"

#define VIZIVEST_SERVICE_UUID        "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define VIZIVEST_COMMAND_UUID        "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define VIZIVEST_STATUS_UUID         "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

class ViziVestBLE : public Usermod
{
private:

  NimBLEServer* bleServer = nullptr;
  NimBLECharacteristic* commandCharacteristic = nullptr;
  NimBLECharacteristic* statusCharacteristic = nullptr;

  bool bleStarted = false;

  // WLED preset numbers.
  // We will configure these after the BLE system is working.
  uint8_t glowPreset   = 1;
  uint8_t leftPreset   = 2;
  uint8_t rightPreset  = 3;
  uint8_t hazardPreset = 4;

  static const char _name[];

  void sendStatus(const char* message)
  {
    if (!statusCharacteristic) return;

    statusCharacteristic->setValue(message);
    statusCharacteristic->notify();
  }

  void runPreset(uint8_t preset)
  {
    if (preset == 0) return;

    applyPreset(preset, CALL_MODE_DIRECT_CHANGE);
  }

  void processCommand(String command)
  {
    command.trim();
    command.toUpperCase();

    DEBUG_PRINT(F("ViziVest BLE command: "));
    DEBUG_PRINTLN(command);

    if (command == "GLOW")
    {
      runPreset(glowPreset);
      sendStatus("GLOW");
    }
    else if (command == "LEFT")
    {
      runPreset(leftPreset);
      sendStatus("LEFT");
    }
    else if (command == "RIGHT")
    {
      runPreset(rightPreset);
      sendStatus("RIGHT");
    }
    else if (command == "HAZARD")
    {
      runPreset(hazardPreset);
      sendStatus("HAZARD");
    }
    else if (command == "OFF")
    {
      bri = 0;
      stateUpdated(CALL_MODE_DIRECT_CHANGE);
      sendStatus("OFF");
    }
    else if (command == "PING")
    {
      sendStatus("VIZIVEST_OK");
    }
    else
    {
      sendStatus("UNKNOWN_COMMAND");
    }
  }

  class CommandCallbacks : public NimBLECharacteristicCallbacks
  {
  public:

    ViziVestBLE* parent;

    CommandCallbacks(ViziVestBLE* p)
    {
      parent = p;
    }

    void onWrite(
      NimBLECharacteristic* characteristic,
      NimBLEConnInfo& connInfo
    ) override
    {
      std::string value = characteristic->getValue();

      if (value.length() == 0) return;

      String command = String(value.c_str());

      parent->processCommand(command);
    }
  };

  CommandCallbacks* commandCallbacks = nullptr;

public:

  void setup() override
  {
    if (bleStarted) return;

    DEBUG_PRINTLN(F("Starting ViziVest BLE..."));

    NimBLEDevice::init("VIZIVEST");

    bleServer = NimBLEDevice::createServer();

    NimBLEService* service =
      bleServer->createService(VIZIVEST_SERVICE_UUID);

    commandCharacteristic =
      service->createCharacteristic(
        VIZIVEST_COMMAND_UUID,
        NIMBLE_PROPERTY::WRITE |
        NIMBLE_PROPERTY::WRITE_NR
      );

    statusCharacteristic =
      service->createCharacteristic(
        VIZIVEST_STATUS_UUID,
        NIMBLE_PROPERTY::READ |
        NIMBLE_PROPERTY::NOTIFY
      );

    commandCallbacks = new CommandCallbacks(this);

    commandCharacteristic->setCallbacks(commandCallbacks);

    statusCharacteristic->setValue("VIZIVEST_READY");

    service->start();

    NimBLEAdvertising* advertising =
      NimBLEDevice::getAdvertising();

    advertising->addServiceUUID(VIZIVEST_SERVICE_UUID);
    advertising->setName("VIZIVEST");
    advertising->start();

    bleStarted = true;

    DEBUG_PRINTLN(F("ViziVest BLE is ready."));
  }

  void addToConfig(JsonObject& root) override
  {
    JsonObject config =
      root.createNestedObject(F("ViziVest BLE"));

    config[F("Glow Preset")] = glowPreset;
    config[F("Left Preset")] = leftPreset;
    config[F("Right Preset")] = rightPreset;
    config[F("Hazard Preset")] = hazardPreset;
  }

  bool readFromConfig(JsonObject& root) override
  {
    JsonObject config =
      root[F("ViziVest BLE")];

    if (config.isNull()) return false;

    glowPreset =
      config[F("Glow Preset")] | glowPreset;

    leftPreset =
      config[F("Left Preset")] | leftPreset;

    rightPreset =
      config[F("Right Preset")] | rightPreset;

    hazardPreset =
      config[F("Hazard Preset")] | hazardPreset;

    return true;
  }

  void addToJsonInfo(JsonObject& root) override
  {
    JsonObject info =
      root[F("ViziVest BLE")];

    info[F("BLE")] =
      bleStarted ? "VIZIVEST" : "OFF";
  }

  uint16_t getId() override
  {
    return 0x5642;
  }
};

const char ViziVestBLE::_name[] PROGMEM = "ViziVest BLE";

static ViziVestBLE viziVestBLE;

REGISTER_USERMOD(viziVestBLE);
