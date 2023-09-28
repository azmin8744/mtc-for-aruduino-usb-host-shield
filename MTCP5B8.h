
#include "Usb.h"
#include "usbhid.h"

#define MTCP5B8_VID    0x0AE4
#define MTCP5B8_PID    0x0004
#define MTCP5B8_MAX_ENDPOINTS 4
#define EP_MAXPKTSIZE           6 // max size for data via USB

/* Names we give to the 3 MTCP5B8 pipes - this is only used for setting the bluetooth address into the MTCP5B8 controllers */
#define MTCP5B8_CONTROL_PIPE        0
#define MTCP5B8_OUTPUT_PIPE         1
#define MTCP5B8_INPUT_PIPE          2

enum HandlePositionEnum {
  HANDLE_EB = 0x81B9,
  HANDLE_B8 = 0x81B5,
  HANDLE_B7 = 0x81B2,
  HANDLE_B6 = 0x81AF,
  HANDLE_B5 = 0x81A8,
  HANDLE_B4 = 0x81A2,
  HANDLE_B3 = 0x819A,
  HANDLE_B2 = 0x8194,
  HANDLE_B1 = 0x818A,
  HANDLE_NEUTRAL = 0x8179,
  HANDLE_P1 = 0x6D79,
  HANDLE_P2 = 0x5479,
  HANDLE_P3 = 0x3F79,
  HANDLE_P4 = 0x2179,
  HANDLE_P5 = 0x0079,
};

enum buttonEnum {
  BUTTON_HAT = 0xFF00,
  BUTTON_B = 0x01,
  BUTTON_A = 0x02,
  BUTTON_C = 0x04,
  BUTTON_D = 0x08,
  BUTTON_START = 0x20,
  BUTTON_SELECT = 0x10,
};

enum hatEnum {
  HAT_UP = 0,
  HAT_RIGHT = 2,
  HAT_DOWN = 4,
  HAT_LEFT = 6,
  HAT_CENTER = 8,
};

struct MTCDataButtons {
  uint8_t hat;
  bool select;
  bool start;
  bool buttonA;
  bool buttonB;
  bool buttonC;
  bool buttonD;
} __attribute__((packed));

union MTCData {
  struct {
    uint8_t dummy;
    uint16_t handlePosition;
    uint8_t padding;
    uint8_t hatState;
    uint16_t buttonState;
  } __attribute__((packed));
  uint32_t val : 12;
} __attribute__((packed));

class MTCP5B8 : public USBDeviceConfig {
  public:

  /**
  * Constructor for the MTCP5B8 class.
  * @param  pUsb   Pointer to USB class instance.
  */
  MTCP5B8(USB *pUsb);

  /** @name USBDeviceConfig implementation */
  /**
    * Initialize the MTCP5B8 Controller.
    * @param  parent   Hub number.
    * @param  port     Port number on the hub.
    * @param  lowspeed Speed of the device.
    * @return          0 on success.
    */
  uint8_t Init(uint8_t parent, uint8_t port, bool lowspeed);
  /**
    * Release the USB device.
    * @return 0 on success.
    */
  uint8_t Release();
  /**
    * Poll the USB Input endpoins and run the state machines.
    * @return 0 on success.
    */
  uint8_t Poll();

  /**
    * Get the device address.
    * @return The device address.
    */
  virtual uint8_t GetAddress() {
          return bAddress;
  };

  /**
    * Used to check if the controller has been initialized.
    * @return True if it's ready.
    */
  virtual bool isReady() {
          return bPollEnable;
  };

  /**
    * Used by the USB core to check what this driver support.
    * @param  vid The device's VID.
    * @param  pid The device's PID.
    * @return     Returns true if the device's VID and PID matches this driver.
    */
  virtual bool VIDPIDOK(uint16_t vid, uint16_t pid) {
          return (vid == MTCP5B8_VID && pid == MTCP5B8_PID);
  };


  /** Variable used to indicate if the P5B8 controller is successfully connected. */
  bool P5B8Connected;
  MTCData mtcData;
  MTCDataButtons buttonClickState;
  bool validHandlePosition();

  protected:
  /** Pointer to USB class instance. */
  USB *pUsb;
  /** Device address. */
  uint8_t bAddress;
  /** Endpoint info structure. */
  EpInfo epInfo[MTCP5B8_MAX_ENDPOINTS];

  private:
  /**
    * Called when the controller is successfully initialized.
    * Use attachOnInit(void (*funcOnInit)(void)) to call your own function.
    * This is useful for instance if you want to set the LEDs in a specific way.
    */
  void onInit();
  void (*pFuncOnInit)(void); // Pointer to function called in onInit()

  bool bPollEnable;
  uint32_t timer;

  MTCData oldMtcData;
  uint8_t readBuf[EP_MAXPKTSIZE]; // General purpose buffer for input data
  uint8_t writeBuf[EP_MAXPKTSIZE]; // General purpose buffer for output data

  void readReport(); // read incoming data
  void printReport(); // print incoming date - Uncomment for debugging
  
  void mapButtonState();
};
