#include "Particle.h"

#include "LIS3DH.h"
#include "AssetTracker.h"
#include "math.h"

// Using SEMI_AUTOMATIC mode to get the lowest possible data usage by
// registering functions and variables BEFORE connecting to the cloud.
SYSTEM_MODE(SEMI_AUTOMATIC);

const int BUILD = 1;

// Various timing constants
const unsigned long MAX_TIME_TO_PUBLISH_MS = 60000; // Only stay awake for 60 seconds trying to connect to the cloud and publish
const unsigned long TIME_AFTER_PUBLISH_MS = 4000; // After publish, wait 4 seconds for data to go out
const unsigned long TIME_AFTER_BOOT_MS = 5000; // At boot, wait 5 seconds before going to sleep again (after coming online)
const unsigned long TIME_PUBLISH_BATTERY_SEC = 22 * 60; // every 22 minutes send a battery update to keep the cellular connection up

const uint8_t movementThreshold = 25;

const int addrConfig = 10;

struct Config{
  // Needed to determine whether config is blank or not
  int version;
  // Set whether you want the device to publish data to the internet by default here.
  // 1 will Particle.publish AND Serial.print, 0 will just Serial.print
  // Extremely useful for saving data while developing close enough to have a cable plugged in.
  // You can also change this remotely using the Particle.function "tmode" defined in setup()
  int tMode;
  // Used to keep track of the last time we published data
  long lastPublish;
  // Manually control GPS on/off
  int tracking;
  // How many times to transmit coordinates over cellular in 1 hour
  int txRate;
  // Last known state (Disarm:0, Arm:1)
  int state;
  // Determines how long the device will sleep(seconds) before being woken to check in status
  int wakeDelay;
};

// Global objects
FuelGauge batteryMonitor;
LIS3DHSPI accel(SPI, A2, WKP);
AssetTracker tracker = AssetTracker();
Config config;

int lastGpsPublish = 0;
int awake = 0;
boolean arming = false;

void setup() {
  Serial.begin(9600);
  initEEPROM();
  Serial.println("Connection Established");
  Serial.println("Special Delivery Build: " + BUILD);

  //TODO Add function for printing Config params
  Particle.function("arm", arm);
  Particle.function("disarm", disarm);
  Particle.function("setTMode", setTransmitMode);
  Particle.function("getLocation", getLocation);
  Particle.function("setTracking", setTracking);
  Particle.function("setTxRate", setTxRate);
  Particle.function("ping", ping);
  Particle.function("reboot", reboot);
  Particle.function("clear_config", clearConfig);

  Particle.function("isGPSFixed", isGPSFixed);
  Particle.connect();

  // Checks if woken up from sleep
  awake = ((accel.clearInterrupt() & LIS3DH::INT1_SRC_IA) != 0);
  if(awake){
    setTracking("1");
    disarm("");
    EEPROM.put(addrConfig, config);
    publishEvent("Motion Detected", "", 7, 3);
  }

  if(config.tracking){
    tracker.gpsOn();
  }
}

void loop() {
  if(arming){
    // Moved this outside of arm() so that function doesn't timeout when called
    System.sleep(SLEEP_MODE_DEEP, config.wakeDelay);
  }
  if(config.tracking){
    updateLocation();
  }
  initLastState();
}

void initEEPROM(){
  EEPROM.get(addrConfig, config);
  if(config.version != 0){
    // Init Default Config in EEPROM. MUST LEAVE VERSION = 0!
    Config configInit = {0, 1, 0, 0, 30, 0, 21600};
    config = configInit;
    EEPROM.put(addrConfig, config);
    Serial.println("Initalized Default Configuration");
  }
}

void initLastState(){
  switch(config.state){
    case 0:
      // Disarmed, do nothing
      break;
    case 1:
      // Armed, resume sleep
      arm("");
      break;
  }
}

// Arms the tracker by dropping into deep sleep and waiting for wake signals
int arm(String command){
  if(command != NULL){
    int wd = atoi(command);
    if(wd != 0){
      config.wakeDelay = wd;
    }
  }

  LIS3DHConfig accelConfig;
	accelConfig.setLowPowerWakeMode(movementThreshold);
  accel.calibrateFilter(2000);
	if (!accel.setup(accelConfig)) {
		Serial.println("accelerometer not found");
	}

  setTracking("0");
  config.state = 1;
  EEPROM.put(addrConfig, config);
  char value[8];
  sprintf(value, "%d", config.wakeDelay);
  publishEvent("Armed", value, 1, 2);
  arming = true;
  return 1;
}

// Disarms tracker and puts device into an idle state
int disarm(String command){
  config.state = 0;
  EEPROM.put(addrConfig, config);
  publishEvent("Disarmed", "", 2, 2);
  return 1;
}

// Actively ask for a GPS reading if you're impatient. Only publishes if there's
// a GPS fix, otherwise returns '0'
int getLocation(String command){
  if(tracker.gpsFix()){
    // If GPS is on for a while it may say it's fixed but output no coords
    if(tracker.readLatLon().length() > 0){
      String data = tracker.readLatLon();
      lastGpsPublish = millis();
      if(config.tMode == 1){
        publishLocation(data);
      }else{
        Serial.println("LOCATION: " + data);
      }
    }
  }
  return 1;
}

int updateLocation(){
  tracker.updateGPS();
  if(lastGpsPublish == 0){
    getLocation("");
  }else{
    int timeDelay = ((60*60)/config.txRate)*1000; // How many milliseconds need to elapse before publish
    int timeElapsed = millis() - lastGpsPublish;
    if(timeElapsed >= timeDelay){
      getLocation("");
    }
  }
  return 1;
}

int isGPSFixed(String command){
  if(tracker.gpsFix()){
    return 1;
  }else{
    return 0;
  }
}

// Allows user to manually turn on or off GPS Receiver
int setTracking(String command){
  config.tracking = atoi(command);
  EEPROM.put(addrConfig, config);
  if(config.tracking == 0){
    tracker.gpsOff();
  }else{
    tracker.gpsOn();
  }
  char value[2];
  sprintf(value, "%d", config.tracking);
  publishEvent("Tracking Toggled", value, 3, 2);
  return 1;
}

int setTxRate(String command){
  config.txRate = atoi(command);
  EEPROM.put(addrConfig, config);
  char value[8];
  sprintf(value, "%d", config.txRate);
  publishEvent("TxRate Changed", value, 5, 2);
  return 1;
}

// Allows you to remotely change whether a device is publishing to the cloud
// or is only reporting data over Serial. Saves data when using only Serial!
// Change the default at the top of the code.
int setTransmitMode(String command){
  config.tMode = atoi(command);
  EEPROM.put(addrConfig, config);
  char value[2];
  sprintf(value, "%d", config.tMode);
  publishEvent("Transmit Mode Changed", value, 6, 1);
  return 1;
}

// Gets all information about tracker, not to be used often
int ping(String command){
  return 1;
}

int reboot(String command){
  publishEvent("Rebooted", "", 9, 2);
  System.reset();
  return 1;
}

int clearConfig(String command){
  publishEvent("Cleared Config", "", 10, 3);
  EEPROM.clear();
  System.reset();
  return 1;
}

int publishEvent(const char *name, const char *value, int code, int priority){
  // Sends json string containing info about events
  char data[64];
  sprintf(data, "{\"name\":\"%s\",\"value\":\"%s\",\"code\":%d,\"priority\":%d}"
    , name, value, code, priority);
  Particle.publish("EVENT", data, 60, PRIVATE);
  return 1;
}

int publishLocation(const char *position){
  String data = String::format("{\"position\":%s}", position);
  Particle.publish("LOCATION", data, 60, PRIVATE);
  return 1;
}
