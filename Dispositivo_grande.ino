// Programación de arduino para Teensy 4.1
// MadMapper v 5
// Teensy 4.1
// WS2812 5v LED Strips 
// 11 universos 1.606 leds
// https://www.cumasystem.xyz

#include <SPI.h>
#include <FastLED.h>

#define MAD_LED_PACKET_HEADER 0xFF
#define MAD_LED_DATA 0xBE
#define MAD_LED_DETECTION 0xAE
#define MAD_LED_DETECTION_REPLY 0xEA
#define MAD_LED_PROTOCOL_VERSION 0x01

#define NUM_LEDS 1606

#define DATA_PIN 14

CRGB leds[NUM_LEDS];

char dataFrame[NUM_LEDS * 3];
int readingFrameOnLine = -1;
bool gotNewDataFrame = false;

enum State {
  State_WaitingNextPacket,
  State_GotPacketHeader,
  State_WaitingLineNumber,
  State_WaitingChannelCountByte1,
  State_WaitingChannelCountByte2,
  State_ReadingDmxFrame
};

State inputState = State_WaitingNextPacket;
unsigned int channelsLeftToRead = 0;
char* frameWritePtr = dataFrame;



void setup() {
  Serial.begin(921600);

  for (unsigned int i = 0; i < sizeof(dataFrame); i++) dataFrame[i] = 0;

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);

  Serial.print("Setup done");

} // fin Setup



void processByte(unsigned char currentByte) {

#ifdef DEBUG_MODE
  Serial.print("GOT BYTE: "); Serial.print(currentByte, HEX);
#endif

  if (currentByte == MAD_LED_PACKET_HEADER) {
    inputState = State_GotPacketHeader;
#ifdef DEBUG_MODE
    Serial.print("GOT PH ");
#endif
  } else if (inputState == State_WaitingNextPacket) {

  } else if (inputState == State_GotPacketHeader) {
    if (currentByte == MAD_LED_DETECTION) {
      Serial.write(MAD_LED_DETECTION_REPLY);
      Serial.write(MAD_LED_PROTOCOL_VERSION);
      inputState = State_WaitingNextPacket;
    } else if (currentByte == MAD_LED_DATA) {
      inputState = State_WaitingLineNumber;
#ifdef DEBUG_MODE
      Serial.print("GOT LD ");
#endif
    } else {
      inputState = State_WaitingNextPacket;
    }
  } else if (inputState == State_WaitingLineNumber) {
    if (currentByte > 0x7F) {
      inputState = State_WaitingNextPacket;
#ifdef DEBUG_MODE
      Serial.print("ErrLineNum: "); Serial.print(currentByte);
#endif
    } else {
      readingFrameOnLine = currentByte;
      inputState = State_WaitingChannelCountByte1;
#ifdef DEBUG_MODE
      Serial.print("GOT LN ");
#endif
    }
  } else if (inputState == State_WaitingChannelCountByte1) {
    if (currentByte > 0x7F) {
      inputState = State_WaitingNextPacket;
#ifdef DEBUG_MODE
      Serial.print("ErrChCNT1: "); Serial.print(currentByte);
#endif
    } else {
      channelsLeftToRead = currentByte;
      inputState = State_WaitingChannelCountByte2;
#ifdef DEBUG_MODE
      Serial.print("GOT CHC1 ");
#endif
    }
  } else if (inputState == State_WaitingChannelCountByte2) {
    if (currentByte > 0x7F) {
      inputState = State_WaitingNextPacket;
#ifdef DEBUG_MODE
      Serial.print("ErrChCNT2: "); Serial.print(currentByte);
#endif
    } else {
      channelsLeftToRead += (int(currentByte) << 7);
      if (channelsLeftToRead == 0) {
        inputState = State_WaitingNextPacket;
#ifdef DEBUG_MODE
        Serial.print("ErrChCNT=0");
#endif
      } else {
        frameWritePtr = dataFrame;
        inputState = State_ReadingDmxFrame;
#ifdef DEBUG_MODE
        Serial.print("GOT CHC2 ");
#endif
      }
    }
  } else if (inputState == State_ReadingDmxFrame) {
    *frameWritePtr++ = currentByte;
    channelsLeftToRead--;
    if (channelsLeftToRead == 0) {
      inputState = State_WaitingNextPacket;
      gotNewDataFrame = true;
#ifdef DEBUG_MODE
      Serial.print("GOT DATA ");
#endif
    }
  }
}  // fin processByte()



void loop() {

#ifdef JUST_TEST_LEDS
  Serial.println("Currently testing...");
  static int value = 254;
  value = (value + 1) % 254;
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB(value, value, value);
  }
#else

  int bytesRead = 0;
  while (Serial.available() > 0 && bytesRead < 30000) {
    processByte(Serial.read());
    bytesRead++;
  }

  if (gotNewDataFrame) {
    gotNewDataFrame = false;
    char* dataPtr = dataFrame;

    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CRGB(dataPtr[0], dataPtr[1], dataPtr[2]);
      dataPtr += 3;
    }

  }
#endif

  FastLED.show();

} // fin Loop
