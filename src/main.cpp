/*|----------------------------------------------------------|*/
/*|WORKING EXAMPLE FOR HTTP/HTTPS CONNECTION                 |*/
/*|TESTED BOARDS: Devkit v1 DOIT, Devkitc v4                 |*/
/*|CORE: June 2018                                           |*/
/*|----------------------------------------------------------|*/
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "esp_wpa2.h"
#include <Wire.h>
#include "FreeRTOS_CLI.h"
#include <time.h>
#include <Esp.h>
#include "azure-storage-blobs.h"
#include "system_cfg.h"
#include "sd_driver.h"

TaskHandle_t  task_connect_ap_handle_t = NULL;
TaskHandle_t  task_download_handle_t = NULL;
TaskHandle_t  task_check_new_video_t =   NULL;
TaskHandle_t  task_power_manage_handle_t = NULL;
TaskHandle_t  task_uart2_handle_t = NULL;

AzrureStorageBlobs  AzureStorageBlobs_p;

typedef struct{
wl_status_t   connect_sta;
String ssid;
String password;
String azure_storage_url;
}esp32_conn_param;

esp32_conn_param   esp32_conn_p;


static void cli_task_create(void);
static void wifi_connect_ap_task_create(void);
static void wifi_download_task_create(void);
static void check_new_task_task_create(void);
static void power_manage_task_create(void);
static void serial2_task_create(void);

/**
 * ----------------------------------------------------------------------------------------------
 * 
 * ----------------------------------------------------------------------------------------------
 */
void setup() {
    delay(200);
    DBG_PRINT.begin(115200);
    Serial2.begin(115200);
    delay(200);
    DBG_PRINT.println();
    DBG_PRINT.println("system start.\r\n");
    DBG_PRINT.printf("flash size: %d\n", ESP.getFlashChipSize());
    Serial2.println("1111111111");//boot success.

    esp32_conn_p.ssid.clear();
    esp32_conn_p.password.clear();
    esp32_conn_p.azure_storage_url.clear();

    cli_task_create();
    wifi_connect_ap_task_create();
    wifi_download_task_create();
    check_new_task_task_create();
    power_manage_task_create();
    serial2_task_create();
}

void loop() {
   
}
/**
 * ----------------------------------------------------------------------------------------------
 * 
 * ----------------------------------------------------------------------------------------------
 */
static int esp32_reboot_device( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    strncpy(pcWriteBuffer, pcCommandString, xWriteBufferLen);

    ESP.restart();
    return pdFALSE;
}

static int esp32_ping_mcu( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    strncpy(pcWriteBuffer, pcCommandString, xWriteBufferLen);

    Serial2.println("ping");
    return pdFALSE;
}

static int esp32_get_info( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    sprintf(pcWriteBuffer, "CPU Frequency: %d.\r\n",
    ESP.getCpuFreqMHz());
    return pdFALSE;
}

static int esp32_set_cpu_furequency( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    const char *parameter;
    int pxParameterStringLength;
    strncpy(pcWriteBuffer, pcCommandString, xWriteBufferLen);

    parameter = FreeRTOS_CLIGetParameter(pcCommandString, 1, &pxParameterStringLength);

    if(parameter == NULL)return pdFALSE;

    // int freq = atoi(parameter);
    // ESP.setCpuFreqMhz(freq);

    return pdFALSE;
}

static int esp32_get_task_list( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    (void)xWriteBufferLen;
    (void)pcCommandString;

    // vTaskList((char *)pcWriteBuffer);

    return pdFALSE;
}
/**
 * 
 * */
static int esp32_enter_deepSleep_mode( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    const char *parameter;
    int pxParameterStringLength;
    strncpy(pcWriteBuffer, pcCommandString, xWriteBufferLen);

    parameter = FreeRTOS_CLIGetParameter(pcCommandString, 1, &pxParameterStringLength);

    if(parameter == NULL)return pdFALSE;

    int usec = atoi(parameter);

    DBG_PRINT.print("Deep sleep time is : ");
    DBG_PRINT.println(usec);

    ESP.deepSleep(usec);

    return pdFALSE;
}
/**
 * 
 * */
static int esp32_connect_ap( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    char ssid[32] = {0};
    char password[32] = {0};
    int pxParameterStringLength;

    strncpy(pcWriteBuffer, pcCommandString, xWriteBufferLen);

    if(esp32_conn_p.connect_sta == WL_CONNECTED)return pdFALSE;

    const char *p = FreeRTOS_CLIGetParameter(pcCommandString, 1, &pxParameterStringLength);
    if(p == NULL){
        return pdFALSE;
    }

    strncpy(ssid, p, pxParameterStringLength);
    esp32_conn_p.ssid.clear();
    esp32_conn_p.ssid += ssid;
    DBG_PRINT.println(esp32_conn_p.ssid);

    p = FreeRTOS_CLIGetParameter(pcCommandString, 2, &pxParameterStringLength);
    if(p == NULL)return pdFALSE;

    strncpy(password, p, pxParameterStringLength);
    esp32_conn_p.password.clear();
    esp32_conn_p.password += password;
    DBG_PRINT.println(esp32_conn_p.password);
    esp32_conn_p.connect_sta = WL_IDLE_STATUS;
    //resume wifi connect router task.
    vTaskResume(task_connect_ap_handle_t);
    return pdFALSE;
}
/**
 * 
 * */
static int esp32_disconnect_ap( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    strncpy(pcWriteBuffer, pcCommandString, xWriteBufferLen);
    if(esp32_conn_p.connect_sta == WL_DISCONNECTED)return pdFALSE;

    esp32_conn_p.connect_sta = WL_DISCONNECTED;
    return pdFALSE;
}
/**
 * 
 * */
void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    DBG_PRINT.println("Failed to obtain time");
    return;
  }

  Serial2.println(&timeinfo, "%a, %b %d %y %H:%M:%S");
  DBG_PRINT.println(&timeinfo, "%a, %b %d %y %H:%M:%S");
}
/**
 * 
 */
static int esp32_sync_network_time( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
  strncpy(pcWriteBuffer, pcCommandString, xWriteBufferLen);
  if(esp32_conn_p.connect_sta != WL_CONNECTED){
      DBG_PRINT.println("Please Connect AP first.\r\n");   
      return pdFALSE;
  }

  const char *ntpServer = "pool.ntp.org";
  const long gmt_offset_sec = 4 * 60 * 60;
  const int  daylight_offset_sec = 4 * 60 * 60;

  configTime(gmt_offset_sec, daylight_offset_sec, ntpServer);
  printLocalTime();

  return pdFALSE;
}
/**
 * 
 * */
static int esp32_download_data( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
  const char *parameter;
  int pxParameterStringLength;
  strncpy(pcWriteBuffer, pcCommandString, xWriteBufferLen);
  
  if(esp32_conn_p.connect_sta != WL_CONNECTED){
      DBG_PRINT.println("Please Connect AP first.\r\n");   
      return pdFALSE;
  }
  parameter = FreeRTOS_CLIGetParameter(pcCommandString, 1, &pxParameterStringLength);
  if(parameter == NULL)return pdFALSE;

  String url = String(parameter);
  AzureStorageBlobs_p.setUrl(url);
  vTaskResume(task_download_handle_t);
  return pdFALSE;
}
/**
 * @brief 
 * 
 * @param pcWriteBuffer 
 * @param xWriteBufferLen 
 * @param pcCommandString 
 * @return int 
 */
static int esp32_download_test( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    const char *parameter;
    int pxParameterStringLength;
    strncpy(pcWriteBuffer, pcCommandString, xWriteBufferLen);
  
    if(esp32_conn_p.connect_sta != WL_CONNECTED){
      DBG_PRINT.println("Please Connect AP first.\r\n");   
      return pdFALSE;
    }

    parameter = FreeRTOS_CLIGetParameter(pcCommandString, 1, &pxParameterStringLength);
    if(parameter == NULL)return pdFALSE;

    String url = String(parameter);
    AzureStorageBlobs_p.downloadTest(url);
    return pdFALSE;
}
/**
 * @brief 
 * 
 * @param pcWriteBuffer 
 * @param xWriteBufferLen 
 * @param pcCommandString 
 * @return int 
 */

enum{
PERIPH_TEST_SD = 0,
PERIPH_TYPE_MAX
};

typedef void (*periph_test_cb_func)(void);

extern void periph_sd_test(void);
const periph_test_cb_func periph_test_func_array[PERIPH_TYPE_MAX] = {
periph_sd_test
};

static int peripheral_test( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
  const char *periph_test_type[PERIPH_TYPE_MAX] = {"SD"};
  const char *parameter;
  int pxParameterStringLength;
  strncpy(pcWriteBuffer, pcCommandString, xWriteBufferLen);

  parameter = FreeRTOS_CLIGetParameter(pcCommandString, 1, &pxParameterStringLength);
  int i = 0;
  for(i = 0; i < PERIPH_TYPE_MAX; i++){
      if(strstr(parameter, periph_test_type[i]) != NULL)break;
  }
  if(i == PERIPH_TYPE_MAX)return false;

  periph_test_func_array[i]();

  return pdFALSE;
}
/**
 * @brief 
 * 
 * @param pcWriteBuffer 
 * @param xWriteBufferLen 
 * @param pcCommandString 
 * @return int 
 */
static int esp32_get_sd_info( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    AzureStorageBlobs_p.addSdCard();
    int cardType = AzureStorageBlobs_p.getSDType();
    if(cardType == CARD_MMC){
        sprintf(pcWriteBuffer, "SD is OK\r\n");
    } else if(cardType == CARD_SD){
        sprintf(pcWriteBuffer, "SD is OK\r\n");
    } else if(cardType == CARD_SDHC){
        sprintf(pcWriteBuffer, "SD is OK\r\n");
    } else {
        sprintf(pcWriteBuffer, "SD not found\r\n");
    }
    Serial2.println(pcWriteBuffer);
    return pdFALSE;
}
/**
 * @brief 
 * 
 * @param pcWriteBuffer 
 * @param xWriteBufferLen 
 * @param pcCommandString 
 * @return int 
 */
static int esp32_get_video_list_from_stm32( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    const char *parameter;
    int pxParameterStringLength;

    strncpy(pcWriteBuffer, pcCommandString, xWriteBufferLen);
    parameter = FreeRTOS_CLIGetParameter(pcCommandString, 1, &pxParameterStringLength);
    DBG_PRINT.println(parameter);
    if(parameter == NULL){
        return pdFALSE;
    }
    AzureStorageBlobs_p.setExitVideoList(parameter);

    Serial2.println("okokok\r\n");
    return pdFALSE;
}
/**
 * @brief 
 * 
 * @param pcWriteBuffer 
 * @param xWriteBufferLen 
 * @param pcCommandString 
 * @return int 
 */
static int esp32_check_new_vidoes_from_azure_bolb( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    // strncpy(pcWriteBuffer, pcCommandString, xWriteBufferLen);
    int pxParameterStringLength;
    const char *parameter = FreeRTOS_CLIGetParameter(pcCommandString, 1, &pxParameterStringLength);
    if(parameter == NULL)return pdFALSE;

    String url = String(parameter);
    AzureStorageBlobs_p.setUrl(url);
    DBG_PRINT.printf("<check new>url is: %s.\r\n", url.c_str());
    vTaskResume(task_check_new_video_t);
    return pdFALSE;
}
/**
 * @brief 
 * 
 */
enum{
CLI_REBOOT = 0,
CLI_PING_MCU,
CLI_SET_CPU_FREQ,
CLI_GET_ESP_FREQ,
CLI_TASK_LIST,
CLI_DEEP_SLEEP,
CLI_CONNECT_AP,
CLI_DISCONNECT_AP,
CLI_SYNC_TIME,
CLI_DOWNLOAD,
CLI_DOWNLOAD_TEST,
CLI_PERIPH_TEST,
CLI_GET_SD_INFO,
CLI_GET_VIDEOS_LIST,
CLI_CHECK_NEW_VIDEOS,
MAX_CLI_COUNT
}_cli_type;

const xCommandLineInput cli_Input[MAX_CLI_COUNT] = {
/* 1*/    {"reboot",                  "reboot                       : reboot device.\r\n", &esp32_reboot_device, 0},
/* 2*/    {"ping mcu",                "ping mcu                     : check connect mcu is ok.\r\n", &esp32_ping_mcu, 0},
/* 3*/    {"set cpu freq",            "set cpu freq=xMHz            : set CPU frequency.\r\n", &esp32_set_cpu_furequency, 1},
/* 4*/    {"get esp32 freq",          "get esp32 freq               : get esp32 frequency.\r\n", &esp32_get_info, 0},
/* 5*/    {"task list",               "task list                    : get all task.\r\n", &esp32_get_task_list, 0},
/* 6*/    {"deepSleep",               "deepSleep=msec               : config enter deep sleep mode.\r\n", &esp32_enter_deepSleep_mode, 1},
/* 7*/    {"connect ap",              "connect ap=ssid=password     : config wifi to connect router.\r\n", &esp32_connect_ap, 2},
/* 8*/    {"disconnect ap",           "disconnect ap                : disconnect router.\r\n",&esp32_disconnect_ap, 0},
/* 9*/    {"sync rtc time",           "sync rtc time                : sync rtc time used by network time.\r\n", &esp32_sync_network_time, 0},
/*10*/    {"download",                "download=url                 : url:download path.\r\n", &esp32_download_data, 1},
/*11*/    {"download test",           "download test=url            : url:download path.\r\n", &esp32_download_test, 1},
/*12*/    {"peripheral test",         "peripheral test=x            : x=SD: sd test.\r\n", &peripheral_test, 1},
/*13*/    {"get sd info",             "get sd info                  : get sd card info.\r\n", &esp32_get_sd_info, 0},
/*14*/    {"get videos",              "get videos=x                 : x: video name.\r\n", &esp32_get_video_list_from_stm32, 1},
/*15*/    {"check new",               "check new                    : check new videos from azure blob.\r\n", &esp32_check_new_vidoes_from_azure_bolb, 1},
};

/**
 * 
 * */
static void uart_command_handle(void *commandInput)
{                                                                   
    const char * const command = (const char * const)commandInput;
    char pcWriteBuffer[256] = {0}; 

    FreeRTOS_CLIProcessCommand(command, pcWriteBuffer, sizeof(pcWriteBuffer));

    DBG_PRINT.println(pcWriteBuffer);
    
    // Serial2.println(pcWriteBuffer);
}
/**
 * 
 * */
static void cli_task_entry( void * param)
{
    int rev_byte;
    char receive_buff[256] = {0};
    uint16_t rev_index = 0;

    for(int i = 0; i < MAX_CLI_COUNT; i++){
        FreeRTOS_CLIRegisterCommand(&cli_Input[i]);
    }

    while(1){      
#if (SERIAL_RECEIVE_USING_POLLING_MODE)
         rev_byte = Serial.read();
         if(rev_byte == -1){
             delay(100);
             continue;
         }
#else
        rev_byte = Serial.read(portMAX_DELAY);
#endif
        receive_buff[rev_index] = (char)rev_byte;
        if(receive_buff[rev_index] == '\n' && receive_buff[rev_index - 1] == '\r'){
          receive_buff[rev_index - 1] = 0;
          uart_command_handle(receive_buff);  
          rev_index = 0;
          continue;
        }

        rev_index++;
        if(rev_index == 256){
          rev_index = 0;
        }

    }
}
static void cli_task_create(void)
{
  DBG_PRINT.println("create cli task.\r\n");

  xTaskCreate(cli_task_entry,
              "cli task",
              6 * 1024,
              NULL,
              tskIDLE_PRIORITY + 10,
              NULL);

}
/**
 * ----------------------------------------------------------------------------------------------
 * 
 * ----------------------------------------------------------------------------------------------
 */
static bool wifi_connect_handle(const char *ssid, const char *password)
{
    uint32_t timeout = 0;
    String printArray;

    printArray.clear();
    printArray+= "WIFI ssid: ";
    printArray += ssid;
    DBG_PRINT.println(printArray);

    printArray.clear();
    printArray+= "WIFI password: ";
    printArray += password;
    DBG_PRINT.println(printArray);
    timeout = 0;
    WiFi.begin(ssid, password);
    WiFi.mode(WIFI_STA);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        timeout+=500;
        if(timeout == 30000){
            DBG_PRINT.println("\r\nError: connect timeout.\r\n");
            
            Serial2.println("Timeout");
            return pdFALSE;
        }
        
        DBG_PRINT.print(".");
        Serial2.println(".");
    }

    DBG_PRINT.println("");
    DBG_PRINT.println("WiFi connected");
    DBG_PRINT.println("IP address: ");
    DBG_PRINT.println(WiFi.localIP());
    delay(100);
    Serial2.println("Connect");
    esp32_conn_p.connect_sta = WL_CONNECTED;
    return pdTRUE;
}

static void wifi_connect_ap_task_entry(void *param)
{
    char ssid[64] = {0};
    char password[64] = {0};
    bool is_connect = false;

    vTaskSuspend( NULL );
    
    DBG_PRINT.println("wait connect router start.\r\n");
    while(1){
        if(esp32_conn_p.connect_sta == WL_DISCONNECTED){
            WiFi.disconnect(true);
            WiFi.mode(WIFI_OFF);
            DBG_PRINT.println("wifi disconnect success.");
            
            Serial2.println("Disconnect");
            vTaskSuspend( NULL );
        }else if(esp32_conn_p.connect_sta == WL_IDLE_STATUS){
            esp32_conn_p.ssid.toCharArray(ssid, sizeof(ssid));
            esp32_conn_p.password.toCharArray(password, sizeof(password));
            is_connect = wifi_connect_handle(ssid, password);
            if(!is_connect){
                esp32_conn_p.connect_sta = WL_CONNECT_FAILED;
            }
        }else if(esp32_conn_p.connect_sta == WL_CONNECT_FAILED){
            vTaskSuspend( NULL );
        }
        else{//esp32_conn_p.connect_sta = WL_CONNECTED;
            is_connect = WiFi.isConnected();
            if(!is_connect){
                DBG_PRINT.println("Wifi connect is lost.");
                
                Serial2.println("Disconnect");
                WiFi.disconnect(true);
                esp32_conn_p.connect_sta = WL_CONNECT_FAILED;
            }
        }
        delay(1000);
        // DBG_PRINT.println("wifi connect ap task run.");
    }
}

static void wifi_connect_ap_task_create(void)
{
  xTaskCreate(wifi_connect_ap_task_entry,
              "connect ap",
              8 * 1024,
              NULL,
              tskIDLE_PRIORITY + 9,
              &task_connect_ap_handle_t);
  if(task_connect_ap_handle_t == NULL){
    while(1);
  }
}
/**
 * ----------------------------------------------------------------------------------------------
 * 
 * ----------------------------------------------------------------------------------------------
 */
static void wifi_download_task_entry(void *param)
{
    bool sta = false;

    while(1){
        vTaskSuspend( NULL );
        sta = AzureStorageBlobs_p.getUrlList();
        if(!sta){
            goto wifi_download_end;
        }
        Serial2.println("okokok");
        sta = AzureStorageBlobs_p.download();
        if(!sta){
            goto wifi_download_end;
        }
        AzureStorageBlobs_p.deleteOldFile();
    wifi_download_end:
        DBG_PRINT.printf("00000000");
        Serial2.printf("00000000");
        vTaskDelay(1000);
        goto wifi_download_end;
    }
}
static void wifi_download_task_create(void)
{
  xTaskCreate(wifi_download_task_entry,
              "wifi download",
              16*1024,
              NULL,
              tskIDLE_PRIORITY + 7,
              &task_download_handle_t);
   if(task_download_handle_t == NULL){
    while(1);
  }
}
/**
 * ----------------------------------------------------------------------------------------------
 * 
 * ----------------------------------------------------------------------------------------------
 */
static void check_new_videos_task_entry(void *param)
{
    while(1){
        vTaskSuspend( NULL );
        DBG_PRINT.println("checknew videos task run.");
        AzureStorageBlobs_p.getUrlList();
        int ret = AzureStorageBlobs_p.getNeedDownloadCount();
        if(ret == 0){
            Serial2.println("nonono\r\n");
            continue;
        }

        Serial2.println("okokok\r\n");
    }
}
static void check_new_task_task_create(void)
{
    xTaskCreate(check_new_videos_task_entry,
            "check new video task",
            16 * 1024,
            NULL,
            tskIDLE_PRIORITY + 6,
            &task_check_new_video_t);
    if(task_check_new_video_t == NULL){
        while(1);
    }
}

/**
 * ----------------------------------------------------------------------------------------------
 * 
 * ----------------------------------------------------------------------------------------------
 */
static void power_manage_task_entry(void *param)
{
  vTaskSuspend( NULL );
  while(1){
    delay(2000);
    DBG_PRINT.println("power manage task run.\r\n");
  }
}
static void power_manage_task_create(void)
{
  xTaskCreate(power_manage_task_entry,
              "power manage",
              1024,
              NULL,
              tskIDLE_PRIORITY + 6,
              &task_power_manage_handle_t);
  if(task_power_manage_handle_t == NULL){
    while(1);
  }
}
/**
 * ----------------------------------------------------------------------------------------------
 * 
 * ----------------------------------------------------------------------------------------------
 */
static void serial2_task_entry(void *param)
{
    int rev_byte;
    char receive_buff[257] = {0};
    uint16_t rev_index = 0;

    DBG_PRINT.println("Serial2 task run.\r\n");

    while(1){
    #if (SERIAL_RECEIVE_USING_POLLING_MODE)
         rev_byte = Serial2.read();
         if(rev_byte == -1){
             delay(100);
             continue;
         }
    #else
        rev_byte = Serial2.read(portMAX_DELAY);
    #endif

        receive_buff[rev_index] = (char)rev_byte;
        if(receive_buff[rev_index] == '\n' && receive_buff[rev_index - 1] == '\r'){
          receive_buff[rev_index - 1] = 0;
          uart_command_handle(receive_buff);  
          rev_index = 0;
          continue;
        }

        rev_index++;
        if(rev_index == 256){
          rev_index = 0;
        }
    }
}

static void serial2_task_create(void)
{
    xTaskCreate(serial2_task_entry,
            "Serial2 task",
            4096,
            NULL,
            tskIDLE_PRIORITY + 6,
            &task_uart2_handle_t);
    if(task_uart2_handle_t == NULL){
        while(1);
    }
}
