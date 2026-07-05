/*
 * hw_eeprom.h — 不揮発メモリ（内蔵FlashのEEPROM代用）の境界API
 *
 * アドレスは「先頭からのオフセット」で指定する。実際のFlashアドレスや
 * ページ構造（マイコンごとに大きく異なる）は platform/hw_eeprom.c に隠蔽する。
 */

#ifndef HW_HW_EEPROM_H_
#define HW_HW_EEPROM_H_

#include <stdint.h>

// 戻り値: 0=成功, 0以外=失敗
int hw_eeprom_enable_write(void); // 領域を消去して書き込みを有効にする
int hw_eeprom_disable_write(void);

// offsetは2バイト単位の通し番号（0, 1, 2, ...）
int hw_eeprom_write_halfword(uint32_t offset, uint16_t data);
uint16_t hw_eeprom_read_halfword(uint32_t offset);

#endif /* HW_HW_EEPROM_H_ */
