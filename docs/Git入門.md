# Git / GitHub 入門 — マウス開発で使いこなす

Git はソフトウェア開発のデファクトスタンダードで、研究室でも企業でも必ず使う。
マウス開発を通して「自然に手が動く」レベルまで習慣化するのがこの部の方針。
ここでは**マウス開発で実際に使う操作だけ**を、使う場面とセットで説明する。

## 0. Git は何を解決するのか

- **タイムマシン**: 「昨日まで走ってたのに」→ 動いていた時点に正確に戻れる
- **実験の安全網**: 大改造を別ブランチでやれば、失敗しても本体は無傷
- **説明責任**: 「いつ・何を・なぜ変えたか」が全部残る。パラメータ調整の記録にもなる
- **共同作業**: 先輩のレビューを受けたり、他人の変更を取り込んだりする土台

## 1. 最初の1回だけやる設定

```sh
git config --global user.name "自分の名前"
git config --global user.email "GitHubに登録したメール"
git config --global init.defaultBranch main
```

GitHub アカウントを作り、リポジトリへの push 認証を設定する
（HTTPS + [Git Credential Manager](https://github.com/git-ecosystem/git-credential-manager)
が簡単。SSH 鍵でもよい）。

## 2. 自分のリポジトリを作る（このプロジェクトの始め方）

1. 標準プログラムのリポジトリページで **「Use this template」→「Create a new repository」**
   （フォークではなくテンプレート。自分がオーナーの独立リポジトリになる）
2. 手元に取得: `git clone https://github.com/<自分>/<リポジトリ名>.git`

## 3. 日常サイクル（これが体に染みつけばOK）

```sh
git status                  # 今何が変わっているか（迷ったらまずこれ）
git diff                    # 変更内容を行単位で確認
git add Core/Src/app/wall.c # コミットに含めるファイルを選ぶ
git commit -m "壁判定のしきい値比較を実装"
git push                    # GitHubに反映
```

### コミットの作法（部のルール）

- **動く区切りごとに小さく刻む**。「課題1が終わった」ではなく
  「前壁の判定を実装」「しきい値を調整」のように
- メッセージは**何をしたかが分かる1文**。「修正」「あああ」は禁止
- ビルドが通る状態でコミットする（壊れた状態を履歴に残さない）
- 走行パラメータの調整もコミットに残す。「大会前日に戻したい」が本当に起きる

## 4. ブランチ — 実験を安全にやる道具

```sh
git switch -c try-pd-control   # 新しいブランチを切って移動
# ...実験する。だめでも main は無傷...
git switch main                # 戻る
git merge try-pd-control       # うまくいったら取り込む
git branch -d try-pd-control   # 用済みブランチは消す
```

使いどころ: ゲイン調整の実験、課題の下書き、大会用パラメータの分離など。
「main は常に走る状態、実験はブランチ」を習慣にすると事故が減る。

このリポジトリに最初からあるブランチ:

- `main` … 自分の開発本線
- `reference` … 模範解答（見比べ用。自分では変更しない）

## 5. GitHub — 見せる・見てもらう

### 先輩にレビューしてもらう（任意だが推奨）

1. リポジトリの Settings → Collaborators で先輩を追加
2. 課題をブランチで作業して push:
   `git push -u origin exercise-04`
3. GitHub 上で **Pull Request** を作成。「何を実装したか」「自信がない箇所」を書く
4. コメントがついたら修正して push（PR に自動で反映される）→ マージ

レビューは減点会ではない。「もっと良い書き方を知る」最短ルートなので、
質問を書き込むくらいの気持ちで使うこと。

### PR の説明に書くとよいこと

- やったこと（1〜3行）
- 確認したこと（例: `Sim/ make test` 全PASS、実機で直進確認）
- 相談したいこと

## 6. 困ったときの復旧コマンド

| 状況 | コマンド |
|---|---|
| ファイルの変更を捨てて元に戻したい | `git restore <ファイル>` |
| add を取り消したい | `git restore --staged <ファイル>` |
| 直前のコミットメッセージを直したい | `git commit --amend` |
| 直前のコミット自体をやり直したい（push前） | `git reset --soft HEAD~1` |
| push 済みのコミットを取り消したい | `git revert <コミットID>`（打ち消しコミットを作る） |
| 過去の状態を一時的に見たい | `git switch -d <コミットID>`（見終わったら `git switch main`） |
| 何かがおかしい、履歴を確認したい | `git log --oneline --graph` |

**鉄則: 分からなくなったら、消したり force push したりする前に先輩に見せる。**
Git はほぼ何でも復旧できるが、`--force` と `rm -rf .git` だけは取り返しがつかない。

## 7. やってはいけないこと

- パスワード・API キー・個人情報をコミットする（push した瞬間に世界公開と思うこと）
- ビルド生成物（`build/`, `Debug/`）をコミットする（`.gitignore` 済み。増えたら追記）
- 共有ブランチへの `git push --force`（自分しか使わないブランチ以外では禁止）
- コンフリクトマーカー（`<<<<<<<`）が残ったままのコミット

## 8. もっと学ぶ

- [Learn Git Branching](https://learngitbranching.js.org/?locale=ja)（ブラウザで動かして学べる。ブランチの理解に最適）
- [GitHub Docs（日本語）](https://docs.github.com/ja)
- [Pro Git 日本語版](https://git-scm.com/book/ja/v2)（無料の決定版。2章と3章だけでも）
