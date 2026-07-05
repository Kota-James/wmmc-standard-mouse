/*
 * lefthand.h — 左手法（壁伝い走行）
 *
 * 「左手を壁につけたまま歩く」戦略。マップも経路計算も使わない
 * もっとも単純な迷路攻略法で，1区画ごとに止まりながら進む。
 *
 * 注意: ループ（回り道）のある迷路では，ゴールにたどり着けず
 * 永遠に同じところを回り続けることがある。それがなぜか，
 * どんな迷路なら必ず成功するかは docs/exercises/03_左手法.md で考える。
 */

#ifndef APP_LEFTHAND_H_
#define APP_LEFTHAND_H_

// 左手法でgoal座標を目指す（規定歩数を超えたら諦めて停止する）
void lefthand_run(void);

#endif /* APP_LEFTHAND_H_ */
