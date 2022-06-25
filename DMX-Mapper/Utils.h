#include <SD.h>
#include <SPI.h>
#include <CSV_Parser.h>
#include <EEPROM.h>

// Read int from file stream
// c: Pointer to char buffer
// skipFirstRead: Use data already in c for first cycle
// ignoreEOF: Don't return -1 if file ends
int readInt(File file, char *c, bool skipFirstRead, bool ignoreEOF = 0) {

  char s[10];
  memset(s, 0, sizeof(s));

  char *sp = s; // Movable pointer
  
  do {

    if (!skipFirstRead) {
      *c = file.read();
    }
    skipFirstRead = 0;

    *sp = *c;
    sp++;
    
    if (!file.available() && !ignoreEOF) {
      return -1;
    }
    
  } while (isDigit(*c));

  return atoi(s);
}

// Save LUT to EEPROM
void writeEEPROM(uint16_t *buf) {
  
  for (int i=0; i<513; i++) {
    EEPROM.update(i*2, buf[i] & 0xFF); // Store lower byte
    EEPROM.update(i*2+1, buf[i] >> 8); // Store upper byte
  }
  // Magic numbers
  EEPROM.update(0x0FFF, 0xBE);
  EEPROM.update(0x1000, 0xEF);
}

// Read LUT from EEPROM
void readEEPROM(uint16_t *buf) {

  // Check magic numbers
  if (EEPROM.read(0x0FFF) != 0xBE ||
      EEPROM.read(0x1000) != 0xEF) {
    return;
  }
  
  for (int i=0; i<513; i++) {
    buf[i]  = EEPROM.read(i*2);        // Read lower byte
    buf[i] |= EEPROM.read(i*2+1) << 8; // Read upper byte
  }
}

char messages[1024]; // Log buffer
int logLength = 0;   // Length of log buffer

// Write message to log buffer
// msg: Message to print
// type:
//   0: Warning
//   1: Error
//   2: Info
// lineNum: line number
void logMessage(char *msg, int lineNum = 0, int type = 0) {

  char msgFormatted[256];

  if (type < 2) {
    if (lineNum > 0) {
      sprintf(msgFormatted, "%s: Line %i: %s\n", type ? "ERROR" : "WARNING", lineNum, msg);
    } else {
      sprintf(msgFormatted, "%s: %s\n", type ? "ERROR" : "WARNING", msg);
    }
  } else {
    sprintf(msgFormatted, "%s\n", msg);
  }

  Serial.print(msgFormatted);

  if (logLength + strlen(msgFormatted) + 1 > sizeof(messages)) return;

  sprintf(messages+logLength, "%s", msgFormatted);
  
  logLength += strlen(msgFormatted) + 1;
}

// Flush logs to file
void writeLogs(char *filePath) {

  SD.remove(filePath);

  if (!logLength) return;

  File logFile = SD.open(filePath, FILE_WRITE);
  if (!logFile) {
    Serial.println("Warning: Unable to write logs");
    return;
  }

  logFile.write(messages, logLength);

  logFile.close();
}

void initMaps(uint16_t *buf) {
  for (int i=0; i<513; i++) {
    buf[i] = i;
  }
}
