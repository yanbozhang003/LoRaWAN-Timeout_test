/*
  RadioLib LoRaWAN End Device ABP Example

  This example sets up a LoRaWAN node using ABP (activation
  by personalization). Before you start, you will have to
  register your device at https://www.thethingsnetwork.org/
  After your device is registered, you can run this example.
  The device will start uploading data directly,
  without having to join the network.

  NOTE: LoRaWAN requires storing some parameters persistently!
        RadioLib does this by using EEPROM, by default
        starting at address 0 and using 32 bytes.
        If you already use EEPROM in your application,
        you will have to either avoid this range, or change it
        by setting a different start address by changing the value of
        RADIOLIB_HAL_PERSISTENT_STORAGE_BASE macro, either
        during build or in src/BuildOpt.h.

  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

// SX1278 has the following connections:
// NSS pin:   10
// DIO0 pin:  2
// RESET pin: 9
// DIO1 pin:  3
SX1276 radio = new Module(PB6, PA10, PC7, PB10);

// create the node instance on the EU-868 band
// using the radio module and the encryption key
// make sure you are using the correct band
// based on your geographical location!
LoRaWANNode node(&radio, &EU868);

unsigned long ts = 0;
unsigned long airtime = 0;
unsigned long offtime = 0;

int Max_RT = 0;               // maximum number of re-transmission
int RT_FLAG = 0;
int emu_device_number = 1;
int base_interval = 15*60;    // assume base interval to be each node transmitting every 15 minutes, convert unit to second
int tx_interval = base_interval;    // tx interval is initilized to be base interval
int MAC_scheme = 1;           // MAC_scheme: 1 -> ALOHA; 2 -> CSMA; 3 -> XCSMA

void setup() {
  Serial.begin(9600);

  // initialize SX1276 with default settings
  Serial.print(F("[SX1276] Initializing ... "));
  // int state = radio.begin(868.3, 125.0, 10, 5, RADIOLIB_SX127X_SYNC_WORD, 14, 8, 1);
  int state = radio.begin();
  if(state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while(true);
  }

  // first we need to initialize the device storage
  // this will reset all persistently stored parameters
  // NOTE: This should only be done once prior to first joining a network!
  //       After wiping persistent storage, you will also have to reset
  //       the end device in TTN!
  //node.wipe();

  // device address - this number can be anything
  // when adding new end device in TTN, you can generate this number,
  // or you can set any value you want, provided it is unique
  uint32_t devAddr = 0x234567A1;      // devAddr for node C3

  // select some encryption keys which will be used to secure the communication
  // there are two of them - network key and application key
  // because LoRaWAN uses AES-128, the key MUST be 16 bytes (or characters) long

  // network key is the ASCII string "topSecretKey1234"
  uint8_t nwkSKey[] = { 0x74, 0x6F, 0x70, 0x53, 0x65, 0x63, 0x72, 0x65,
                        0x74, 0x4B, 0x65, 0x79, 0x31, 0x32, 0x33, 0xA1 };       // network session key for node C3

  // application key is the ASCII string "aDifferentKeyABC"
  uint8_t appSKey[] = { 0x61, 0x44, 0x69, 0x66, 0x66, 0x65, 0x72, 0x65,
                        0x6E, 0x74, 0x4B, 0x65, 0x79, 0x41, 0x42, 0xA1 };       // app session key for node C3

  // prior to LoRaWAN 1.1.0, only a single "nwkKey" is used
  // when connecting to LoRaWAN 1.0 network, "appKey" will be disregarded
  // and can be set to NULL

  // some frequency bands only use a subset of the available channels
  // you can set the starting channel and their number
  // for example, the following corresponds to US915 FSB2 in TTN
  /*
    node.startChannel = 8;
    node.numChannels = 8;
  */

  // start the device by directly providing the encryption keys and device address
  Serial.print(F("[LoRaWAN] Attempting ABP activation ... "));
  state = node.beginABP(devAddr, (uint8_t*)nwkSKey, (uint8_t*)appSKey);
  if(state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while(true);
  }

  // node.setCSMA(8, 4, true);

  // wait for 2 seconds for RxWindow to expire
  delay(2000);

  // after the device has been activated,
  // network can be rejoined after device power cycle
  // by calling "begin"
  /*
    Serial.print(F("[LoRaWAN] Resuming previous session ... "));
    state = node.begin();
    if(state == RADIOLIB_ERR_NONE) {
      Serial.println(F("success!"));
    } else {
      Serial.print(F("failed, code "));
      Serial.println(state);
      while(true);
    }
  */
}

// counter to keep track of transmitted packets
int count = 0;
int RT_count = 0;

void loop() {
  // send uplink to port 10
  Serial.print(F("[LoRaWAN] Sending uplink packet || "));
  String strUp = "Hello World! #" + String(count);
  Serial.println(strUp);

  int randomNumber = random(1001);
  delay(randomNumber);

  ts = millis();

  int state = node.uplink(strUp, 10, 1);       // send confirmed frame
  // }else{
  //   int state = node.uplink(strUp, 10, false);      // send unconfirmed frame
  // }
  airtime = millis() - ts;

  RT_FLAG = 0;

  tx_interval = base_interval / emu_device_number;

  Serial.print(F("Emulated device number: "));
  Serial.print(emu_device_number);
  Serial.print(F(" | airtime/offtime: "));
  Serial.print(airtime);
  Serial.print(F("/"));
  Serial.print(tx_interval*1000);
  // Serial.print(F(" | SF: "));
  // Serial.print(SF);
  // Serial.print(F(" | TX_POWER: "));
  // Serial.print(TX_POWER);
  // Serial.print(F(" | CF: "));
  // Serial.print(CENTER_FREQUENCY);

  if(state == RADIOLIB_ERR_NONE) {
    Serial.println(F(" || success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
  }

  // after uplink, you can call downlink(),
  // to receive any possible reply from the server
  // this function must be called within a few seconds
  // after uplink to receive the downlink!
  Serial.print(F("[LoRaWAN] Waiting for downlink ... "));
  String strDown;
  state = node.downlink(strDown);
  if(state == RADIOLIB_ERR_NONE) {
    count++;
    RT_count = 0;
    // Serial.println(F("success!"));
    //
    // // print data of the packet (if there are any)
    // Serial.print(F("[LoRaWAN] Data:\t\t"));
    // if(strDown.length() > 0) {
    //   Serial.println(strDown);
    // } else {
    //   Serial.println(F("<MAC commands only>"));
    // }
    //
    // print RSSI (Received Signal Strength Indicator)
    Serial.print(("PHY measurement: RSSI: "));
    Serial.print(radio.getRSSI());
    Serial.print((" dBm || "));

    // print SNR (Signal-to-Noise Ratio)
    Serial.print(("SNR: "));
    Serial.print(radio.getSNR());
    Serial.println((" dB"));

    // // print frequency error
    // Serial.print(F("[LoRaWAN] Frequency error:\t"));
    // Serial.print(radio.getFrequencyError());
    // Serial.println(F(" Hz"));
  
  } else {
    if(state == RADIOLIB_ERR_RX_TIMEOUT) {
      Serial.println(F("timeout!"));
    } else {
      Serial.print(F("failed, code "));
      Serial.println(state);
    }

    RT_count++;
    RT_FLAG = 1;

    if(RT_count == Max_RT+1){
      count++;
      RT_count = 0;
      RT_FLAG = 0;
    }
  }
  // else if(state == RADIOLIB_ERR_RX_TIMEOUT) {
  //   Serial.println(F("timeout!"));
  
  // } else {
  //   Serial.print(F("failed, code "));
  //   Serial.println(state);
  // }

  if(RT_FLAG == 0){
    ts = millis();
    while((millis() - ts) < tx_interval*1000) {
      if(Serial.available()) {
        String cmd = Serial.readString();
        cmd.trim();
        Serial.println(cmd);
        String c = "";
        for(int i = 0; i < sizeof(cmd); i++) {
          if(cmd[i] != ' ') {
            c += cmd[i];
            if(c.equals("DN")) {
              emu_device_number = cmd.substring(i + 1).toInt();
              if( emu_device_number < 1 || emu_device_number > 500) {
                Serial.println(F("Invalid Duty Cycle Value. Must be positive integer from 1 - 500"));
                break;
              }
            
              tx_interval = base_interval / emu_device_number;
              count=0;      // set counter to 0 every time reconfiguring the parameters
              RT_count=0;
            
              Serial.print(F("Device number set to "));
              Serial.print(emu_device_number);
              Serial.print(F(" -> New offset time: "));
              Serial.print(tx_interval);
              Serial.println(F("sec"));
            }
            if(c.equals("RT")) {
              int Max_RT_temp = cmd.substring(i + 1).toInt();
              if( Max_RT_temp < 0 || Max_RT_temp > 15) {
                Serial.println(F("Invalid Max. Re-transmission Value. Must be positive integer from 0 - 15 (0 means no re-transmission)."));
                break;
              }

              Max_RT = Max_RT_temp;

              count=0;      // set counter to 0 every time reconfiguring the parameters
              RT_count=0;
            
              Serial.print(F("Max. Re-transmission number set to "));
              Serial.println(Max_RT);
            }
            if(c.equals("MAC")) {
              int MAC_scheme_temp = cmd.substring(i + 1).toInt();
              // if( MAC_scheme_temp < 0 || MAC_scheme_temp > 2) {
              //   Serial.println(F("Invalid MAC scheme Value. Must be within {0,1,2}."));
              //   break;
              // }
              // 0: ALOHA
              // 9: CSMA
              // 1-8: XCSMA with 1-8 number of consecutive up/down chirps

              MAC_scheme = MAC_scheme_temp;

              node.setCSMA(8, 4, MAC_scheme);

              count=0;      // set counter to 0 every time reconfiguring the parameters
              RT_count=0;
            
              Serial.print(F("MAC scheme set to "));
              Serial.println(MAC_scheme);
            }
          }
          else{
            break;
          }
        }
      }
    }
  }else{
    delay(2000);
  }
}
