#include "azure-storage-blobs.h"
#include "HTTPClient.h"
#include "nvs_flash.h"
#include "esp_vfs_fat.h"
#include "FS.h"
#include "SD_MMC.h"

/*
 * Connect the SD card to the following pins:
 *
 * SD Card | ESP32
 *    D2       12
 *    D3       13
 *    CMD      15
 *    VSS      GND
 *    VDD      3.3V
 *    CLK      14
 *    VSS      GND
 *    D0       2  (add 1K pull up after flashing)
 *    D1       4
 */

AzrureStorageBlobs::AzrureStorageBlobs()
{
    RequestHeaders.x_ms_blob_type = "BlockBlob";
    UrlList.clear();
    existFileList.clear();
}

AzrureStorageBlobs::~AzrureStorageBlobs()
{

}

bool AzrureStorageBlobs::addSdCard()
{
    if(!SD_MMC.begin()){
        Serial.println("Card Mount Failed");
        return false;
    }
    uint8_t cardType = SD_MMC.cardType();

    if(cardType == CARD_NONE){
        Serial.println("No SD_MMC card attached");
        return false;
    }

    Serial.print("SD_MMC Card Type: ");
    if(cardType == CARD_MMC){
        Serial.println("MMC");
    } else if(cardType == CARD_SD){
        Serial.println("SDSC");
    } else if(cardType == CARD_SDHC){
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }

    uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
    Serial.printf("SD_MMC Card Size: %lluMB\n", cardSize);

    Serial.printf("Total space: %lluMB\n", SD_MMC.totalBytes() / (1024 * 1024));
    Serial.printf("Used space: %lluMB\n", SD_MMC.usedBytes() / (1024 * 1024));
    return true;
}
/**
 * struct -requestHeaders{
        String Authorization;
        String x-ms-date;
        String x-ms-version;
        String Content-Length;
        String x-ms-copy-source;
        String x-ms-blob-type;
        String x-ms-blob-content-disposition;
    } RequestHeaders;
 * */
bool AzrureStorageBlobs::addRequestHeaders(const String resType, const String value)
{
    if(resType == "Authorization"){
        RequestHeaders.Authorization = value;
    }
    else if(resType == "x-ms-date"){
        RequestHeaders.x_ms_date = value;
    }
    else if(resType == "x-ms-version"){
        RequestHeaders.x_ms_version = value;
    }
    else if(resType == "Content-Length"){
        RequestHeaders.Content_Length = value;
    }
    else if(resType == "x-ms-copy-source"){
        RequestHeaders.x_ms_copy_source = value;  
    }
    else if(resType == "x-ms-blob-type"){
        RequestHeaders.x_ms_blob_type = value;  
    }
    else if(resType == "x-ms-blob-content-disposition"){
        RequestHeaders.x_ms_blob_content_disposition = value;
    }
    else{
        return false;
    }

    return true;
}

bool AzrureStorageBlobs::splitUrlList(char *pathList)
{
    String fileType = String(".avi");
    String protocol  = String("https");

    if(strstr(pathList, ".bin") != NULL){//检测到bin升级文件。

        return true;
    }

    // Serial.printf("\nPath List:\n %s\n", pathList);

    String strPathList = String(pathList);
    int pos = strPathList.indexOf(fileType);
    while(pos != -1){
        String getPath = strPathList.substring(0, pos + fileType.length());
        int newPos = getPath.indexOf(protocol);
        getPath.remove(0, newPos);// remove \r\n or other strings.
        checkUrlIsValid(getPath);
        strPathList.remove(0, pos + fileType.length());
        pos = strPathList.indexOf(fileType);
    }

    for(int i = 0; i < UrlList.size(); i++){
        Serial.printf("Video %d url is: %s.\n", i, UrlList.at(i).c_str());
    }
    
    return true;
}
bool AzrureStorageBlobs::getExistFileList()
{
    Serial.println("get exist file list.");
    bool sdcard_sta = addSdCard();
    if(!sdcard_sta)return false;

    SD.listDir(SD_MMC, "/Videos", 0);

    existFileList = SD.getFileList();
    for(int i = 0; i < existFileList.size(); i++){
        Serial.printf("File %d is %s.\n", i, existFileList.at(i).c_str());
    }

    return true;
}
/**
 * https://wichargestorage.blob.core.windows.net/videos/480x272.avi
 * https://wichargestorage.blob.core.windows.net/videos/800x480 long.avi
 * https://wichargestorage.blob.core.windows.net/videos/800x480.avi
 * */
bool AzrureStorageBlobs::getUrlList()
{
    bool status = true;
    HTTPClient http;
    char buff[256] = {0};

    Serial.println("get url list from azure.");

    if(azureUrl.isEmpty())return false;
    Serial.printf("video list file path: %s\n", azureUrl.c_str());

    downloadFileList.clear();
    UrlList.clear();

    http.begin(azureUrl);
    int httpCode = http.GET();
    if(httpCode > 0){
        Serial.printf("[Http]GET Code: %d\n", httpCode);

        if(httpCode == HTTP_CODE_OK){
            int len = http.getSize();
            Serial.printf("the file size is: %d\n", len);

            WiFiClient *stream = http.getStreamPtr();
            while(http.connected() && (len > 0 || len == -1)){
                size_t size = stream->available();
                if(size){
                    int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
                    Serial.write(buff, c);
                    if(len > 0){
                        len -= c;
                    }
                }
            }
            splitUrlList(buff);
        }else{
            status = false;
        }
    }
    else{
        Serial.printf("[Http] Get failed, error: %s\n", http.errorToString(httpCode).c_str());
        status = false;
    }

    http.end();
    Serial.println("video list download ok.\r\n");

    return status ;
}

bool  AzrureStorageBlobs::checkUrlIsValid(String &url)
{
    String fileName = getFileNameFromUrl(url);
    Serial.println();
    Serial.printf("file name is: %s\n", fileName.c_str());

    int i = 0;
    while(i < existFileList.size()){
        if(existFileList.at(i++) == fileName){
            downloadFileList.push_back(fileName);
            Serial.printf("current file:( %s ) is exist,didn't need to download.\r\n", fileName.c_str());
            return false;       
        }
    }
    Serial.printf("input < %s >  URL( %s ) to download list.\r\n", fileName.c_str(), url.c_str());
    UrlList.push_back(url);
    return true;
}

uint8_t download_buff[32*1024] = {0};
bool AzrureStorageBlobs::downloadFile(String& url)
{
    HTTPClient http;
    bool status = true;

    String filePath = String("/Videos/") + getFileNameFromUrl(url);
    Serial.print("save file to : ");
    Serial.println(filePath);
    SD.fileOpen(SD_MMC, filePath.c_str());
    delay(500);
    http.begin(url);
    Serial.printf("http download video url is :%s\r\n", url.c_str());
    int httpCode = http.GET();
    if(httpCode > 0){
        Serial.printf("[Http]GET Code: %d\n", httpCode);

        if(httpCode == HTTP_CODE_OK){
            int len = http.getSize();
            int total_size = 0;
            int download_count = 0;
            fileTotalSize = len;
            Serial.printf("the file size is: %d\n", len);
        
            WiFiClient *stream = http.getStreamPtr();
            while(http.connected() && (len > 0 || len == -1)){
                size_t size = stream->available();
                if(size){
                    int c = stream->readBytes(download_buff, ((size > sizeof(download_buff)) ? sizeof(download_buff) : size));
                    //write receive data to sd card.
                    // SD.fileWrite(download_buff, c);
                    if(len > 0){
                        len -= c;
                        total_size += c;
                        download_count ++;
                        fileRemainingSize = len;
                    }
                }
            }
            Serial.printf("get size is %d. download count is %d.\r\n", total_size, download_count);
        }else{
            status = false;
        }
    }
    else{
        Serial.printf("[Http] Get failed, error: %s\n", http.errorToString(httpCode).c_str());
        status = false;
    }
    SD.fileClose();
    http.end();
    if(!status){
        Serial.printf("Http Get failed\r\n");
    }
    else{
        Serial.printf("download video succcess, save to sd card.\r\n");
    }
    
    return status ;
}
/**
 * @brief 
 * 
 * @param url 
 * @return true 
 * @return false 
 */
bool AzrureStorageBlobs::downloadTest(String& url)
{
    HTTPClient http;
    bool status = true;

    http.begin(url);
    Serial.printf("http download video url is :%s\r\n", url.c_str());
     int httpCode = http.GET();
    if(httpCode > 0){
        Serial.printf("[Http]GET Code: %d\n", httpCode);

        if(httpCode == HTTP_CODE_OK){
            int len = http.getSize();
            int total_size = 0;
            int download_count = 0;
            fileTotalSize = len;
            Serial.printf("the file size is: %d\n", len);
        
            WiFiClient *stream = http.getStreamPtr();
            while(http.connected() && (len > 0 || len == -1)){
                size_t size = stream->available();
                if(size){
                    int c = stream->readBytes(download_buff, ((size > sizeof(download_buff)) ? sizeof(download_buff) : size));
                    //write receive data to sd card.
                    Serial.printf("download count %d. bytes %d.\r\n",download_count, c);
                    if(len > 0){
                        len -= c;
                        total_size += c;
                        download_count ++;
                        fileRemainingSize = len;
                    }
                }
            }
        }else{
            status = false;
        }
    }
    else{
        Serial.printf("[Http] Get failed, error: %s\n", http.errorToString(httpCode).c_str());
        status = false;
    }
    SD.fileClose();
    http.end();
    if(!status){
        Serial.printf("download failed.\r\n");
    }
    else{
        Serial.printf("download success.\r\n");
    }
    
    return status ;
}

bool AzrureStorageBlobs::download()
{
    if(UrlList.empty())return false;
    Serial.printf("need download file number: %d\n", UrlList.size());
    
    for(int i = 0; i < UrlList.size(); i++){
        Serial.printf("start download video: %s\n.", UrlList.at(i).c_str());
        downloadFile(UrlList.at(i));
    }
    return true;
}

String AzrureStorageBlobs::getFileNameFromUrl(String &url)
{
    // String filename = url.find_last_of('/');
    int index = url.lastIndexOf('/');
    return url.substring(index + 1);// remove '/'.
}

void AzrureStorageBlobs::deleteOldFile()
{
    int i, j, errCount;
    for(i = 0; i < existFileList.size(); i++){
        errCount = 0;
        for(j = 0; j < downloadFileList.size(); j++){
            if(existFileList.at(i) != downloadFileList.at(j)){
                errCount++;
            }
        }
        if(errCount == j){//need delete existFileList.at(i);
            String filePath = String("/Videos/") + downloadFileList.at(j);
            SD.deleteFile(SD_MMC, filePath.c_str());
        }
    }
}