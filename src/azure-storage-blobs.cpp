#include "azure-storage-blobs.h"
#include "HTTPClient.h"
#include "nvs_flash.h"
#include "esp_vfs_fat.h"
#include "FS.h"
#include "SD_MMC.h"
#include "system_cfg.h"
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
    existVideoList.clear();
}

AzrureStorageBlobs::~AzrureStorageBlobs()
{

}

bool AzrureStorageBlobs::addSdCard()
{
    if(!SD_MMC.begin()){
        DBG_PRINT.println("Card Mount Failed");
        sdType = CARD_UNKNOWN;
        return false;
    }
    
    uint8_t cardType = SD_MMC.cardType();

    if(cardType == CARD_NONE){
        DBG_PRINT.println("No SD_MMC card attached");
        sdType = CARD_UNKNOWN;
        return false;
    }
    
    sdType = cardType;
    DBG_PRINT.print("SD_MMC Card Type: ");
    if(cardType == CARD_MMC){
        DBG_PRINT.println("MMC");
    } else if(cardType == CARD_SD){
        DBG_PRINT.println("SDSC");
    } else if(cardType == CARD_SDHC){
        DBG_PRINT.println("SDHC");
    } else {
        DBG_PRINT.println("UNKNOWN");
    }

    uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
    DBG_PRINT.printf("SD_MMC Card Size: %lluMB\n", cardSize);

    DBG_PRINT.printf("Total space: %lluMB\n", SD_MMC.totalBytes() / (1024 * 1024));
    DBG_PRINT.printf("Used space: %lluMB\n", SD_MMC.usedBytes() / (1024 * 1024));
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
    String protocol  = String("http");
    DBG_PRINT.println("\nenter < splitUrlList >");

    if(strstr(pathList, ".bin") != NULL){//检测到bin升级文件。

        return true;
    }

    String strPathList = String((const char *)pathList);
    int pos = strPathList.indexOf(fileType);
    DBG_PRINT.printf("url list is:\n%s\n", strPathList.c_str());
    while(pos != -1){
        String getPath = strPathList.substring(0, pos + fileType.length());
        int newPos = getPath.indexOf(protocol);
        getPath.remove(0, newPos);// remove \r\n or other strings.
        checkUrlIsValid(getPath);
        strPathList.remove(0, pos + fileType.length());
        pos = strPathList.indexOf(fileType);
    }

    for(int i = 0; i < UrlList.size(); i++){
        DBG_PRINT.printf("Video %d url is: %s.\n", i, UrlList.at(i).c_str());
    }
    
    DBG_PRINT.println("exit < splitUrlList >");
    return true;
}

bool AzrureStorageBlobs::setExitVideoList(const char *video_name)
{
    String path = String(video_name);
    int index = path.lastIndexOf('/');
    String name = path.substring(index + 1);
    DBG_PRINT.printf("rev file is : %s.\r\n", name.c_str());
    existVideoList.push_back(name);
    return true;
}
/**
 * @brief 
 * 
 * @return true 
 * @return false 
 * @note   save video name format: xxx.avi,ex: 800x480.avi
 */
bool AzrureStorageBlobs::getExistVideoList()
{
    DBG_PRINT.println("enter < getExistVideoList >");
    DBG_PRINT.println("get exist file list.");
    bool sdcard_sta = addSdCard();
    if(!sdcard_sta)return false;

    SD.listDir(SD_MMC, "/Videos", 0);

    existVideoList.clear();
    std::vector<String> fileList = SD.getFileList();

    for(int i = 0; i < fileList.size(); i++){
        int size  = SD.fileSize(SD_MMC, fileList.at(i).c_str());
        int index = fileList.at(i).lastIndexOf('/');
        String  getFileName = fileList.at(i).substring(index + 1);
        if(size > 0){
            existVideoList.push_back(getFileName);
        }

        DBG_PRINT.printf("File %d is %s, Size %d\n", i, getFileName.c_str(), size);
    }
    DBG_PRINT.println("Valid Video list:");
    for(int i = 0; i < existVideoList.size(); i++){
        DBG_PRINT.println(existVideoList.at(i));
    }

    DBG_PRINT.println("exit < getExistVideoList >");
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
    DBG_PRINT.println("enter < getUrlList >");
    DBG_PRINT.println("get url list from azure.");

    if(azureUrl.isEmpty())return false;
    DBG_PRINT.printf("video list file path: %s\n", azureUrl.c_str());

    downloadVideoList.clear();
    UrlList.clear();

    http.begin(azureUrl);
    int httpCode = http.GET();
    if(httpCode > 0){
        DBG_PRINT.printf("[Http]GET Code: %d\n", httpCode);

        if(httpCode == HTTP_CODE_OK){
            int len = http.getSize();
            DBG_PRINT.printf("the file size is: %d\n", len);

            WiFiClient *stream = http.getStreamPtr();
            while(http.connected() && (len > 0 || len == -1)){
                size_t size = stream->available();
                if(size){
                    int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
                    DBG_PRINT.write(buff, c);
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
        DBG_PRINT.printf("[Http] Get failed, error: %s\n", http.errorToString(httpCode).c_str());
        status = false;
    }

    http.end();
    DBG_PRINT.println("video list download ok.\r\n");
    DBG_PRINT.println("exit < getUrlList >");
    return status ;
}

bool  AzrureStorageBlobs::checkUrlIsValid(String &url)
{
    String fileName = getFileNameFromUrl(url);
    DBG_PRINT.println();
    DBG_PRINT.println("enter < checkUrlIsValid >");
    DBG_PRINT.printf("url is: < %s >\nfile name is: < %s >\n", url.c_str(), fileName.c_str());
    bool sta = true;
    int i = 0;
    while(i < existVideoList.size()){
        if(existVideoList.at(i++) == fileName){//网络上的视频和本地视频一样，直接将该视频放到已下载列表中。
            downloadVideoList.push_back(fileName);
            DBG_PRINT.printf("current file:( %s ) is exist,didn't need to download.\r\n", fileName.c_str());
            sta = false;       
            break;
        }
    }
    if(sta){
        DBG_PRINT.printf("input < %s >  URL( %s ) to download list.\r\n", fileName.c_str(), url.c_str());
        UrlList.push_back(url);
    }
    DBG_PRINT.println("exit < checkUrlIsValid >");

    return sta;
}

uint8_t download_buff[32768] = {0};
bool AzrureStorageBlobs::downloadFile(String& url)
{
    HTTPClient http;
    bool status = true;
    DBG_PRINT.println("enter < downloadFile >");
    String filePath = String("/Videos/") + getFileNameFromUrl(url);
    DBG_PRINT.print("save file to : ");
    DBG_PRINT.println(filePath);
    SD.fileOpen(SD_MMC, filePath.c_str());
    delay(500);
    http.begin(url);
    DBG_PRINT.printf("http download video url is :%s\r\n", url.c_str());
    int httpCode = http.GET();
    String fileName = getFileNameFromUrl(url);

    if(httpCode > 0){
        DBG_PRINT.printf("[Http]GET Code: %d\n", httpCode);

        if(httpCode == HTTP_CODE_OK){
            int len = http.getSize();
            int download_count = 0;
            int download_size = 0;
            int start_ms = 0;
            int cur_ms = 0;
            int total_ms = 0;
            int bkp_total_ms = 0;
            int downloadPersent = 0;

            fileTotalSize = len;
            DBG_PRINT.printf("the file size is: %d\n", len);

            WiFiClient *stream = http.getStreamPtr();
            
            start_ms = xTaskGetTickCount();
            
            while(http.connected() && (len > 0 || len == -1)){
                size_t size = stream->available();
                if(size){
                    int c = stream->readBytes(download_buff, ((size > sizeof(download_buff)) ? sizeof(download_buff) : size));
                    cur_ms = xTaskGetTickCount();
                    total_ms = cur_ms - start_ms;
                    total_ms /= 1000;
                    if(total_ms > bkp_total_ms){
                        bkp_total_ms = total_ms;
                        downloadPersent = download_size * 100 / fileTotalSize;
                        DBG_PRINT.printf("%d s, %d bytes per sec.\r\n", total_ms, download_size / total_ms);
                        DBG_PRINT.printf("(%d/%d)\n%s\n%d %%\n",curDownloadFileIndex, needDownloadFileNumber, fileName.c_str(), downloadPersent);
                        Serial2.printf("(%d/%d)\n%s\n%d %%\n\r\n",curDownloadFileIndex, needDownloadFileNumber, fileName.c_str(), downloadPersent);
                    }
                    //write receive data to sd card.
                    SD.fileWrite(download_buff, c);
                    
                    if(len > 0){
                        len -= c;
                        download_size += c;
                        download_count ++;
                        fileRemainingSize = len;
                    }
                }
            }
            DBG_PRINT.printf("file size is %d. download count is %d.\r\n", download_size, download_count);
        }else{//download failed. delete new create file.
            DBG_PRINT.printf("download failed.\r\n");
            Serial2.printf("download failed.\r\n");
            SD.deleteFile(SD_MMC, filePath.c_str());
            status = false;
        }
    }
    else{
        DBG_PRINT.printf("[Http] Get failed, error: %s\n", http.errorToString(httpCode).c_str());
        status = false;
    }
    SD.fileClose();
    http.end();
    if(!status){
        DBG_PRINT.printf("Http Get failed\r\n");
    }
    else{
        downloadVideoList.push_back(fileName);
        DBG_PRINT.printf("download video succcess, save to sd card.\r\n");
    }
    DBG_PRINT.println("exit < downloadFile >");
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
    DBG_PRINT.printf("http download video url is :%s\r\n", url.c_str());
     int httpCode = http.GET();
    if(httpCode > 0){
        DBG_PRINT.printf("[Http]GET Code: %d\n", httpCode);

        if(httpCode == HTTP_CODE_OK){
            int len = http.getSize();
            int download_size = 0;
            int download_count = 0;
            int start_ms = 0;
            int cur_ms = 0;
            int total_ms = 0;
            int bkp_total_ms = 0;
            fileTotalSize = len;
            DBG_PRINT.printf("the file size is: %d\n", len);
        
            WiFiClient *stream = http.getStreamPtr();
            start_ms = xTaskGetTickCount();
            while(http.connected() && (len > 0 || len == -1)){
                size_t size = stream->available();
                if(size){
                    int c = stream->readBytes(download_buff, ((size > sizeof(download_buff)) ? sizeof(download_buff) : size));
                    //write receive data to sd card.
                    cur_ms = xTaskGetTickCount();
                    total_ms = cur_ms - start_ms;
                    total_ms /= 1000;
                    if(total_ms > bkp_total_ms){
                        bkp_total_ms = total_ms;
                        DBG_PRINT.printf("%d s, %d bytes per sec.\r\n", total_ms, download_size / total_ms);
                    }
                    if(len > 0){
                        len -= c;
                        download_size += c;
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
        DBG_PRINT.printf("[Http] Get failed, error: %s\n", http.errorToString(httpCode).c_str());
        status = false;
    }
    SD.fileClose();
    http.end();
    if(!status){
        DBG_PRINT.printf("download failed.\r\n");
    }
    else{
        DBG_PRINT.printf("download success.\r\n");
    }
    
    return status ;
}

bool AzrureStorageBlobs::download()
{
    if(UrlList.empty()){
        return false;
    }
    DBG_PRINT.printf("need download file number: %d\n", UrlList.size());
    needDownloadFileNumber = UrlList.size();
    for(int i = 0; i < UrlList.size(); i++){
        DBG_PRINT.printf("start download video: %s\n.", UrlList.at(i).c_str());
        curDownloadFileIndex = i + 1;
        downloadFile(UrlList.at(i));
    }
    return true;
}
/**
 * @brief 
 * 
 * @param url 
 * @return String  ex : return  800x480.avi
 */
String AzrureStorageBlobs::getFileNameFromUrl(String &url)
{
    // String filename = url.find_last_of('/');
    int index = url.lastIndexOf('/');
    return url.substring(index + 1);// remove '/'.
}

bool AzrureStorageBlobs::deleteOldFile()
{
    int i, j, errCount;
    int downloadFileSize = downloadVideoList.size();
    int existFileSize    = existVideoList.size();
    DBG_PRINT.println("enter < deleteOldFile >");

    if(downloadFileSize == 0 || existFileSize == 0){
        goto exit_delete_old_file;
    }

    for(i = 0; i < existFileSize; i++){
        errCount = 0;
        for(j = 0; j < downloadFileSize; j++){
            if(existVideoList.at(i) != downloadVideoList.at(j)){
                errCount++;
            }
        }
        String filePath = String("/Videos/" + existVideoList.at(i));
        DBG_PRINT.println(filePath);
        if(errCount == j && errCount != 0){//need delete existVideoList.at(i);
            DBG_PRINT.printf("delete file is %s.\n", existVideoList.at(i).c_str());
            SD.deleteFile(SD_MMC, filePath.c_str());
        }
    }

exit_delete_old_file:
    DBG_PRINT.println("exit < deleteOldFile >");
    return true;
}