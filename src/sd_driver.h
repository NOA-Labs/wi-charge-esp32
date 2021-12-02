#ifndef __SD_DRIVER_H
#define __SD_DRIVER_H

#include <Arduino.h>
#include "FS.h"
#include "SD_MMC.h"

class SD_Card
{
public:
SD_Card();
~SD_Card();

void listDir(fs::FS &fs, const char * dirname, uint8_t levels);
std::vector<String> getFileList(){return fileList;}
void createDir(fs::FS &fs, const char * path);
void removeDir(fs::FS &fs, const char * path);
void readFile(fs::FS &fs, const char * path);
void writeFile(fs::FS &fs, const char * path, const char * message);
void appendFile(fs::FS &fs, const char * path, const char * message);
void renameFile(fs::FS &fs, const char * path1, const char * path2);
void deleteFile(fs::FS &fs, const char * path);
void testFileIO(fs::FS &fs, const char * path);
bool fileOpen(fs::FS &fs, const char * path);
bool fileClose();
bool fileWrite(uint8_t *buff, int size);
private:
    std::vector<String> fileList;
    File file;
};
#endif
