#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9340.h"
#include <Wire.h>
#include <SPI.h>

//Array to store one Lepton frame
byte leptonFrame[164];
//Array to store 80 x 60 RAW14 pixels
uint16_t rawValues[4800];
uint16_t normalized_rawValues[4800];

//I don't know what this does, but if it's still here, it's only because it broke when I removed it
#if defined(__SAM3X8E__)
    #undef __FlashStringHelper::F(string_literal)
    #define F(string_literal) string_literal
#endif

//Define the CS port for your FLIR Lepton here
#define Lepton_CS 10//pulled from working code as 10, but I remember 10 being the right one.  Check this on the circuit.

//Define the pins for the ILI9340 TFT here
#define _sclk 13
#define _miso 12
#define _mosi 11
#define _cs 15
#define _dc 9
#define _rst 8

// Using software SPI is really not suggested, its incredibly slow
//Adafruit_ILI9340 tft = Adafruit_ILI9340(_cs, _dc, _mosi, _sclk, _rst, _miso);
// Use hardware SPI
Adafruit_ILI9340 tft = Adafruit_ILI9340(_cs, _dc, _rst);

/* SPI and Serial debug setup */
void setup() {
  
  tft.begin();
  
  //Also set all other SPI devices CS lines to Output and High here
  pinMode(Lepton_CS, OUTPUT);
  digitalWrite(Lepton_CS, HIGH);
  //Start SPI
  SPI.begin();
  //Start UART
  Serial.begin(115200);
  getTemperatures();
  printValues();
  //int color;

  //for(int i=0;i<60;i++){
  //  for(int j=0;j<80;j++){
  //    color = 10 * rawValues[(i*80) + j]-4000;
  //    tft.drawPixel(i, j, color);
  //  }
  //}
}

/* Main loop, get temperatures and print them */
void loop() {
    int color;
    getTemperatures();
    //printValues();
    for(int i=0;i<60;i++){
      for(int j=0;j<80;j++){
        color = tft.Color565((rawValues[(i*80) + j]-0)/4, 0, 0);
        //tft.drawPixel(i, -j, color);
        tft.fillRect(2*i, 2*(80-j), 2, 2, color);
        }
      }  
    
}


/* Start Lepton SPI Transmission */
void beginLeptonSPI() {
  //Begin SPI Transaction on alternative Clock
  SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
  //Start transfer  - CS LOW
  digitalWriteFast(Lepton_CS, LOW);

}

/* End Lepton SPI Transmission */
void endLeptonSPI() {
  //End transfer - CS HIGH
  digitalWriteFast(Lepton_CS, HIGH);
  //End SPI Transaction
  SPI.endTransaction();
}

/* Print out the 80 x 60 raw values array for every complete image */
void printValues(){
  for(int i=0;i<60;i++){
    for(int j=0;j<80;j++){
      Serial.print(rawValues[(i*80) + j]);
      Serial.print(" ");
      //tft.drawPixel(i, j, 7000);
    }
    Serial.println();
  }
}

/* Reads one line (164 Bytes) from the lepton over SPI */
boolean leptonReadFrame(uint8_t line) {

  bool success = true;
  //Receive one frame over SPI
  SPI.transfer(leptonFrame, 164);
  //Check for success
  if ((leptonFrame[0] & 0x0F) == 0x0F) {
    success = false;
  }
  else if (leptonFrame[1] != line) {
    success = false;
  }
  return success;
}

/* Get one image from the Lepton module */
void getTemperatures() {
  byte leptonError = 0;
  byte line;

  int color;
  
  //Begin SPI Transmission
  beginLeptonSPI();
  do {
    leptonError = 0;
    for (line = 0; line < 60; line++) {
      //Reset if the expected line does not match the answer
      if (!leptonReadFrame(line)) {
        //Reset line to -1, will be zero in the next cycle
        line = -1;
        //Raise Error count
        leptonError++;
        //Little delay
        delay(1);
        //If the Error count is too high, reset the device
        if (leptonError > 100) {
          //Re-Sync the Lepton
          endLeptonSPI();
          delay(186);
          beginLeptonSPI();
          break;
        }
      }
      //If line matches answer, save the packet
      else {
        //Go through the video pixels for one video line
        for (int column = 0; column < 80; column++) {
          uint16_t result = (uint16_t)(leptonFrame[2 * column + 4] << 8
                                       | leptonFrame[2 * column + 5]);
          //Save to array
          rawValues[column + (line * 80)] = result;
        }
      }
    }
  //} while ((leptonError > 100) || (line != 60));
  } while ((line != 60));
  //End Lepton SPI
  endLeptonSPI();
}
