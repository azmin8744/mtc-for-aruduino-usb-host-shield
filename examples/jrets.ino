// JR東日本トレインシミュレーター用に、マウス・キーボード出力を行うサンプルスケッチ
#include "../MTCP5B8.h"

#include <SPI.h>
#include <Keyboard.h>
#include <KeyboardLayout.h>
#include <Mouse.h>

USB Usb;
MTCP5B8 Mtc(&Usb);

static uint16_t prevHandlePosition;
static MTCDataButtons prevButtonState;
static bool hatPressed;
static uint8_t prevArrowKey;

#define DEBUG_USB_HOST
#define PRINTREPORT
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
#if !defined(__MIPSEL__)
  while (!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
#endif
  if (Usb.Init() == -1) {
    Serial.print(F("\r\nOSC did not start"));
    while (1); //halt
  }
  Serial.print(F("\r\nMulti Train Controller P5B8 USB Library Started"));
  
  prevHandlePosition = Mtc.mtcData.handlePosition;
  Mouse.begin();
  Keyboard.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  Usb.Task();

  if(Mtc.P5B8Connected) {
    
      if(Mtc.buttonClickState.buttonA != prevButtonState.buttonA) {
        Serial.println("\r\n Button A");
        handleKey(Mtc.buttonClickState.buttonA, KEY_RETURN);
        prevButtonState.buttonA = Mtc.buttonClickState.buttonA;
      }
      if(Mtc.buttonClickState.buttonB != prevButtonState.buttonB) {
        Serial.println("\r\n Button B");
        handleKey(Mtc.buttonClickState.buttonB, ' ');
        prevButtonState.buttonB = Mtc.buttonClickState.buttonB;
      }
      if(Mtc.buttonClickState.buttonC != prevButtonState.buttonC) {
        Serial.println("\r\n Button C");
        handleKey(Mtc.buttonClickState.buttonC, 'E');
        prevButtonState.buttonC = Mtc.buttonClickState.buttonC;
      }
      if(Mtc.buttonClickState.buttonD != prevButtonState.buttonD) {
        Serial.println("\r\n Button D");
        handleKey(Mtc.buttonClickState.buttonD, 'C');
        prevButtonState.buttonD = Mtc.buttonClickState.buttonD;
      }

      if(Mtc.buttonClickState.start != prevButtonState.start) {
        Serial.println("\r\n Button Start");
        handleKey(Mtc.buttonClickState.start, KEY_RETURN);
        prevButtonState.start = Mtc.buttonClickState.start;
      }
      if(Mtc.buttonClickState.select != prevButtonState.select) {
        Serial.println("\r\n Button Select");
        handleKey(Mtc.buttonClickState.select, KEY_ESC);
        prevButtonState.select = Mtc.buttonClickState.select;
      }

      if(Mtc.buttonClickState.hat != prevButtonState.hat) {
        PrintHex<uint8_t> (Mtc.buttonClickState.hat, 0x80);
        Serial.println("\r\n Hat input.");
        hatPressed = Mtc.buttonClickState.hat != HAT_CENTER;
        switch(Mtc.buttonClickState.hat) {
          case HAT_UP: prevArrowKey = KEY_UP_ARROW; break;
          case HAT_RIGHT: prevArrowKey = KEY_RIGHT_ARROW; break;
          case HAT_DOWN: prevArrowKey = KEY_DOWN_ARROW; break;
          case HAT_LEFT: prevArrowKey = KEY_LEFT_ARROW; break;
        }
        handleKey(hatPressed, prevArrowKey);
        prevButtonState.hat = Mtc.buttonClickState.hat;
      }

    if(Mtc.validHandlePosition() && prevHandlePosition != Mtc.mtcData.handlePosition) {
      if (Mtc.mtcData.handlePosition == HANDLE_NEUTRAL) {
        Serial.println("\r\n Handle neutral.");
        Mouse.click(MOUSE_MIDDLE);
      } else if (prevHandlePosition < Mtc.mtcData.handlePosition) {
        Serial.println("\r\n Brake added.");
        Mouse.move(0, 0, 1);
      } else {
        Serial.println("\r\n Brake removed.");
        Mouse.move(0, 0, -1);
      }
      prevHandlePosition = Mtc.mtcData.handlePosition;
    }
  }
}

void handleKey(bool state, uint8_t keyCode) {
  if(state) {
    Keyboard.press(keyCode);
  } else {
    Keyboard.release(keyCode);
  }
}
