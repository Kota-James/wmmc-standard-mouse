# テスト迷路

| ファイル | 内容 |
|---|---|
| `empty.maze` | 外周以外に壁がない迷路。アルゴリズムの最初の動作確認用。左手法では**解けない**（なぜかは課題3で） |
| `practice01.maze` `practice02.maze` | ループ（回り道）を含む練習迷路 |
| `practice03.maze` | ループのない「完全迷路」。左手法でも必ず解ける |

## 実大会の迷路で試す

過去の全日本大会などの迷路データが GitHub の
[micromouseonline/mazefiles](https://github.com/micromouseonline/mazefiles)
（`classic/` 配下、本シミュレータと同じテキスト形式）に多数まとまっている。

同リポジトリにはライセンス表記がないため、このリポジトリには同梱していない。
使いたい人は次のコマンドで各自の手元に取得する（`real/` は git 管理外）:

```sh
cd Sim/mazes
sh fetch_real.sh
../sim real/japan2019.maze
```

動作確認済み: `japan2019.txt`（全日本2019）で探索〜二次走行まで成功する。
