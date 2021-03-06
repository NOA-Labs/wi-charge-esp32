#include "sd_driver.h"

SD_Card::SD_Card()
{

}

SD_Card::~SD_Card()
{

}

void SD_Card::listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    //Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        //Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        //Serial.println("Not a directory");
        return;
    }
    fileList.clear();
    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            //Serial.print("  DIR : ");
            //Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            //Serial.print("  FILE: ");
            //Serial.print(file.name());
            //Serial.print("  SIZE: ");
            //Serial.println(file.size());
        }
        fileList.push_back(String(file.name()));
        file = root.openNextFile();
    }

    root.close();
}

void SD_Card::createDir(fs::FS &fs, const char * path){
    //Serial.printf("Creating Dir: %s\n", path);
    if(fs.mkdir(path)){
        //Serial.println("Dir created");
    } else {
        //Serial.println("mkdir failed");
    }
}

void SD_Card::removeDir(fs::FS &fs, const char * path){
    //Serial.printf("Removing Dir: %s\n", path);
    if(fs.rmdir(path)){
        //Serial.println("Dir removed");
    } else {
        //Serial.println("rmdir failed");
    }
}

void SD_Card::readFile(fs::FS &fs, const char * path){
    //Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if(!file){
        //Serial.println("Failed to open file for reading");
        return;
    }

    //Serial.print("Read from file: ");
    while(file.available()){
        //Serial.write(file.read());
    }
}

void SD_Card::writeFile(fs::FS &fs, const char * path, const char * message){
    //Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        //Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        //Serial.println("File written");
    } else {
        //Serial.println("Write failed");
    }
}

void SD_Card::appendFile(fs::FS &fs, const char * path, const char * message){
    //Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        //Serial.println("Failed to open file for appending");
        return;
    }
    if(file.print(message)){
        //Serial.println("Message appended");
    } else {
        //Serial.println("Append failed");
    }
}

void SD_Card::renameFile(fs::FS &fs, const char * path1, const char * path2){
    //Serial.printf("Renaming file %s to %s\n", path1, path2);
    if (fs.rename(path1, path2)) {
        //Serial.println("File renamed");
    } else {
        //Serial.println("Rename failed");
    }
}

void SD_Card::deleteFile(fs::FS &fs, const char * path){
    //Serial.printf("Deleting file: %s\n", path);
    if(fs.remove(path)){
        //Serial.println("File deleted");
    } else {
        //Serial.println("Delete failed");
    }
}

void SD_Card::testFileIO(fs::FS &fs, const char * path){
    File file = fs.open(path);
    static uint8_t buf[512];
    size_t len = 0;
    uint32_t start = millis();
    uint32_t end = start;
    if(file){
        len = file.size();
        size_t flen = len;
        start = millis();
        while(len){
            size_t toRead = len;
            if(toRead > 512){
                toRead = 512;
            }
            file.read(buf, toRead);
            len -= toRead;
        }
        end = millis() - start;
        //Serial.printf("%u bytes read for %u ms\n", flen, end);
        file.close();
    } else {
        //Serial.println("Failed to open file for reading");
    }


    file = fs.open(path, FILE_WRITE);
    if(!file){
        //Serial.println("Failed to open file for writing");
        return;
    }

    size_t i;
    start = millis();
    for(i=0; i<2048; i++){
        file.write(buf, 512);
    }
    end = millis() - start;
    //Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
    file.close();
}

bool SD_Card::fileOpen(fs::FS &fs, const char * path)
{
    file = fs.open(path, FILE_WRITE);
    if(!file){
        //Serial.println("Failed to open file for writing");
        return false;
    }

    return true;
}

bool SD_Card::fileClose()
{
    file.close();
    return true;
}

size_t SD_Card::fileSize(fs::FS &fs, const char * path)
{
    File root = fs.open(path);
    if(!root){
        return 0;
    }

    size_t size = root.size();

    root.close();

    return size;
}

bool SD_Card::fileWrite(uint8_t *buff, int size)
{
    file.write(buff, size);
    return true;
}