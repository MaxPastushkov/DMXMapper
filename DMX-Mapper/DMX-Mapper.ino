#include <TeensyDMX.h>

#include "ParseLUT.h"

// Optional modification to allow detection of SD card.
// The SD card slot switch contact needs to be soldered to pin:
#define SD_SENSE 37

char mapPath[] = "dmx-mapper.csv";
char logPath[] = "dmx-mapper.log";

using namespace ::qindesign::teensydmx;

Receiver dmxRx{Serial1};
Sender dmxTx{Serial2};

uint8_t buf1[513];
uint8_t buf2[513];
uint16_t LUT[513];

bool checkSense = 0;
bool sdRemoved = 0;

void setup() {
  dmxRx.begin();
  dmxTx.begin();
  Serial.begin(9600);

  pinMode(SD_SENSE, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);

  memset(buf1, 0, sizeof(buf1));
  memset(buf2, 0, sizeof(buf2));
  memset(messages, 0, sizeof(messages));

  // Initialize maps
  initMaps(LUT);

  if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println("Warning: SD card removed");
    readEEPROM(LUT);
    return;
  }

  // Record if modification has been performed
  checkSense = !digitalRead(SD_SENSE);

  readFromSD(mapPath, 0, LUT);
  writeEEPROM(LUT);
  
  writeLogs(logPath);

  //digitalWrite(LED_BUILTIN, HIGH);
}

void loop() {

  // Check for possible re-insertion of SD card
  if (checkSense) {
    if (!digitalRead(SD_SENSE) && sdRemoved) {
      // SD card has been re-inserted
      Serial.println("SD card reinserted");
      initMaps(LUT);
      delay(500);
      SD.begin(BUILTIN_SDCARD);
      readFromSD(mapPath, 0, LUT);
      writeLogs(logPath);
    }
    sdRemoved = digitalRead(SD_SENSE);
  }
  
  int bytesRead = dmxRx.readPacket(buf1, 0, 513);
  if (bytesRead < 1) {
    return;
  }

  // Perform mapping
  for (int i=0; i<bytesRead; i++) {
    if (LUT[i] >= 1000) {
      buf2[i] = LUT[i]-1000; // Constant
    } else {
      buf2[i] = buf1[LUT[i]];
    }
  }

  dmxTx.set(0, buf2, 513);
}
