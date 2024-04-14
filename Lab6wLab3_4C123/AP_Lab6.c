// AP_Lab6.c
// Runs on either MSP432 or TM4C123
// see GPIO.c file for hardware connections 

// Daniel Valvano and Jonathan Valvano
// November 20, 2016
// CC2650 booster or CC2650 LaunchPad, CC2650 needs to be running SimpleNP 2.2 (POWERSAVE)

#include <stdint.h>
#include "../inc/UART0.h"
#include "../inc/UART1.h"
#include "../inc/AP.h"
#include "AP_Lab6.h"
//**debug macros**APDEBUG defined in AP.h********
#ifdef APDEBUG
#define OutString(STRING) UART0_OutString(STRING)
#define OutUHex(NUM) UART0_OutUHex(NUM)
#define OutUHex2(NUM) UART0_OutUHex2(NUM)
#define OutChar(N) UART0_OutChar(N)
#else
#define OutString(STRING)
#define OutUHex(NUM)
#define OutUHex2(NUM)
#define OutChar(N)
#endif

//****links into AP.c**************
extern const uint32_t RECVSIZE;
extern uint8_t RecvBuf[];
typedef struct characteristics{
  uint16_t theHandle;          // each object has an ID
  uint16_t size;               // number of bytes in user data (1,2,4,8)
  uint8_t *pt;                 // pointer to user data, stored little endian
  void (*callBackRead)(void);  // action if SNP Characteristic Read Indication
  void (*callBackWrite)(void); // action if SNP Characteristic Write Indication
}characteristic_t;
extern const uint32_t MAXCHARACTERISTICS;
extern uint32_t CharacteristicCount;
extern characteristic_t CharacteristicList[];
typedef struct NotifyCharacteristics{
  uint16_t uuid;               // user defined 
  uint16_t theHandle;          // each object has an ID (used to notify)
  uint16_t CCCDhandle;         // generated/assigned by SNP
  uint16_t CCCDvalue;          // sent by phone to this object
  uint16_t size;               // number of bytes in user data (1,2,4,8)
  uint8_t *pt;                 // pointer to user data array, stored little endian
  void (*callBackCCCD)(void);  // action if SNP CCCD Updated Indication
}NotifyCharacteristic_t;
extern const uint32_t NOTIFYMAXCHARACTERISTICS;
extern uint32_t NotifyCharacteristicCount;
extern NotifyCharacteristic_t NotifyCharacteristicList[];
//**************Lab 6 routines*******************
// **********SetFCS**************
// helper function, add check byte to message
// assumes every byte in the message has been set except the FCS
// used the length field, assumes less than 256 bytes
// FCS = 8-bit EOR(all bytes except SOF and the FCS itself)
// Inputs: pointer to message
//         stores the FCS into message itself
// Outputs: none
void SetFCS(uint8_t *msg){
//****You implement this function as part of Lab 6*****
  uint8_t fcs = 0;
  uint32_t size=AP_GetSize(msg);
  msg++; //skip the SOF
  fcs=fcs^(*msg);  // LSB length
  msg++;
  fcs=fcs^(*msg);  // MSB length
  msg++;
  fcs=fcs^(*msg);  // CMD0
  msg++;
  fcs=fcs^(*msg);  // CMD1
  msg++;
  // calculate fcs for payload
  for(int i=0;i<size;i++)
  {
    fcs=fcs^(*msg); 
    msg++; // payload
  }
  *msg = fcs;
}
//*************BuildGetStatusMsg**************
// Create a Get Status message, used in Lab 6
// Inputs pointer to empty buffer of at least 6 bytes
// Output none
// build the necessary NPI message that will Get Status
void BuildGetStatusMsg(uint8_t *msg){
// hint: see NPI_GetStatus in AP.c
//****You implement this function as part of Lab 6*****
  uint8_t NPI_GetStatus[6] =   {SOF,0x00,0x00,0x55,0x06,0x00};
  SetFCS(NPI_GetStatus);
  for (uint8_t i=0; i<6; i++)
  {
    *msg = NPI_GetStatus[i];
    msg++;
  }
}
//*************Lab6_GetStatus**************
// Get status of connection, used in Lab 6
// Input:  none
// Output: status 0xAABBCCDD
// AA is GAPRole Status
// BB is Advertising Status
// CC is ATT Status
// DD is ATT method in progress
uint32_t Lab6_GetStatus(void){volatile int r; uint8_t sendMsg[8];
  OutString("\n\rGet Status");
  BuildGetStatusMsg(sendMsg);
  r = AP_SendMessageResponse(sendMsg,RecvBuf,RECVSIZE);
  return (RecvBuf[4]<<24)+(RecvBuf[5]<<16)+(RecvBuf[6]<<8)+(RecvBuf[7]);
}

//*************BuildGetVersionMsg**************
// Create a Get Version message, used in Lab 6
// Inputs pointer to empty buffer of at least 6 bytes
// Output none
// build the necessary NPI message that will Get Status
void BuildGetVersionMsg(uint8_t *msg){
// hint: see NPI_GetVersion in AP.c
//****You implement this function as part of Lab 6*****
  uint8_t NPI_GetVersion[6] =  {SOF,0x00,0x00,0x35,0x03,0x00};
  SetFCS(NPI_GetVersion);
  for (uint8_t i=0; i<6; i++)
  {
    *msg = NPI_GetVersion[i];
    msg++;
  }
}
//*************Lab6_GetVersion**************
// Get version of the SNP application running on the CC2650, used in Lab 6
// Input:  none
// Output: version
uint32_t Lab6_GetVersion(void){volatile int r;uint8_t sendMsg[8];
  OutString("\n\rGet Version");
  BuildGetVersionMsg(sendMsg);
  r = AP_SendMessageResponse(sendMsg,RecvBuf,RECVSIZE); 
  return (RecvBuf[5]<<8)+(RecvBuf[6]);
}

//*************BuildAddServiceMsg**************
// Create an Add service message, used in Lab 6
// Inputs uuid is 0xFFF0, 0xFFF1, ...
//        pointer to empty buffer of at least 9 bytes
// Output none
// build the necessary NPI message that will add a service
void BuildAddServiceMsg(uint16_t uuid, uint8_t *msg){
//****You implement this function as part of Lab 6*****
  uint8_t NPI_AddService[9] = 
  {
  SOF,3,0x00,     // length = 3
  0x35,0x81,      // SNP Add Service
  0x01,           // Primary Service
  0x00,0x00,
  0x00            // FCS
  };
  NPI_AddService[6] = uuid&0xFF;
  NPI_AddService[7] = uuid>>8;
  SetFCS(NPI_AddService);
  for (uint8_t i=0; i<9; i++)
  {
    *msg = NPI_AddService[i];
    msg++;
  }
}
//*************Lab6_AddService**************
// Add a service, used in Lab 6
// Inputs uuid is 0xFFF0, 0xFFF1, ...
// Output APOK if successful,
//        APFAIL if SNP failure
int Lab6_AddService(uint16_t uuid){ int r; uint8_t sendMsg[12];
  OutString("\n\rAdd service");
  BuildAddServiceMsg(uuid,sendMsg);
  r = AP_SendMessageResponse(sendMsg,RecvBuf,RECVSIZE);  
  return r;
}
//*************AP_BuildRegisterServiceMsg**************
// Create a register service message, used in Lab 6
// Inputs pointer to empty buffer of at least 6 bytes
// Output none
// build the necessary NPI message that will register a service
void BuildRegisterServiceMsg(uint8_t *msg){
//****You implement this function as part of Lab 6*****
  uint8_t NPI_Register[6] = 
  {   
  SOF,0x00,0x00,  // length = 0
  0x35,0x84,      // SNP Register Service
  0x00            // FCS
  };
  SetFCS(NPI_Register);
  for (uint8_t i=0; i<6; i++)
  {
    *msg = NPI_Register[i];
    msg++;
  }
}
//*************Lab6_RegisterService**************
// Register a service, used in Lab 6
// Inputs none
// Output APOK if successful,
//        APFAIL if SNP failure
int Lab6_RegisterService(void){ int r; uint8_t sendMsg[8];
  OutString("\n\rRegister service");
  BuildRegisterServiceMsg(sendMsg);
  r = AP_SendMessageResponse(sendMsg,RecvBuf,RECVSIZE);
  return r;
}

//*************BuildAddCharValueMsg**************
// Create a Add Characteristic Value Declaration message, used in Lab 6
// Inputs uuid is 0xFFF0, 0xFFF1, ...
//        permission is GATT Permission, 0=none,1=read,2=write, 3=Read+write 
//        properties is GATT Properties, 2=read,8=write,0x0A=read+write, 0x10=notify
//        pointer to empty buffer of at least 14 bytes
// Output none
// build the necessary NPI message that will add a characteristic value
void BuildAddCharValueMsg(uint16_t uuid,  
  uint8_t permission, uint8_t properties, uint8_t *msg){
// set RFU to 0 and
// set the maximum length of the attribute value=512
// for a hint see NPI_AddCharValue in AP.c
// for a hint see first half of AP_AddCharacteristic and first half of AP_AddNotifyCharacteristic
//****You implement this function as part of Lab 6*****
  uint8_t NPI_AddCharValue[14] = 
  {   
  SOF,0x08,0x00,  // length = 8
  0x35,0x82,      // SNP Add Characteristic Value Declaration
  0x00,           // 0=none,1=read,2=write, 3=Read+write, GATT Permission
  0x00,0x00,      // 2=read,8=write,0x0A=read+write,0x10=notify, GATT Properties
  0x00,           // RFU
  0x00,0x02,      // Maximum length of the attribute value=512
  0x00,0x00,      // UUID
  0x00            // FCS
  };
  NPI_AddCharValue[5] = permission; // 0=none,1=read,2=write, 3=Read+write, GATT Permission
  NPI_AddCharValue[6] = properties; // 2=read,8=write,0x0A=read+write,0x10=notify, GATT Properties
  NPI_AddCharValue[11] = 0xFF&uuid; //uuid
  NPI_AddCharValue[12] = uuid>>8;   //uuid
  SetFCS(NPI_AddCharValue);  //FCS
  for (uint8_t i=0; i<14; i++)
  {
    *msg = NPI_AddCharValue[i];
    msg++;
  }
}

//*************BuildAddCharDescriptorMsg**************
// Create a Add Characteristic Descriptor Declaration message, used in Lab 6
// Inputs name is a null-terminated string, maximum length of name is 20 bytes
//        pointer to empty buffer of at least 32 bytes
// Output none
// build the necessary NPI message that will add a Descriptor Declaration
void BuildAddCharDescriptorMsg(char name[], uint8_t *msg){
// set length and maxlength to the string length
// set the permissions on the string to read
// for a hint see NPI_AddCharDescriptor in AP.c
// for a hint see second half of AP_AddCharacteristic
//****You implement this function as part of Lab 6*****
  uint8_t NPI_AddCharDescriptor[] = {   
  SOF,0x17,0x00,  // length determined at run time 6+string length
  0x35,0x83,      // SNP Add Characteristic Descriptor Declaration
  0x80,           // User Description String
  0x01,           // GATT Read Permissions
  0x14,0x00,      // Maximum Possible length of the user description string
  0x14,0x00,      // Initial length of the user description string
  'C','h','a','r','a','c','t','e','r','i','s','t','i','c',' ','0',0, // Initial user description string
  0x00,0,0,0      // FCS
  }; 
  uint8_t i = 0;
  while((i<20)&&(name[i]))
  {
    NPI_AddCharDescriptor[11+i] = name[i]; 
    i++;
  }
  NPI_AddCharDescriptor[11+i] = 0; 
  i++;
  NPI_AddCharDescriptor[1] = 6+i;  // frame length
  NPI_AddCharDescriptor[5] = 0x80; // User Description String
  NPI_AddCharDescriptor[6] = 0x01; // GATT Read Permissions
  NPI_AddCharDescriptor[7] = NPI_AddCharDescriptor[9] = i;  // string length
  NPI_AddCharDescriptor[8] = NPI_AddCharDescriptor[10] = 0; // string length
  SetFCS(NPI_AddCharDescriptor);  // FCS
  for (uint8_t i=0; i<32; i++)
  {
    *msg = NPI_AddCharDescriptor[i];
    msg++;
  }
}

//*************Lab6_AddCharacteristic**************
// Add a read, write, or read/write characteristic, used in Lab 6
//        for notify properties, call AP_AddNotifyCharacteristic 
// Inputs uuid is 0xFFF0, 0xFFF1, ...
//        thesize is the number of bytes in the user data 1,2,4, or 8 
//        pt is a pointer to the user data, stored little endian
//        permission is GATT Permission, 0=none,1=read,2=write, 3=Read+write 
//        properties is GATT Properties, 2=read,8=write,0x0A=read+write
//        name is a null-terminated string, maximum length of name is 20 bytes
//        (*ReadFunc) called before it responses with data from internal structure
//        (*WriteFunc) called after it accepts data into internal structure
// Output APOK if successful,
//        APFAIL if name is empty, more than 8 characteristics, or if SNP failure
int Lab6_AddCharacteristic(uint16_t uuid, uint16_t thesize, void *pt, uint8_t permission,
  uint8_t properties, char name[], void(*ReadFunc)(void), void(*WriteFunc)(void)){
  int r; uint16_t handle; 
  uint8_t sendMsg[32];  
  if(thesize>8) return APFAIL;
  if(name[0]==0) return APFAIL;       // empty name
  if(CharacteristicCount>=MAXCHARACTERISTICS) return APFAIL; // error
  BuildAddCharValueMsg(uuid,permission,properties,sendMsg);
  OutString("\n\rAdd CharValue");
  r=AP_SendMessageResponse(sendMsg,RecvBuf,RECVSIZE);
  if(r == APFAIL) return APFAIL;
  handle = (RecvBuf[7]<<8)+RecvBuf[6]; // handle for this characteristic
  OutString("\n\rAdd CharDescriptor");
  BuildAddCharDescriptorMsg(name,sendMsg);
  r=AP_SendMessageResponse(sendMsg,RecvBuf,RECVSIZE);
  if(r == APFAIL) return APFAIL;
  CharacteristicList[CharacteristicCount].theHandle = handle;
  CharacteristicList[CharacteristicCount].size = thesize;
  CharacteristicList[CharacteristicCount].pt = (uint8_t *) pt;
  CharacteristicList[CharacteristicCount].callBackRead = ReadFunc;
  CharacteristicList[CharacteristicCount].callBackWrite = WriteFunc;
  CharacteristicCount++;
  return APOK; // OK
} 
  

//*************BuildAddNotifyCharDescriptorMsg**************
// Create a Add Notify Characteristic Descriptor Declaration message, used in Lab 6
// Inputs name is a null-terminated string, maximum length of name is 20 bytes
//        pointer to empty buffer of at least bytes
// Output none
// build the necessary NPI message that will add a Descriptor Declaration
void BuildAddNotifyCharDescriptorMsg(char name[], uint8_t *msg){
// set length and maxlength to the string length
// set the permissions on the string to read
// set User Description String
// set CCCD parameters read+write
// for a hint see NPI_AddCharDescriptor4 in VerySimpleApplicationProcessor.c
// for a hint see second half of AP_AddNotifyCharacteristic
//****You implement this function as part of Lab 6*****
  uint8_t NPI_AddCharDescriptor4[] = 
  {   
  SOF,12,0x00,    // length = 12
  0x35,0x83,      // SNP Add Characteristic Descriptor Declaration
  0x84,           // User Description String+CCCD
  0x03,           // CCCD parameters read+write
  0x01,           // GATT Read Permissions
  0x14,0x00,      // Maximum Possible length of the user description string
  0x14,0x00,      // Initial length of the user description string
  'C','o','u','n','t',0, // Initial user description string
  0x00};          // FCS
  uint8_t i = 0;
  while((i<20)&&(name[i]))
  {
    NPI_AddCharDescriptor4[12+i] = name[i]; 
    i++;
  }
  NPI_AddCharDescriptor4[12+i] = 0; 
  i++;
  NPI_AddCharDescriptor4[1] = 7+i;  // frame length
  NPI_AddCharDescriptor4[5] = 0x84; // User Description String
  NPI_AddCharDescriptor4[6] = 0x03, // CCCD parameters read+write
  NPI_AddCharDescriptor4[7] = 0x01; // GATT Read Permissions
  NPI_AddCharDescriptor4[8] = NPI_AddCharDescriptor4[10] = i;  // string length
  NPI_AddCharDescriptor4[9] = NPI_AddCharDescriptor4[11] = 0; // string length
  SetFCS(NPI_AddCharDescriptor4);  // FCS
  for (uint8_t i=0; i<(i+(NPI_AddCharDescriptor4[1]-i)+5); i++)
  {
    *msg = NPI_AddCharDescriptor4[i];
    msg++;
  }
}
  
//*************Lab6_AddNotifyCharacteristic**************
// Add a notify characteristic, used in Lab 6
//        for read, write, or read/write characteristic, call AP_AddCharacteristic 
// Inputs uuid is 0xFFF0, 0xFFF1, ...
//        thesize is the number of bytes in the user data 1,2,4, or 8 
//        pt is a pointer to the user data, stored little endian
//        name is a null-terminated string, maximum length of name is 20 bytes
//        (*CCCDfunc) called after it accepts , changing CCCDvalue
// Output APOK if successful,
//        APFAIL if name is empty, more than 4 notify characteristics, or if SNP failure
int Lab6_AddNotifyCharacteristic(uint16_t uuid, uint16_t thesize, void *pt,   
  char name[], void(*CCCDfunc)(void)){
  int r; uint16_t handle; 
  uint8_t sendMsg[36];  
  if(thesize>8) return APFAIL;
  if(NotifyCharacteristicCount>=NOTIFYMAXCHARACTERISTICS) return APFAIL; // error
  BuildAddCharValueMsg(uuid,0,0x10,sendMsg);
  OutString("\n\rAdd Notify CharValue");
  r=AP_SendMessageResponse(sendMsg,RecvBuf,RECVSIZE);
  if(r == APFAIL) return APFAIL;
  handle = (RecvBuf[7]<<8)+RecvBuf[6]; // handle for this characteristic
  OutString("\n\rAdd CharDescriptor");
  BuildAddNotifyCharDescriptorMsg(name,sendMsg);
  r=AP_SendMessageResponse(sendMsg,RecvBuf,RECVSIZE);
  if(r == APFAIL) return APFAIL;
  NotifyCharacteristicList[NotifyCharacteristicCount].uuid = uuid;
  NotifyCharacteristicList[NotifyCharacteristicCount].theHandle = handle;
  NotifyCharacteristicList[NotifyCharacteristicCount].CCCDhandle = (RecvBuf[8]<<8)+RecvBuf[7]; // handle for this CCCD
  NotifyCharacteristicList[NotifyCharacteristicCount].CCCDvalue = 0; // notify initially off
  NotifyCharacteristicList[NotifyCharacteristicCount].size = thesize;
  NotifyCharacteristicList[NotifyCharacteristicCount].pt = (uint8_t *) pt;
  NotifyCharacteristicList[NotifyCharacteristicCount].callBackCCCD = CCCDfunc;
  NotifyCharacteristicCount++;
  return APOK; // OK
}

//*************BuildSetDeviceNameMsg**************
// Create a Set GATT Parameter message, used in Lab 6
// Inputs name is a null-terminated string, maximum length of name is 24 bytes
//        pointer to empty buffer of at least 36 bytes
// Output none
// build the necessary NPI message to set Device name
void BuildSetDeviceNameMsg(char name[], uint8_t *msg){
// for a hint see NPI_GATTSetDeviceNameMsg in VerySimpleApplicationProcessor.c
// for a hint see NPI_GATTSetDeviceName in AP.c
//****You implement this function as part of Lab 6*****
  uint8_t NPI_GATTSetDeviceNameMsg[] = {   
  SOF,18,0x00,    // length = 18
  0x35,0x8C,      // SNP Set GATT Parameter (0x8C)
  0x01,           // Generic Access Service
  0x00,0x00,      // Device Name
  'S','h','a','p','e',' ','t','h','e',' ','W','o','r','l','d',
  0x00};          // FCS
  uint8_t i = 0;
  while((i<24)&&(name[i]))
  {
    NPI_GATTSetDeviceNameMsg[8+i] = name[i]; 
    i++;
  }
  NPI_GATTSetDeviceNameMsg[1] = 3+i;  // frame length
  SetFCS(NPI_GATTSetDeviceNameMsg);
  for (uint8_t i=0; i<36; i++)
  {
    *msg = NPI_GATTSetDeviceNameMsg[i];
    msg++;
  }
}
//*************BuildSetAdvertisementData1Msg**************
// Create a Set Advertisement Data message, used in Lab 6
// Inputs pointer to empty buffer of at least 16 bytes
// Output none
// build the necessary NPI message for Non-connectable Advertisement Data
void BuildSetAdvertisementData1Msg(uint8_t *msg){
// for a hint see NPI_SetAdvertisementMsg in VerySimpleApplicationProcessor.c
// for a hint see NPI_SetAdvertisement1 in AP.c
// Non-connectable Advertisement Data
// GAP_ADTYPE_FLAGS,DISCOVERABLE | no BREDR  
// Texas Instruments Company ID 0x000D
// TI_ST_DEVICE_ID = 3
// TI_ST_KEY_DATA_ID
// Key state=0
//****You implement this function as part of Lab 6*****
  uint8_t NPI_SetAdvertisementMsg[] = {   
  SOF,11,0x00,    // length = 11
  0x55,0x43,      // SNP Set Advertisement Data
  0x01,           // Not connected Advertisement Data
  0x02,0x01,0x06, // GAP_ADTYPE_FLAGS,DISCOVERABLE | no BREDR
  0x06,0xFF,      // length, manufacturer specific
  0x0D ,0x00,     // Texas Instruments Company ID
  0x03,           // TI_ST_DEVICE_ID
  0x00,           // TI_ST_KEY_DATA_ID
  0x00,           // Key state
  0x00};          // FCS
  SetFCS(NPI_SetAdvertisementMsg);
  for (uint8_t i=0; i<16; i++)
  {
    *msg = NPI_SetAdvertisementMsg[i];
    msg++;
  }
  
}

//*************BuildSetAdvertisementDataMsg**************
// Create a Set Advertisement Data message, used in Lab 6
// Inputs name is a null-terminated string, maximum length of name is 24 bytes
//        pointer to empty buffer of at least 36 bytes
// Output none
// build the necessary NPI message for Scan Response Data
void BuildSetAdvertisementDataMsg(char name[], uint8_t *msg){
// for a hint see NPI_SetAdvertisementDataMsg in VerySimpleApplicationProcessor.c
// for a hint see NPI_SetAdvertisementData in AP.c
//****You implement this function as part of Lab 6*****
  uint8_t NPI_SetAdvertisementDataMsg[] = {   
  SOF,27,0x00,    // length = 27
  0x55,0x43,      // SNP Set Advertisement Data
  0x00,           // Scan Response Data
  16,0x09,        // length, type=LOCAL_NAME_COMPLETE
  'S','h','a','p','e',' ','t','h','e',' ','W','o','r','l','d',
// connection interval range
  0x05,           // length of this data
  0x12,           // GAP_ADTYPE_SLAVE_CONN_INTERVAL_RANGE
  0x50,0x00,      // DEFAULT_DESIRED_MIN_CONN_INTERVAL
  0x20,0x03,      // DEFAULT_DESIRED_MAX_CONN_INTERVAL
// Tx power level
  0x02,           // length of this data
  0x0A,           // GAP_ADTYPE_POWER_LEVEL
  0x00,           // 0dBm
  0x00};          // FCS 
  uint8_t i = 0;
  while((i<24)&&(name[i]))
  {
    NPI_SetAdvertisementDataMsg[8+i] = name[i]; 
    i++;
  }
  NPI_SetAdvertisementDataMsg[1] = 12 + i;  // frame length
  NPI_SetAdvertisementDataMsg[6] = i + 1;  // length of data message
  SetFCS(NPI_SetAdvertisementDataMsg);
  for (uint8_t i=0; i<36; i++)
  {
    *msg = NPI_SetAdvertisementDataMsg[i];
    msg++;
  }
}
//*************BuildStartAdvertisementMsg**************
// Create a Start Advertisement Data message, used in Lab 6
// Inputs advertising interval
//        pointer to empty buffer of at least 20 bytes
// Output none
// build the necessary NPI message to start advertisement
void BuildStartAdvertisementMsg(uint16_t interval, uint8_t *msg){
// for a hint see NPI_StartAdvertisementMsg in VerySimpleApplicationProcessor.c
// for a hint see NPI_StartAdvertisement in AP.c
//****You implement this function as part of Lab 6*****
  uint8_t NPI_StartAdvertisementMsg[] = {   
  SOF,14,0x00,    // length = 14
  0x55,0x42,      // SNP Start Advertisement
  0x00,           // Connectable Undirected Advertisements
  0x00,0x00,      // Advertise infinitely.
  0x64,0x00,      // Advertising Interval (100 * 0.625 ms=62.5ms)
  0x00,           // Filter Policy RFU
  0x00,           // Initiator Address Type RFU
  0x00,0x01,0x00,0x00,0x00,0xC5, // RFU
  0x02,           // Advertising will restart with connectable advertising when a connection is terminated
  0x00};          // FCS
  NPI_StartAdvertisementMsg[8] = interval&0xFF;
  NPI_StartAdvertisementMsg[9] = interval>>8;
  SetFCS(NPI_StartAdvertisementMsg);
  for (uint8_t i=0; i<20; i++)
  {
    *msg = NPI_StartAdvertisementMsg[i];
    msg++;
  }
}

//*************Lab6_StartAdvertisement**************
// Start advertisement, used in Lab 6
// Input:  none
// Output: APOK if successful,
//         APFAIL if notification not configured, or if SNP failure
int Lab6_StartAdvertisement(void){volatile int r; uint8_t sendMsg[40];
  OutString("\n\rSet Device name");
  BuildSetDeviceNameMsg("Shape the World",sendMsg);
  r =AP_SendMessageResponse(sendMsg,RecvBuf,RECVSIZE);
  OutString("\n\rSetAdvertisement1");
  BuildSetAdvertisementData1Msg(sendMsg);
  r =AP_SendMessageResponse(sendMsg,RecvBuf,RECVSIZE);
  OutString("\n\rSetAdvertisement Data");
  BuildSetAdvertisementDataMsg("Shape the World",sendMsg);
  r =AP_SendMessageResponse(sendMsg,RecvBuf,RECVSIZE);
  OutString("\n\rStartAdvertisement");
  BuildStartAdvertisementMsg(100,sendMsg);
  r =AP_SendMessageResponse(sendMsg,RecvBuf,RECVSIZE);
  return r;
}

