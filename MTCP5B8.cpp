#include "MTCP5B8.h"

MTCP5B8::MTCP5B8(USB *p) :
pUsb(p), // pointer to USB class instance - mandatory
bAddress(0), // device address - mandatory
bPollEnable(false) // don't start polling before dongle is connected
{
    if(pUsb) // register in USB subsystem
    pUsb->RegisterDeviceClass(this); //set devConfig[] entry
}

uint8_t MTCP5B8::Init(uint8_t parent, uint8_t port, bool lowspeed) {
  uint8_t buf[sizeof (USB_DEVICE_DESCRIPTOR)];
  USB_DEVICE_DESCRIPTOR * udd = reinterpret_cast<USB_DEVICE_DESCRIPTOR*>(buf);
  uint8_t rcode;
  UsbDevice *p = NULL;
  EpInfo *oldep_ptr = NULL;
  uint16_t PID;
  uint16_t VID;

  
  // get memory address of USB device address pool
  AddressPool &addrPool = pUsb->GetAddressPool();
#ifdef EXTRADEBUG
  Notify(PSTR("\r\nMTC P5B8 USB Init"), 0x80);
#endif
  // check if address has already been assigned to an instance
  if(bAddress) {
#ifdef DEBUG_USB_HOST
          Notify(PSTR("\r\nAddress in use"), 0x80);
#endif
          return USB_ERROR_CLASS_INSTANCE_ALREADY_IN_USE;
  }

  // Get pointer to pseudo device with address 0 assigned
  p = addrPool.GetUsbDevicePtr(0);

  if(!p) {
#ifdef DEBUG_USB_HOST
          Notify(PSTR("\r\nAddress not found"), 0x80);
#endif
          return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;
  }

  if(!p->epinfo) {
#ifdef DEBUG_USB_HOST
          Notify(PSTR("\r\nepinfo is null"), 0x80);
#endif
          return USB_ERROR_EPINFO_IS_NULL;
  }

  // Save old pointer to EP_RECORD of address 0
  oldep_ptr = p->epinfo;

  // Temporary assign new pointer to epInfo to p->epinfo in order to avoid toggle inconsistence
  p->epinfo = epInfo;

  p->lowspeed = lowspeed;

  // Get device descriptor
  rcode = pUsb->getDevDescr(0, 0, sizeof (USB_DEVICE_DESCRIPTOR), (uint8_t*)buf); // Get device descriptor - addr, ep, nbytes, data
  // Restore p->epinfo
  p->epinfo = oldep_ptr;

  if(rcode) {
        Notify(PSTR("\r\nMTC P5B8 Init Failed, error code: "), 0x80);
        NotifyFail(rcode);
        Release();
        return rcode;
  }

  VID = udd->idVendor;
  PID = udd->idProduct;

  // Allocate new address according to device class
  bAddress = addrPool.AllocAddress(parent, false, port);

  if(!bAddress)
          return USB_ERROR_OUT_OF_ADDRESS_SPACE_IN_POOL;

  // Extract Max Packet Size from device descriptor
  epInfo[0].maxPktSize = udd->bMaxPacketSize0;

  // Assign new address to the device
  rcode = pUsb->setAddr(0, 0, bAddress);
  if(rcode) {
          p->lowspeed = false;
          addrPool.FreeAddress(bAddress);
          bAddress = 0;
#ifdef DEBUG_USB_HOST
          Notify(PSTR("\r\nsetAddr: "), 0x80);
          D_PrintHex<uint8_t > (rcode, 0x80);
#endif
          return rcode;
  }
  #ifdef EXTRADEBUG
        Notify(PSTR("\r\nAddr: "), 0x80);
        D_PrintHex<uint8_t > (bAddress, 0x80);
#endif
  //delay(300); // Spec says you should wait at least 200ms

  p->lowspeed = false;

  //get pointer to assigned address record
  p = addrPool.GetUsbDevicePtr(bAddress);
  if(!p)
          return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;

  p->lowspeed = lowspeed;

  // Assign epInfo to epinfo pointer - only EP0 is known
  rcode = pUsb->setEpInfoEntry(bAddress, 1, epInfo);
  if(rcode)
          goto FailSetDevTblEntry;


  /* The application will work in reduced host mode, so we can save program and data
      memory space. After verifying the PID and VID we will use known values for the
      configuration values for device, interface, endpoints and HID for the MTCP5B8 Controllers */

  /* Initialize data structures for endpoints of device */
  epInfo[ MTCP5B8_OUTPUT_PIPE ].epAddr = 0x02; // MTCP5B8 output endpoint
  epInfo[ MTCP5B8_OUTPUT_PIPE ].epAttribs = USB_TRANSFER_TYPE_INTERRUPT;
  epInfo[ MTCP5B8_OUTPUT_PIPE ].bmNakPower = USB_NAK_NOWAIT; // Only poll once for interrupt endpoints
  epInfo[ MTCP5B8_OUTPUT_PIPE ].maxPktSize = EP_MAXPKTSIZE;
  epInfo[ MTCP5B8_OUTPUT_PIPE ].bmSndToggle = 0;
  epInfo[ MTCP5B8_OUTPUT_PIPE ].bmRcvToggle = 0;
  epInfo[ MTCP5B8_INPUT_PIPE ].epAddr = 0x01; // MTCP5B8 report endpoint
  epInfo[ MTCP5B8_INPUT_PIPE ].epAttribs = USB_TRANSFER_TYPE_INTERRUPT;
  epInfo[ MTCP5B8_INPUT_PIPE ].bmNakPower = USB_NAK_NOWAIT; // Only poll once for interrupt endpoints
  epInfo[ MTCP5B8_INPUT_PIPE ].maxPktSize = EP_MAXPKTSIZE;
  epInfo[ MTCP5B8_INPUT_PIPE ].bmSndToggle = 0;
  epInfo[ MTCP5B8_INPUT_PIPE ].bmRcvToggle = 0;

  rcode = pUsb->setEpInfoEntry(bAddress, 3, epInfo);
  if(rcode)
          goto FailSetDevTblEntry;

  delay(200); //Give time for address change

  rcode = pUsb->setConf(bAddress, epInfo[ MTCP5B8_CONTROL_PIPE ].epAddr, 1);
  if(rcode)
    goto FailSetConfDescr;

  if(PID == MTCP5B8_PID) {
#ifdef DEBUG_USB_HOST
    Notify(PSTR("\r\nMulti Train Controller P5B8 Connected"), 0x80);
#endif
    P5B8Connected = true;
  }
  onInit();

  bPollEnable = true;
  Notify(PSTR("\r\n"), 0x80);
  timer = (uint32_t)millis();
  return 0; // Successful configuration

  FailGetDevDescr:
#ifdef DEBUG_USB_HOST
  NotifyFailGetDevDescr();
  goto Fail;
#endif

FailSetDevTblEntry:
#ifdef DEBUG_USB_HOST
  NotifyFailSetDevTblEntry();
  goto Fail;
#endif

FailSetConfDescr:
#ifdef DEBUG_USB_HOST
  NotifyFailSetConfDescr();
#endif
  goto Fail;
Fail:
#ifdef DEBUG_USB_HOST
  Notify(PSTR("\r\nMTC P5B8 Init Failed, error code: "), 0x80);
  NotifyFail(rcode);
#endif
  Release();
  return rcode;
}
/* Performs a cleanup after failed Init() attempt */
uint8_t MTCP5B8::Release() {
        P5B8Connected = false;
        pUsb->GetAddressPool().FreeAddress(bAddress);
        bAddress = 0;
        bPollEnable = false;
        return 0;
}

uint8_t MTCP5B8::Poll() {
  if(!bPollEnable)
    return 0;
  if(P5B8Connected) {

    uint16_t BUFFER_SIZE = EP_MAXPKTSIZE;
    pUsb->inTransfer(bAddress, epInfo[ MTCP5B8_INPUT_PIPE ].epAddr, &BUFFER_SIZE, readBuf); // input on endpoint 1
    
    if((int32_t)((uint32_t)millis() - timer) > 100) { // Loop 100ms before processing data
      readReport();
#ifdef PRINTREPORT
      printReport(); // Uncomment "#define PRINTREPORT" to print the report send by the MTCP5B8 Controllers
#endif
    }
  }
  return 0;
}

void MTCP5B8::readReport() {
  //Notify(PSTR("\r\nButtonState", 0x80);
  //PrintHex<uint32_t>(ButtonState, 0x80);

  memcpy(&mtcData, readBuf, min(EP_MAXPKTSIZE, sizeof(MTCData)));
  mapButtonState();
}

void MTCP5B8::printReport() { // Uncomment "#define PRINTREPORT" to print the report send by the MTCP5B8 Controllers
#ifdef PRINTREPORT
        for(uint8_t i = 0; i < EP_MAXPKTSIZE; i++) {
                D_PrintHex<uint8_t > (readBuf[i], 0x80);
                Notify(PSTR(" "), 0x80);
        }
        Notify(PSTR("\r\n"), 0x80);
#endif
}

void MTCP5B8::mapButtonState() {
  buttonClickState.hat = mtcData.hatState;
  buttonClickState.select = mtcData.buttonState & BUTTON_SELECT;
  buttonClickState.start = mtcData.buttonState & BUTTON_START;
  buttonClickState.buttonA = mtcData.buttonState & BUTTON_A;
  buttonClickState.buttonB = mtcData.buttonState & BUTTON_B;
  buttonClickState.buttonC = mtcData.buttonState & BUTTON_C;
  buttonClickState.buttonD = mtcData.buttonState & BUTTON_D;
}

bool MTCP5B8::validHandlePosition() {
  switch (mtcData.handlePosition) {
    case HANDLE_EB: return true;
    case HANDLE_B8: return true;
    case HANDLE_B7: return true;
    case HANDLE_B6: return true;
    case HANDLE_B5: return true;
    case HANDLE_B4: return true;
    case HANDLE_B3: return true;
    case HANDLE_B2: return true;
    case HANDLE_B1: return true;
    case HANDLE_NEUTRAL: return true;
    case HANDLE_P1: return true;
    case HANDLE_P2: return true;
    case HANDLE_P3: return true;
    case HANDLE_P4: return true;
    case HANDLE_P5: return true;
    default: return false;
  }
}

void MTCP5B8::onInit() {
  memcpy(&mtcData, readBuf, min(EP_MAXPKTSIZE, sizeof(MTCData)));
  mapButtonState();
  oldMtcData = mtcData;
}
