
#include <SPIFFS.h>
#include <Wire.h>
#include <Arduino.h>
#include <Adafruit_I2CDevice.h>
#include <FS.h>
#include <WiFi.h>
#include <BLEHIDDevice.h>
#include "FS.h"
#include <SPI.h>
#include <SD.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

#define CALIBRATION_FILE "/calibrationData1"

#define SD_MOSI 23
#define SD_MISO 19
#define SD_SCLK 18
#define SD_CS 5

File myFile;

void displaySDContentOnTFT(File root, int numTabs)
{
  while (true)
  {
    File file = root.openNextFile();
    if (!file)
    {
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++)
    {
      Serial.print('\t');
    }
    if (file.isDirectory())
    {
      Serial.print("DIR : ");
      Serial.println(file.name());
      displaySDContentOnTFT(file, numTabs + 1);
    }
    else
    {
      Serial.print("FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file.close();
  }
}

void setup(void)
{
  uint16_t calibrationData[5];
  uint8_t calDataOK = 0;

  Serial.begin(115200);
  Serial.println("starting");

  SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  SD.begin(5);


  myFile = SD.open("/");

  tft.init();

  tft.setRotation(3);
  tft.fillScreen((0xFFFF));

  tft.setCursor(20, 0, 2);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.setTextSize(1);
  tft.println("calibration run");

  // check file system
  if (!SPIFFS.begin())
  {
    Serial.println("formatting file system");

    SPIFFS.format();
    SPIFFS.begin();
  }

  // check if calibration file exists
  if (SPIFFS.exists(CALIBRATION_FILE))
  {
    File f = SPIFFS.open(CALIBRATION_FILE, "r");
    if (f)
    {
      if (f.readBytes((char *)calibrationData, 14) == 14)
        calDataOK = 1;
      f.close();
    }
  }
  if (calDataOK)
  {
    // calibration data valid
    tft.setTouch(calibrationData);
  }
  else
  {
    // data not valid. recalibrate
    tft.calibrateTouch(calibrationData, TFT_WHITE, TFT_RED, 15);
    // store data
    File f = SPIFFS.open(CALIBRATION_FILE, "w");
    if (f)
    {
      f.write((const unsigned char *)calibrationData, 14);
      f.close();
    }
  }

  tft.fillScreen((0xFFFF));

  tft.fillRect(0, 0, 30, 30, TFT_RED);
}

// drawing using touch
// storing points in array
// drawing lines between points but also only those points which correspond to a particular stroke

// also we display the directory of contents in sd card

void loop(void)
{

  displaySDContentOnTFT(myFile, 0);

  uint16_t x, y;
  uint16_t lastX = 0;
  uint16_t lastY = 0;
  uint16_t stroke = 0;
  uint16_t strokeColor[3] = {TFT_RED, TFT_GREEN, TFT_BLUE};
  uint16_t strokeCount = 0;
  uint16_t strokeStart[3] = {0, 0, 0};
  uint16_t strokeEnd[3] = {0, 0, 0};
  uint16_t strokePoint[3][100];
  uint16_t strokePointCount[3] = {0, 0, 0};

  while (1)
  {
    if (tft.getTouch(&x, &y))
    {
      if (x < 40 && y < 40)
      {
        // clear screen
        tft.fillScreen((0xFFFF));
        strokeCount = 0;
        stroke = 0;
        strokePointCount[0] = 0;
        strokePointCount[1] = 0;
        strokePointCount[2] = 0;
        tft.fillRect(0, 0, 30, 30, TFT_RED);
        continue;
      }
      if (lastX != 0)
      {
        tft.drawLine(lastX, lastY, x, y, strokeColor[stroke]);
        strokeEnd[stroke] = strokePointCount[stroke];
        strokePoint[stroke][strokePointCount[stroke]++] = x;
        strokePoint[stroke][strokePointCount[stroke]++] = y;
      }
      else
      {
        strokeStart[stroke] = strokePointCount[stroke];
        strokePoint[stroke][strokePointCount[stroke]++] = x;
        strokePoint[stroke][strokePointCount[stroke]++] = y;
      }
      lastX = x;
      lastY = y;
    }
    else
    {
      if (lastX != 0)
      {
        strokeCount++;
        stroke++;
        lastX = 0;
        lastY = 0;
      }
    }
  }
}