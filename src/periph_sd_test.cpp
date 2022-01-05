#include "azure-storage-blobs.h"

static AzrureStorageBlobs   sd_test;

void periph_sd_test(void)
{
    sd_test.getExistVideoList();
}

/**
 * ----------------------------------------------------------------------------------------------
 * 
 * ----------------------------------------------------------------------------------------------
 */
#if defined (PERIPHERAL_TEST)

SD_Card   sd_card;

static void periph_test_task_entry(void *param)
{
  if(!SD_MMC.begin()){
      Serial.println("Card Mount Failed.");
  }

  int cardType = SD_MMC.cardType();
  if(cardType == CARD_NONE){
      Serial.println("No SD_MMC Card attached");
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

    sd_card.listDir(SD_MMC, "/", 0);
    sd_card.createDir(SD_MMC, "/mydir");
    sd_card.listDir(SD_MMC, "/", 0);
    sd_card.removeDir(SD_MMC, "/mydir");
    sd_card.listDir(SD_MMC, "/", 2);
    sd_card.writeFile(SD_MMC, "/hello.txt", "Hello ");
    sd_card.appendFile(SD_MMC, "/hello.txt", "World!\n");
    sd_card.readFile(SD_MMC, "/hello.txt");
    sd_card.deleteFile(SD_MMC, "/foo.txt");
    sd_card.renameFile(SD_MMC, "/hello.txt", "/foo.txt");
    sd_card.readFile(SD_MMC, "/foo.txt");
    sd_card.testFileIO(SD_MMC, "/test.txt");
    Serial.printf("Total space: %lluMB\n", SD_MMC.totalBytes() / (1024 * 1024));
    Serial.printf("Used space: %lluMB\n", SD_MMC.usedBytes() / (1024 * 1024));

  while(1){
    delay(2000);
    Serial.println("periph test task run.\r\n");
  }
}
static void periph_test_task_create(void)
{
  xTaskCreate(periph_test_task_entry,
              "periph test",
              4096,
              NULL,
              tskIDLE_PRIORITY + 6,
              &periph_test_handle_t);
  if(periph_test_handle_t == NULL){
    while(1);
  }
}
#endif