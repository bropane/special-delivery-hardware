
#include "Particle.h"

#include "LIS3DH.h"
#include "AssetTracker.h"
#include "math.h"

// Example of Wake On Move with the AssetTracker and the Electron
//
// Official project location:
// https://github.com/rickkas7/LIS3DH


// System threading is required for this project
SYSTEM_THREAD(ENABLED);

const int BUILD = 1;

// Various timing constants
const unsigned long MAX_TIME_TO_PUBLISH_MS = 60000; // Only stay awake for 60 seconds trying to connect to the cloud and publish
const unsigned long TIME_AFTER_PUBLISH_MS = 4000; // After publish, wait 4 seconds for data to go out
const unsigned long TIME_AFTER_BOOT_MS = 5000; // At boot, wait 5 seconds before going to sleep again (after coming online)
const unsigned long TIME_PUBLISH_BATTERY_SEC = 22 * 60; // every 22 minutes send a battery update to keep the cellular connection up

const uint8_t movementThreshold = 16;

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
Config config;

int lastGpsPublish = 0;

void setup() {
  Serial.begin(9600);
  initEEPROM();
  Serial.println("Connection Established");
  Serial.println("Special Delivery Build: " + BUILD);

  Particle.variable("tmode",config.tMode);
  Particle.variable("track",config.tracking);
  Particle.variable("txRate", config.txRate);
  Particle.variable("armState",armStateToString);

  Particle.function("arm", arm);
  Particle.function("disarm", disarm);
  Particle.function("tMode", transmitMode);
  Particle.function("batt", batteryStatus);
  Particle.function("coords", getCoords);
  Particle.function("tracking", setTracking);
  Particle.function("status", publishStatus);
  Particle.function("setTxRate", setTxRate);
  Particle.function("ping", ping);
  Particle.function("reboot", reboot);
  Particle.function("clear_config", clearConfig);

  initLastState();
}


void loop() {

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

  String armStateToString(){
      switch(config.state){
          case 0:
          return "DISARMED";
          case 1:
          return "ARMED";
      }
  }

  // Allows you to remotely change whether a device is publishing to the cloud
  // or is only reporting data over Serial. Saves data when using only Serial!
  // Change the default at the top of the code.
  int transmitMode(String command){
      config.tMode = atoi(command);
      EEPROM.put(addrConfig, config);
      return 1;
  }

  // Arms the tracker by dropping into deep sleep and waiting for wake signals
  int arm(String command){
    if(command != NULL){
        int wd = atoi(command);
        if(wd != 0){
            config.wakeDelay = wd;
        }
    }

    LIS3DHConfig config;
		config.setLowPowerWakeMode(movementThreshold);
    accel.calibrateFilter(2000);
		if (!accel.setup(config)) {
			Serial.println("accelerometer not found");
			state = SLEEP_STATE;
			break;
		}

    config.state = 1;
    EEPROM.put(addrConfig, config);
    publishStatus("");
    System.sleep(SLEEP_MODE_DEEP, config.wakeDelay);

    return 1;
  }

  // Disarms tracker and puts device into an idle state
  int disarm(String command){
      config.state = 0;
      EEPROM.put(addrConfig, config);
      publishStatus("");
      return 1;
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
      return 1;
  }

  int setTxRate(String command){
      config.txRate = atoi(command);
      EEPROM.put(addrConfig, config);
      return 1;
  }

  // Gets all information about tracker, not to be used often
  int ping(String command){
      return 1;
  }

  // Actively ask for a GPS reading if you're impatient. Only publishes if there's
  // a GPS fix, otherwise returns '0'
  int getCoords(String command){
      if(tracker.gpsFix()){
          lastGpsPublish = Time.now();
          if(config.tMode == 1){
              Particle.publish("COORD", tracker.readLatLon(), 60, PRIVATE);
          }else{
              Serial.println("Coordinates: " + tracker.readLatLon());
          }
          return 1;
      }
      else{
          return 0;
      }
  }

  int updateCoords(){
     if(lastGpsPublish == 0){
         getCoords("");
     }else{
         int timeDelay = (60*60)/config.txRate; // How many seconds need to elapse before publish
         int timeElapsed = Time.now() - lastGpsPublish;
         if(timeElapsed >= timeDelay){
             getCoords("");
         }
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

  int publishStatus(String command){
      Particle.publish("STATUS", armStateToString(), 60, PRIVATE);
      return 1;
  }
  int reboot(String command){
      return 1;
  }

  int clearConfig(String command){
      EEPROM.clear();
      System.reset();
      return 1;
  }

}
