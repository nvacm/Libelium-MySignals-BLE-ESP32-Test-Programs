/**
 * Libelium Main BLE Unit, Sensors: Position by wire, Blood Pressure by wire, PulseOximeter by wire, Temperature by wire 
 * Test program for ESP32 Heltek Lora32 v2
 * Author NetseekVA
 * If you use part of this code you must put link in about dialog or help console message to this github page and mention me!
 * Arduino IDE 1.8.13  Heltek-ESP32 sdk 0.0.5 NimBLE-Arduino 1.2.0  
 */

#include <NimBLEDevice.h>
                             
NimBLEUUID chrPOximeterUuid("362ba7aa-b620-41d3-89ee-48f865559129"); //362ba7aa-b620-41d3-89ee-48f865559129-Cable  362ba7aa-b620-41d3-89ee-48f865559129-BLE

uint8_t dtPOPulse=0; 
uint8_t dtPOOxi=0; 
uint8_t dtPosType=0;
uint8_t dtPosX=0;
uint8_t dtPosY=0;
uint8_t dtPosZ=0;
float   dtTemp=0.0;
uint8_t dtBLEState=0; // 0-not connected 1-connected
//  Notification / Indication receiving handler callback 
void notifyCB(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify)
{
    std::string str = (isNotify == true) ? "Notification" : "Indication";
    str += " from ";
    /** NimBLEAddress and NimBLEUUID have std::string operators */
    str += std::string(pRemoteCharacteristic->getRemoteService()->getClient()->getPeerAddress());
    str += ": Service = " + std::string(pRemoteCharacteristic->getRemoteService()->getUUID());
    str += ", Characteristic = " + std::string(pRemoteCharacteristic->getUUID());
    str += ", Value = " + std::string((char*)pData, length);
    str += ", len = ";
    Serial.print(str.c_str());
    Serial.println(length);
    if(pRemoteCharacteristic->getUUID() == chrPOximeterUuid){
      Serial.print("Notify PulseOximeter Oxigen: ");
            //uint16_t *uiData = (uint16_t*)pData; Serial.println((uint16_t) *uiData, HEX); 
            //Serial.println((long)*pData, HEX);

            //std::string value = pRemoteCharacteristic->readValue();
            //const uint8_t *uiOxigen = (const uint8_t *)value.c_str()[0];
            //const uint8_t *uiPulse = (const uint8_t *)value.c_str()[1];
            //Serial.print((unsigned char)*uiOxigen); Serial.print("  Pulse: "); Serial.println((unsigned char)*uiPulse);
            
            uint8_t *uiOxigen = (uint8_t *)pData;
            uint8_t *uiPulse = pData+1;
            Serial.print((unsigned char)*uiOxigen); 
            Serial.print("  Pulse: "); Serial.println((unsigned char)*uiPulse);
    }
}

class ClientCallbacks : public NimBLEClientCallbacks
{
  uint32_t onPassKeyRequest()
  { //Tricky - you must enter pin (from Libelium screen) in 3 sec to register on BLE - trust me it possible. I have success with this code. You must do it once as last registration are saved in Libelium device.
    Serial.println("Client Passkey Request. Please Enter PIN:");

    delay(3000);
    Serial.setTimeout(3000);

    long lpin=0;
    //String spin = "";
    int slen=0;
    //while(slen < 5){
    String spin = Serial.readString();
    slen = spin.length();
    Serial.print("Entered PIN (str): ");
    Serial.println(spin);
    
    lpin = spin.toInt();
    Serial.print("Entered PIN (long): ");
    Serial.println(lpin);
    //}
    Serial.setTimeout(1000);
    
    /** return the passkey to send to the server */
    /** Change this to be different from NimBLE_Secure_Server if you want to test what happens on key mismatch */
    /*delay(1000);
    Serial.setTimeout(2000);
    String spin = Serial.readString();
    Serial.setTimeout(1000);
    long lpin = spin.toInt();*/
    return (uint32_t)lpin;
  };
  bool onConfirmPIN(uint32_t pass_key){
        Serial.print("The passkey YES/NO number: ");
        Serial.println(pass_key);
        //Return false if passkeys don't match.
        return true;
  };
};
static ClientCallbacks clientCB;

// Initialize String Array
const char *strBPos[7] = { "0-NA", "1-Prone-vniz", "2-Left-Na levom boku", "3-Right-Na pravom boku", "4-Supine-Vverh", "5-Stand/Sit-Stoya/Sidya", "6-Unknown-Neizvestno/Vniz golovoy"};
//std::string colour[4] = { "Blue", "Red", "Orange", "Yellow" };

NimBLEScan *pScan;
 
void setup()
{
  Serial.begin(115200);
  Serial.println("Starting NimBLE Client");

  NimBLEDevice::init("");
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);
  NimBLEDevice::setSecurityAuth(true, true, true);   //(true, true, true);  
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_KEYBOARD_ONLY); // use passkey
  //NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_YESNO); //use numeric comparison
  pScan = NimBLEDevice::getScan();
}

void loop()
{
  dtBLEState = 0;
  delay(5000); Serial.print(".");
  NimBLEScanResults results = pScan->start(10);
  Serial.print(results.getCount());

  NimBLEUUID serviceUuid("1800");// "1800 00001800-0000-1000-8000-00805f9b34fb"   5b552788-5c7b-4ce8-8362-cf5dd093251d-srvcdata
  NimBLEUUID charct("2a00");
  #define LIBELIUM_BLE_MAC "55:77:33:22:33:11"
  for (int i = 0; i < results.getCount(); i++)
  {
    NimBLEAdvertisedDevice device = results.getDevice(i);
    Serial.println(i);
    Serial.println(device.getAddress().toString().c_str());
    if(device.getAddress().equals( NimBLEAddress(LIBELIUM_BLE_MAC))) Serial.println(">>>Addr equal");

    size_t count = device.getServiceUUIDCount();
    for(size_t i = 0; i < count; i++) {
          Serial.println(std::string(device.getServiceUUID(i)).c_str());
    }

    if (device.getAddress().equals( NimBLEAddress("LIBELIUM_BLE_MAC"))) //  device.isAdvertisingService(serviceUuid)
    {
      Serial.println("Advert");
      NimBLEClient *pClient = NimBLEDevice::createClient();
      pClient->setClientCallbacks(&clientCB, false);
      Serial.println("Create client");

      if (pClient->connect(&device))
      {
        //delay(4000);
        Serial.println("connect1");
        pClient->secureConnection();


        NimBLERemoteService *pSrvcDevOpt = pClient->getService("eed82c0a-b1c2-401e-ae4a-afac80c80c72"); //wifi security etc
        if (pSrvcDevOpt != nullptr)
        {
          Serial.println("srvcDevOpt");
          NimBLERemoteCharacteristic *pSCharcDevId = pSrvcDevOpt->getCharacteristic("88017ef1-515a-4029-b27b-6eb77f82e811"); //b518989c-98c6-4e1c-a567-e8f48faa7e76 - 5509
          if (pSCharcDevId != nullptr)
          {
            dtBLEState = 1;
            Serial.println("DevID ");
            //pSCharcDevId->writeValue(newValue.c_str(), newValue.length());
            //pSCharcDevId->writeValue("10396");
            unsigned char cSList[]={0x31,0x30,0x33,0x39,0x36};
            pSCharcDevId->writeValue(cSList, 5); // pCharacteristic->setValue((uint8_t*)&value, 4);
            
            std::string value = pSCharcDevId->readValue();
            Serial.println(value.c_str());
          }
        }
        
        NimBLERemoteService *pSrvcUser = pClient->getService("13744e03-9c88-4808-89a7-c2897e4e1b90");
        if (pSrvcUser != nullptr)
        {
          Serial.println("srvcUser");
          NimBLERemoteCharacteristic *pSCharcUser = pSrvcUser->getCharacteristic("b518989c-98c6-4e1c-a567-e8f48faa7e76"); //b518989c-98c6-4e1c-a567-e8f48faa7e76 - 5509
          if (pSCharcUser != nullptr)
          {
            Serial.println("CharUserName ");
            //pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
            ///pSCharcUser->writeValue("5509");
            unsigned char cSList[]={0x35,0x35,0x30,0x39};
            pSCharcUser->writeValue(cSList, 4); // pCharacteristic->setValue((uint8_t*)&value, 4);
            
            std::string value = pSCharcUser->readValue();
            Serial.println(value.c_str());
          }

        // 13744e039c88480889a7c2897e4e1b90 9737456d-d754-460d-a22b-39ab73ccd0af  0x010101aa7800
       NimBLERemoteCharacteristic *pSCharcUData = pSrvcUser->getCharacteristic("9737456d-d754-460d-a22b-39ab73ccd0af"); //b518989c-98c6-4e1c-a567-e8f48faa7e76 - 5509
          if (pSCharcUData != nullptr)
          {
            Serial.println("CharUserData ");
            //pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
            unsigned char cSList[]={0x01,0x01,0x01,0xaa,0x78,0x00};
            pSCharcUData->writeValue(cSList, 6); // pCharacteristic->setValue((uint8_t*)&value, 4);
            
            std::string value = pSCharcUData->readValue();
            Serial.println(value.c_str());

            uint32_t *uistr = (uint32_t *) value.c_str();
            Serial.println((unsigned long) *uistr, HEX);
            uistr = (uint32_t *) cSList;
            Serial.println((unsigned long) *uistr, HEX);
          }
          
          NimBLERemoteCharacteristic *pSCharcCountry = pSrvcUser->getCharacteristic("9f1c0185-57c6-4037-a4e8-9f5d3b3c0ca6"); //"User Country" 9f1c0185-57c6-4037-a4e8-9f5d3b3c0ca6 - es
          if (pSCharcCountry != nullptr)
          {
            Serial.println("CharcCountry");
            ///pSCharcCountry->writeValue("es");
            unsigned char csVal[]={'e','s'};
            pSCharcCountry->writeValue(csVal, 2);
            
            std::string value = pSCharcCountry->readValue();
            Serial.println(value.c_str());
          }

        }
        NimBLERemoteService *pSrvcData = pClient->getService("5b552788-5c7b-4ce8-8362-cf5dd093251d");
        if (pSrvcData != nullptr)
        { Serial.println("pSrvcData OK");
          NimBLERemoteCharacteristic *pSCharcSList = pSrvcData->getCharacteristic("e562b410-e8d2-4fe0-89c1-91432f108fe1"); //"Sensor list" e562b410-e8d2-4fe0-89c1-91432f108fe1 - 50-A8-00
          if (pSCharcSList != nullptr)
          {
            Serial.println("CharcSensorList ");//52a800-all except ble button    5288 all-noPulsOxim
            char cSList[]={0x52,0xA8,0x00};
            //cSList[0]=0x50; cSList[1]=0xA8; cSList[2]=0x00;
            pSCharcSList->writeValue(cSList, 3);
            //pSCharcSList->setValue((char*)cSList, 3);
            std::string value = pSCharcSList->readValue();
            uint32_t *uistr = (uint32_t *) value.c_str();
            Serial.println((unsigned long) *uistr, HEX);
            //Serial.println(value.c_str());
          }
          // PulsOximiter Pulse Oxigen   ("362ba79d-b620-41d3-89ee-48f865559129"); //value: (0x) 14-13
          /*NimBLERemoteCharacteristic *pSCharcPOxim = pSrvcData->getCharacteristic(chrPOximeterUuid);
          if (pSCharcPOxim != nullptr)
          {
            if(pSCharcPOxim->canNotify()) {
                //if(!pChr->registerForNotify(notifyCB)) {
                pSCharcPOxim->registerForNotify(notifyCB,true);
                //if(!pSCharcPOxim->subscribe(true, notifyCB)) {        Serial.println("Disconnect if subscribe failed");                    pClient->disconnect();                    return; }
            }
          }*/
       }
       
       while(true){
        
        if (!pClient->isConnected()) {
          dtBLEState=0;
          ///Serial.println("Error: Device Disconnected");
          NimBLEDevice::deleteClient(pClient);
          return;
        }
        
        if (pSrvcData != nullptr)
        {
          // PulsOximiter Pulse Oxigen 362ba7aa-b620-41d3-89ee-48f865559129  value: (0x) 5F-4B, "_K"
          NimBLERemoteCharacteristic *pSCharcPOxim = pSrvcData->getCharacteristic("362ba7aa-b620-41d3-89ee-48f865559129"); //value: (0x) 14-13
          if (pSCharcPOxim != nullptr)
          {
            ///Serial.print("Charc PulseOximeter Oxigen: ");
            
            std::string value = pSCharcPOxim->readValue();
            ///Serial.print("len="); Serial.print(value.length()); Serial.print(" ");
            uint8_t *uiOxigen = (uint8_t *)value.c_str();
            uint8_t *uiPulse = (uint8_t *)uiOxigen+1;

            dtPOPulse=*uiPulse; dtPOOxi=*uiOxigen; 
            ///Serial.print((unsigned char)*uiOxigen); Serial.print("  Pulse: ");   Serial.println((unsigned char)*uiPulse);
          }
          
          // Body position
          NimBLERemoteCharacteristic *pSCharcBPos = pSrvcData->getCharacteristic("acfe574f-ceaf-4290-90d4-4a94b0c5b8ef"); //value: (0x) 04-03-00-F1
          if (pSCharcBPos != nullptr)
          {
            ///Serial.println("Charc Position XYZP: ");
            std::string value = pSCharcBPos->readValue();
            uint8_t *uiPos = (uint8_t *)value.c_str();
            uint8_t *uiX = uiPos+1;
            uint8_t *uiY = uiPos+2;
            uint8_t *uiZ = uiPos+3;
            dtPosType = *uiPos; dtPosX = *uiX; dtPosY = *uiY; dtPosZ = *uiZ;
            
            //const uint8_t *uiZ = (const uint8_t *)value.c_str()[2];
            //const uint8_t *uiPos = (const uint8_t *)value.c_str()[3];
            ///Serial.print((unsigned char)*uiX, HEX); Serial.print(" "); Serial.print((unsigned char)*uiY, HEX); Serial.print(" "); 
            ///Serial.print((unsigned char)*uiZ, HEX); Serial.print(" "); Serial.print((unsigned char)*uiPos, HEX); Serial.print(" ");
            ///if(*uiPos>=0 && *uiPos<=6) Serial.print(strBPos[*uiPos]);
            ///Serial.println(" ");
          }
          NimBLERemoteCharacteristic *pSCharcTemp = pSrvcData->getCharacteristic("362ba79d-b620-41d3-89ee-48f865559129"); //value: (0x) 03-0A
          if (pSCharcTemp != nullptr)
          {
            ///Serial.print("CharTemp ");
            std::string value = pSCharcTemp->readValue();
            //const char *cstr = value.c_str();
            uint16_t *uistr = (uint16_t *)value.c_str();
            ///Serial.println((unsigned int)*uistr);
            ///Serial.println((unsigned int)*uistr, HEX);
            dtTemp = (*uistr) / 100.0;
            //Serial.print(cstr[0]); Serial.println(cstr[1]);
          }
        }
        //std::string NimBLEClient::getValue(const NimBLEUUID &serviceUUID, const NimBLEUUID &characteristicUUID)
        //acfe574f-ceaf-4290-90d4-4a94b0c5b8ef "Body Position"
          PrintJSONStr();
         delay(1500);
        } //while
      }
      else
      {
        // failed to connect
        Serial.println("failed to connect");
      }
      NimBLEDevice::deleteClient(pClient);
    }
  }
  PrintJSONStr();
}

void PrintJSONStr(){
  if(dtBLEState!=1) {dtPosX=0; dtPosY=0; dtPosZ=0; dtTemp=0;}
  Serial.print("{ \"BLEStatus\": "); Serial.print(dtBLEState); Serial.print(",");
  Serial.print(" \"Temperature\": "); Serial.print(dtTemp); Serial.print(",");
  Serial.print(" \"Pulse\": "); Serial.print(dtPOPulse); Serial.print(",");
  Serial.print(" \"Oxigen\": "); Serial.print(dtPOOxi); Serial.print(",");
  
  Serial.print(" \"PositionType\": "); Serial.print(dtPosType); Serial.print(",");
  if(dtPosType>=0 && dtPosType<=6) Serial.print(" \"PositionTypeStr\": \"");  Serial.print(strBPos[dtPosType]); Serial.print("\",");
  Serial.print(" \"PositionX\": "); Serial.print(dtPosX); Serial.print(",");
  Serial.print(" \"PositionY\": "); Serial.print(dtPosY); Serial.print(",");
  Serial.print(" \"PositionZ\": "); Serial.print(dtPosZ); //Serial.print(",");
  
  //Serial.print("{ \"BLEStatus\": "); Serial.print(dtBLEState); Serial.println(",");
  //Serial.print("{ \"BLEStatus\": "); Serial.print(dtBLEState); Serial.println(",");
  //  "last_seen": "2024-08-13T21:32:33+03:00",
  Serial.println(" }");
  
}

