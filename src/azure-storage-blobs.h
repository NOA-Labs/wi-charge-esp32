#ifndef __AZURE_STORAGE_BLOBS_H
#define __AZURE_STORAGE_BLOBS_H

#include <Arduino.h>
#include "sd_driver.h"

// ref: https://docs.microsoft.com/en-us/rest/api/storageservices/put-blob-from-url
class AzrureStorageBlobs
{
public:
    AzrureStorageBlobs();
    ~AzrureStorageBlobs();
    bool addSdCard();
    int getSDType(){return sdType;}
    bool addRequestHeaders(const String resType, const String value);
    void setUrl(const String url){azureUrl = url;}
    String getUrl(){return azureUrl;}
    bool setExitVideoList(const char *video_name);
    bool getExistVideoList();
    bool getUrlList();
    int  getNeedDownloadCount(){return UrlList.size();}
    bool checkUrlIsValid(String &url);
    bool splitUrlList(char *pathList);
    bool downloadFile(String& url);
    bool downloadTest(String& url);
    bool download();
    String getFileNameFromUrl(String &url);
    bool deleteOldFile();
    int  getFileTotalSize(){return fileTotalSize;}
    int  getFileRemainingSize(){return fileRemainingSize;}
private:
    class SD_Card SD;
    int sdType;
    int fileTotalSize;
    int fileRemainingSize;
    int needDownloadFileNumber;
    int curDownloadFileIndex;
    String azureUrl;
    struct _requestHeaders{
        String Authorization;
        String x_ms_date;
        String x_ms_version;
        String Content_Length;
        String x_ms_copy_source;
        String x_ms_blob_type;
        String x_ms_blob_content_disposition;
    } RequestHeaders;
    struct _responseHeaders{
        String ETag;
        String Last_Modified;
        String Content_MD5;
        String x_ms_conten_crc64;
        String x_ms_request_id;
        String x_ms_version;
        String Date;
        String Access_Control_Allow_Origin;
        String Access_Control_Allow_Expose_Headers;
        String Access_Control_Allow_Credentials;
        String x_ms_request_server_encrypted;
        String x_ms_encryption_key_sha256;
        String x_ms_encryption_scope;
        String x_ms_version_id;         //This header returns an opaque DateTime value that uniquely identifies the blob.
    }ResponseHeaders;
    
    String header;

    std::vector<String> UrlList;
    std::vector<String> existVideoList;
    std::vector<String> downloadVideoList;
    
    String myAccountName;
    String myAccountSharedKey;
    String myContainer;
    String myBlob;
};

#endif
