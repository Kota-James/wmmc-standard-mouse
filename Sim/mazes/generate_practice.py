#!/usr/bin/env python3
"""練習迷路の生成スクリプト（mazefiles互換テキスト形式）

日本のクラシック競技ルールに合わせた制約で生成する:
  - 16×16区画、外周は壁
  - スタート区画(0,0)は北だけが開いている
  - ゴールは中央2×2領域（(7,7)〜(8,8)）。内壁のない部屋で入口は1箇所
  - ゴール以外に「内壁のない2×2の空き部屋」を作らない
    （ゴールと紛らわしい見た目になるため）

使い方: python3 generate_practice.py
"""
import random

N = 16
GOAL = (7, 7)  # ゴール領域の南西角

# h[y][x] = 区画(x,y)の北壁 / v[y][x] = 区画(x,y)の西壁


def render(h, v, path):
    lines = []
    for fy in range(N - 1, -1, -1):
        row = ""
        for x in range(N):
            row += "o" + ("---" if h[fy][x] else "   ")
        lines.append(row + "o")
        row = ""
        for x in range(N):
            row += ("|" if v[fy][x] else " ") + "   "
        lines.append(row + "|")
    lines.append("o" + "o".join(["---"] * N) + "o")
    with open(path, "w") as f:
        f.write("\n".join(lines) + "\n")


def neighbors_open(h, v, x, y):
    """(x,y)から壁なしで行ける隣接区画"""
    out = []
    if y < N - 1 and not h[y][x]:
        out.append((x, y + 1))
    if y > 0 and not h[y - 1][x]:
        out.append((x, y - 1))
    if x < N - 1 and not v[y][x + 1]:
        out.append((x + 1, y))
    if x > 0 and not v[y][x]:
        out.append((x - 1, y))
    return out


def all_connected(h, v):
    """全区画が(0,0)から到達可能か（壁を足したとき迷路を分断していないか）"""
    seen = {(0, 0)}
    stack = [(0, 0)]
    while stack:
        x, y = stack.pop()
        for nx, ny in neighbors_open(h, v, x, y):
            if (nx, ny) not in seen:
                seen.add((nx, ny))
                stack.append((nx, ny))
    return len(seen) == N * N


def open_2x2_rooms(h, v):
    """内壁が全くない2×2ブロックの南西角座標を列挙する"""
    rooms = []
    for y in range(N - 1):
        for x in range(N - 1):
            if (not h[y][x] and not h[y][x + 1] and not v[y][x + 1] and
                    not v[y + 1][x + 1]):
                rooms.append((x, y))
    return rooms


def generate(seed, path, extra_openings=40):
    random.seed(seed)

    # ---- 深さ優先探索で完全迷路（ループなし・全区画連結）を掘る ----
    h = [[True] * N for _ in range(N)]
    v = [[True] * N for _ in range(N)]
    visited = [[False] * N for _ in range(N)]
    stack = [(0, 0)]
    visited[0][0] = True
    while stack:
        x, y = stack[-1]
        cand = []
        if y < N - 1 and not visited[y + 1][x]:
            cand.append(("N", x, y + 1))
        if y > 0 and not visited[y - 1][x]:
            cand.append(("S", x, y - 1))
        if x < N - 1 and not visited[y][x + 1]:
            cand.append(("E", x + 1, y))
        if x > 0 and not visited[y][x - 1]:
            cand.append(("W", x - 1, y))
        if not cand:
            stack.pop()
            continue
        d, nx, ny = random.choice(cand)
        if d == "N":
            h[y][x] = False
        elif d == "S":
            h[ny][nx] = False
        elif d == "E":
            v[y][nx] = False
        elif d == "W":
            v[y][x] = False
        visited[ny][nx] = True
        stack.append((nx, ny))

    # ---- 壁を抜いてループ（回り道）を作る。大会迷路らしくするため ----
    removed = 0
    while removed < extra_openings:
        x, y = random.randrange(N), random.randrange(N)
        if random.random() < 0.5 and y != N - 1 and h[y][x]:
            h[y][x] = False
            removed += 1
        elif 0 < x < N and v[y][x]:
            v[y][x] = False
            removed += 1

    # ---- 競技ルールの制約 ----
    for x in range(N):
        h[N - 1][x] = True  # 外周（北）
    for y in range(N):
        v[y][0] = True  # 外周（西）
    h[0][0] = False  # スタートは北だけ開いている
    v[0][1] = True

    # ゴール領域: 内壁なしの2×2の部屋、入口は1箇所
    gx, gy = GOAL
    h[gy][gx] = h[gy][gx + 1] = False  # 部屋の内側の横壁
    v[gy][gx + 1] = v[gy + 1][gx + 1] = False  # 部屋の内側の縦壁
    boundary = [("h", gy - 1, gx), ("h", gy - 1, gx + 1),  # 南
                ("h", gy + 1, gx), ("h", gy + 1, gx + 1),  # 北
                ("v", gy, gx), ("v", gy + 1, gx),          # 西
                ("v", gy, gx + 2), ("v", gy + 1, gx + 2)]  # 東
    for kind, a, b in boundary:
        (h if kind == "h" else v)[a][b] = True
    kind, a, b = random.choice(boundary)  # 入口を1箇所だけ開ける
    (h if kind == "h" else v)[a][b] = False

    if not all_connected(h, v):
        raise RuntimeError(f"seed={seed}: ゴール部屋を閉じたら分断された。別のseedを使うこと")

    # ---- ゴール以外の「2×2の空き部屋」を潰す ----
    # 偶然できた空き部屋はゴールと紛らわしいので、内壁を1枚足して部屋でなくす。
    # 壁を足すと迷路が分断されることがあるため、連結性を確認しながら選ぶ
    for _ in range(200):
        rooms = [r for r in open_2x2_rooms(h, v) if r != GOAL]
        if not rooms:
            break
        x, y = rooms[0]
        inner = [("h", y, x), ("h", y, x + 1),
                 ("v", y, x + 1), ("v", y + 1, x + 1)]
        random.shuffle(inner)
        for kind, a, b in inner:
            grid = h if kind == "h" else v
            grid[a][b] = True
            if all_connected(h, v):
                break  # この壁で確定
            grid[a][b] = False  # 分断されるなら戻して別の壁を試す
        else:
            raise RuntimeError(f"seed={seed}: ({x},{y})の空き部屋を潰せなかった")
    else:
        raise RuntimeError(f"seed={seed}: 空き部屋の除去が収束しなかった")

    render(h, v, path)
    print(f"{path}: 空き部屋={open_2x2_rooms(h, v)}（ゴールのみであること）")


def generate_perfect(seed, path):
    """壁抜きなしの完全迷路（課題3の左手法用。ループが無いので必ず壁伝いで解ける）"""
    random.seed(seed)
    h = [[True] * N for _ in range(N)]
    v = [[True] * N for _ in range(N)]
    visited = [[False] * N for _ in range(N)]
    stack = [(0, 0)]
    visited[0][0] = True
    while stack:
        x, y = stack[-1]
        cand = []
        if y < N - 1 and not visited[y + 1][x]:
            cand.append(("N", x, y + 1))
        if y > 0 and not visited[y - 1][x]:
            cand.append(("S", x, y - 1))
        if x < N - 1 and not visited[y][x + 1]:
            cand.append(("E", x + 1, y))
        if x > 0 and not visited[y][x - 1]:
            cand.append(("W", x - 1, y))
        if not cand:
            stack.pop()
            continue
        d, nx, ny = random.choice(cand)
        if d == "N":
            h[y][x] = False
        elif d == "S":
            h[ny][nx] = False
        elif d == "E":
            v[y][nx] = False
        elif d == "W":
            v[y][x] = False
        visited[ny][nx] = True
        stack.append((nx, ny))
    for x in range(N):
        h[N - 1][x] = True
    for y in range(N):
        v[y][0] = True
    h[0][0] = False
    v[0][1] = True
    render(h, v, path)
    print(f"{path}: 完全迷路（ループなし）")


if __name__ == "__main__":
    generate(2023, "practice01.maze")
    generate(2026, "practice02.maze")
    generate_perfect(90, "practice03.maze")
