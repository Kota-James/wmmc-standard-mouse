#!/bin/sh
# 実大会の迷路データを取得して real/ に保存する
#
# データ元: https://github.com/micromouseonline/mazefiles （classic/ 配下）
# 同リポジトリにはライセンス表記がないため、このリポジトリには同梱せず
# 各自がこのスクリプトで手元に取得する形にしている。
# 他の迷路が欲しければ上記リポジトリの classic/ を眺めて FILES に追加すればよい。
#
# 使い方: sh fetch_real.sh
#         ../sim real/japan2019.maze で走らせる

BASE="https://raw.githubusercontent.com/micromouseonline/mazefiles/master/classic"
FILES="japan2019 japan2018 japan2017ef japan2016-ef japan2015-ef"

mkdir -p real
for name in $FILES; do
    # リポジトリ側の命名ゆれに対応（.txt固定）
    if curl -sfL "$BASE/$name.txt" -o "real/$name.maze"; then
        echo "OK   $name"
    else
        rm -f "real/$name.maze"
        echo "SKIP $name (not found)"
    fi
done

if [ -x ../sim ]; then
    echo "done. 例: ../sim real/japan2019.maze"
else
    echo "done. シミュレータが未ビルドです。次を実行:"
    echo "  cd .. && make && ./sim mazes/real/japan2019.maze"
fi
