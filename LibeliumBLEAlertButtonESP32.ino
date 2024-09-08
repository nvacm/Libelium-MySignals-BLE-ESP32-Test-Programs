/**
 * Libelium MySignals BLE Alert Button simple program to catch button pressed alert
 * Test program for ESP32 Heltek Lora32 v2
 * Author NetseekVA
 * If you use part of this code you must put link in about dialog or help console message to this github page and mention me!
 * Arduino IDE 1.8.13  Heltek-ESP32 sdk 0.0.5 NimBLE-Arduino 1.2.0  
 */

#include "BLEDevice.h"
//#include "BLEScan.h"

// The remote service we wish to connect to.
//service advert - 00001803-0000-1000-8000-00805f9b34fb   button notify - 0000ffe0-0000-1000-8000-00805f9b34fb
static BLEUUID  advertserviceUUID("00001803-0000-1000-8000-00805f9b34fb");
static BLEUUID  serviceUUID("0000ffe0-0000-1000-8000-00805f9b34fb");  // serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");   
// The characteristic of the remote service we are interested in.
//characteristic button notify - 0000ffe1-0000-1000-8000-00805f9b34fb
static BLEUUID    ChrcUUIDButton("0000ffe1-0000-1000-8000-00805f9b34fb");   //ChrcUUIDButton("beb5483e-36e1-4688-b7f5-ea07361b26a8");
static BLEUUID    ChrcUUIDAlarm("00002a06-0000-1000-8000-00805f9b34fb");

//BLEClient*  pClient;
static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = true;
static BLERemoteCharacteristic* pChrcButton;
static BLERemoteCharacteristic* pChrcAlarm;
static BLEAdvertisedDevice* myDevice;

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    Serial.print("Button Alert Notify "); //callback for characteristic
    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    Serial.print(" of data length ");
    Serial.print(length);
    Serial.print(" data: ");
    Serial.println((uint8_t)*pData, HEX);
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    doConnect = false;
    doScan = true;
    Serial.println("onDisconnect");
  }
};

bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    
    BLEClient* pClient = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find Button service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found Button service");

    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pChrcButton = pRemoteService->getCharacteristic(ChrcUUIDButton);
    if (pChrcButton == nullptr) {
      Serial.print("Failed to find Button characteristic UUID: ");
      Serial.println(ChrcUUIDButton.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found Button characteristic");

    // Read the value of the characteristic.
    if(pChrcButton->canRead()) {
      std::string value = pChrcButton->readValue();
      Serial.print(" The characteristic value was: ");
      Serial.println(value.c_str());
    }

    if(pChrcButton->canNotify()){
      pChrcButton->registerForNotify(notifyCallback);
      Serial.println(" - Register notify for Button characteristic");
    }

    // Obtain a reference to the service we are after in the remote BLE server.
    pRemoteService = pClient->getService("1802");
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find Alarm service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found Alarm service");
    
    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pChrcAlarm = pRemoteService->getCharacteristic(ChrcUUIDAlarm);
    if (pChrcAlarm == nullptr) {
      Serial.print("Failed to find Alarm characteristic UUID: ");
      Serial.println(ChrcUUIDAlarm.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found Alarm characteristic");

    // Write the value of the Alarm characteristic.
    //if(pChrcAlarm->canWrite()) {
      // Send Alarm signal
      uint8_t uiAlarmCode[] = {0x02, 0};
      //pChrcAlarm->writeValue(uiAlarmCode);
      pChrcAlarm->writeValue((uint8_t*) uiAlarmCode, (size_t) 1);
      Serial.println(" - Send Alarm signal.");
    //}
    connected = true;
    return true;
}
/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    //if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(advertserviceUUID)) {
    #define LIBELIUM_BLE_BUTTON_MAC "55:77:33:22:33:22"
	if (advertisedDevice.getAddress().equals( BLEAddress(LIBELIUM_BLE_BUTTON_MAC))){
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true; //CM
      Serial.println(" ITAG BLE Button connected.");
    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks


void setup() {
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(10, false);
} // End of setup.

// This is the Arduino main loop function.
void loop() {

  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("Connected to the BLE Server.");
       
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
       
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {
    //String newValue = "Time since boot: " + String(millis()/1000);
    //Serial.println("Setting new characteristic value to \"" + newValue + "\"");
    // Set the characteristic's value to be the array of bytes that is actually a string.
    ///cm  pChrcButton->writeValue(newValue.c_str(), newValue.length());
  }else {
    BLEDevice::getScan()->start(4);  // this is just sample to start scan after disconnect, most likely there is better way to do it in arduino
  }
  delay(1000); // Delay a second between loops.
} // End of loop
