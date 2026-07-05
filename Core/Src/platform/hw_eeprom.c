/*
 * hw_eeprom.c — 内蔵FlashのEEPROM代用（STM32F303K8実装）
 *
 * Flashの最終ページ（Page 31, 2KB）をデータ保存領域として使う。
 * F303のFlashは2KB単位のページ消去・2バイト単位の書き込み。
 * ※ マイコンを変えるとページサイズ・書き込み単位・APIが大きく変わるため，
 *   このファイルは移植時に全面的に書き直す前提でよい（hw_eeprom.hは変えない）。
 */

#include "main.h"
#include "hw/hw_eeprom.h"

#define EEPROM_START_ADDRESS ((uint32_t)0x0800F800) // Page 31 の先頭

int hw_eeprom_enable_write(void) {
    HAL_StatusTypeDef status;
    FLASH_EraseInitTypeDef erase;
    uint32_t page_error = 0;

    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.PageAddress = EEPROM_START_ADDRESS;
    erase.NbPages = 1;

    status = HAL_FLASH_Unlock();
    if (status != HAL_OK) {
        return -1;
    }
    status = HAL_FLASHEx_Erase(&erase, &page_error);
    if (status != HAL_OK) {
        HAL_FLASH_Lock(); // 消去に失敗したときにunlockのまま残さない
        return -1;
    }
    return 0;
}

int hw_eeprom_disable_write(void) {
    return (HAL_FLASH_Lock() == HAL_OK) ? 0 : -1;
}

int hw_eeprom_write_halfword(uint32_t offset, uint16_t data) {
    uint32_t address = offset * 2 + EEPROM_START_ADDRESS;
    HAL_StatusTypeDef status =
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, address, data);
    return (status == HAL_OK) ? 0 : -1;
}

uint16_t hw_eeprom_read_halfword(uint32_t offset) {
    uint32_t address = offset * 2 + EEPROM_START_ADDRESS;
    return *(__IO uint16_t *)address;
}
