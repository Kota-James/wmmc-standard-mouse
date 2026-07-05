/*
 * app_hooks.h — platform層からapp層を呼び出すためのフック宣言
 *
 * 割り込みなど platform 側のタイミングで実行したい処理のうち、
 * 中身がアルゴリズム（app層の責務）であるものをここに宣言する。
 * 実装は app 側にある（関数ポインタを使わずリンク時に結合される）。
 */

#ifndef HW_APP_HOOKS_H_
#define HW_APP_HOOKS_H_

// 12ms周期の制御タイミングで呼ばれる（タイマ割り込み内なので重い処理・printf禁止）
// 実装: Core/Src/app/control.c
void app_control_tick(void);

#endif /* HW_APP_HOOKS_H_ */
