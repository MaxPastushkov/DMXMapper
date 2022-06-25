#include "Utils.h"

#define MAX_CSV_SIZE 4096

void readCSV(File mapFile, uint16_t *buf) {

  char msg[128];

  if (mapFile.size() > MAX_CSV_SIZE) {
    sprintf(msg, "%s exceeds specified maximum size (%i bytes)", mapFile.name(), MAX_CSV_SIZE);
    logMessage(msg, 0, 1);
    return;
  }
  
  char *csvStr = (char*)malloc(mapFile.size());
  memset(csvStr, 0, mapFile.size());
  mapFile.read(csvStr, mapFile.size());
  CSV_Parser cp(csvStr, "ss-");
  free(csvStr);

  for (int currLine=0; currLine<cp.getRowsCount(); currLine++) {

    char *inChannelStr  = ((char**)cp[0])[currLine];
    char *outChannelStr = ((char**)cp[1])[currLine];

    // Ignore empty lines
    if (strlen(inChannelStr) < 1 || strlen(outChannelStr) < 1) continue;

    // Make sure outChannel starts with a digit
    if (!isDigit(outChannelStr[0])) {
      sprintf(msg, "Invalid output channel");
      logMessage(msg, currLine+1);
      continue;
    }

    // Remove trailing spaces
    while (outChannelStr[strlen(outChannelStr)-1] == ' ') {
      outChannelStr[strlen(outChannelStr)-1] = 0;
    }

    // -1: Skip current line (error)
    //  0: Single channel
    //  1: Multiple channels
    //  2: Constant
    int mode = 0;
    char *constStr; // If needed

    // Get mode
    for (unsigned int i=0; i<strlen(outChannelStr); i++) {

      char c = outChannelStr[i];

      // Ignore spaces after commas
      if (i > 0 && c == ' ' && outChannelStr[i-1] == ',') {
        continue;
      }

      // Check for invalid characters
      if (!isDigit(c) && c != ',' && c != '@') {
        sprintf(msg, "Unexpected symbol '%c'", c);
        logMessage(msg, currLine+1);
        mode = -1;
        break;
      }

      // Multiple channels
      if (c == ',') {
        if (mode != 2 && isDigit(outChannelStr[i-1])) {
          mode = 1;
        } else {
          sprintf(msg, "Unexpected comma");
          logMessage(msg, currLine+1);
          mode = -1;
          break;
        }
      }

      // Constant
      if (c == '@') {
        if (mode == 0) {
          mode = 2;
          constStr = outChannelStr+i+1;
        } else { // Mode already assigned
          sprintf(msg, "Unexpected @");
          logMessage(msg, currLine+1);
          mode = -1;
          break;
        }
      }
    }

    if (mode == -1) {
      continue;
    }

    if (mode == 2) { // Constant

      if (!isDigit(constStr[0])) {
        sprintf(msg, "Missing constant after @");
        logMessage(msg, currLine+1);
        continue;
      }

      int constant = atoi(constStr);
      if (constant > 255) {
        sprintf(msg, "Constant exceeds 255");
        logMessage(msg, currLine+1);
        continue;
      }

      int outChannel = atoi(outChannelStr);
      if (outChannel > 512) {
        sprintf(msg, "Output channel out of bounds");
        logMessage(msg, currLine+1);
        continue;
      }

      if (buf[outChannel] != outChannel) {
        if (buf[outChannel] >= 1000) {
          sprintf(msg, "Duplicate constant for output channel %i", outChannel);
        } else {
          sprintf(msg, "Duplicate rule for output channel %i, replacing map with constant", outChannel);
        }
        logMessage(msg, currLine+1);
      }

      // Record constant into LUT
      sprintf(msg, "Writing constant %i --> Channel %i", constant, outChannel);
      logMessage(msg, currLine+1, 2);
      buf[outChannel] = constant + 1000; // 1xxx means constant
      
    } else { // Single or multiple output channels

      int inChannel = atoi(inChannelStr);
      if (inChannel < 0 || inChannel > 512) {
        sprintf(msg, "Input channel out of bounds");
        logMessage(msg, currLine+1);
        continue;
      }
      
      char *token = strtok(outChannelStr, ",");

      while (token != NULL) {

        int outChannel = atoi(token);
        
        if (outChannel > 512) {
          sprintf(msg, "Output channel out of bounds");
          logMessage(msg, currLine+1);
          
        } else {

          if (buf[outChannel] != outChannel) {
            sprintf(msg, "Duplicate rule for output channel %i", outChannel);
            logMessage(msg, currLine+1);
          }

          // Record channel into LUT
          sprintf(msg, "Mapping channel %i --> %i", inChannel, outChannel);
          logMessage(msg, currLine+1, 2);
          buf[outChannel] = inChannel;
        }

        token = strtok(NULL, ",");
      }
    }
  }
}

void readCustom(File mapFile, uint16_t *buf) {

  int currLine = 1; // Current line
  bool bypassRead = 0; // Set if data is already stored at the end of last cycle
  char c;

  char msg[128];
  
  // While not end of file
  while (mapFile.available()) {

    if (!bypassRead) {
      c = mapFile.read();
    }
    
    bypassRead = 0;

    if (c == '\n') {
      
      currLine++;
      
    } else if (c == ';') {

      // Continue until end of line
      do {
        if (!mapFile.available()) {
          mapFile.close();
          return;
        }
        c = mapFile.read();
      } while (c != '\n');

      currLine++;
      
    } else if (c == ' ' || c == '\t') {

      continue;

    // Read data
    } else if (isDigit(c)) {

      int inChannel = readInt(mapFile, &c, 1); // Read inChannel

      if (inChannel < 0) {
        sprintf(msg, "Unexpected EOF (expected range/output channel)");
        logMessage(msg, currLine, 1);
        mapFile.close();
        return;
      }

      if (inChannel > 512) {
        sprintf(msg, "Input channel out of bounds, skipping");
        logMessage(msg, currLine);
        currLine++;
        continue;
      }

      // Range mode
      int range = 1;
      if (c == '/') {
        
        range = readInt(mapFile, &c, 0); // Read range length

        if (range < 0) {
          sprintf(msg, "Unexpected EOF (expected output channel)");
          logMessage(msg, currLine, 1);
          mapFile.close();
          return;
        }

        if (inChannel + range > 513) {
          sprintf(msg, "Selected input range (%i) extends past 512 channels, skipping", range);
          logMessage(msg, currLine);
          currLine++;
          continue;
        }
      }

      // Skip over whitespace
      while (c == ' ' || c == '\t') {

        if (!mapFile.available()) {
          sprintf(msg, "Unexpected EOF (expected whitespace)");
          logMessage(msg, currLine, 1);
          mapFile.close();
          return;
        }

        c = mapFile.read();
        
      }

      // outChannel
      if (!isDigit(c)) {
        sprintf(msg, "Unexpected symbol '%c'", c);
        logMessage(msg, currLine);
        currLine++;
        continue;
      }

      int outChannel = readInt(mapFile, &c, 1, 1);

      // c already contains new data
      bypassRead = 1;

      if (outChannel > 512) {
        sprintf(msg, "Output channel out of bounds, skipping");
        logMessage(msg, currLine);
        currLine++;
        continue;
      }

      if (outChannel + range > 513) {
        sprintf(msg, "Selected output range (%i) extends past 512 channels, skipping", range);
        logMessage(msg, currLine);
        currLine++;
        continue;
      }

      for (int i=0; i<range; i++) {

        // Check if map already exists
        if (buf[outChannel+i] != outChannel+i) {
          sprintf(msg, "Duplicate rule for channel %i, mapping to %i", inChannel+i, outChannel+i);
          logMessage(msg, currLine);
        }

        sprintf(msg, "Mapping channel %i --> %i", inChannel+i, outChannel+i);
        logMessage(msg, 0, 2);
  
        // Actually record map
        buf[outChannel+i] = inChannel+i;
      }

      // Append newline
      msg[0] = 0;
      logMessage(msg, 0, 2);
      
    } else {

      sprintf(msg, "Unexpected symbol '%c'", c);
      logMessage(msg, currLine);

      // Continue until end of line
      do {
        if (!mapFile.available()) {
          mapFile.close();
          return;
        }
        c = mapFile.read();
      } while (c != '\n');
      
      currLine++;
    }
  }
  
  mapFile.close();
}

// Read LUT into buffer
// filePath: Path to file to read
// fileFormat:
//   0 - CSV
//   1 - Custom format
void readFromSD(char *filePath, int fileFormat, uint16_t *buf) {

  char msg[128];

  File mapFile = SD.open(filePath);
  if (!mapFile) {
    sprintf(msg, "Unable to open %s! Using saved map.", filePath);
    logMessage(msg);
    readEEPROM(buf);
    return;
  }

  switch(fileFormat) {

    case 0:
      readCSV(mapFile, buf);
      break;

    case 1:
      readCustom(mapFile, buf);
      break;
  }

  mapFile.close();
}
