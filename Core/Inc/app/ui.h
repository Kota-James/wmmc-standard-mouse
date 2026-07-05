/*
 * ui.h — モード選択などのユーザーインターフェース
 */

#ifndef APP_UI_H_
#define APP_UI_H_

// スイッチでモード番号を選ぶ（SW1: +1, SW2: -1, SW3: 決定）
// 選択中の番号はLED1〜3に2進数で表示される
// 引数: モード番号の初期値 / 戻り値: 決定されたモード番号
int select_mode(int mode);

#endif /* APP_UI_H_ */
