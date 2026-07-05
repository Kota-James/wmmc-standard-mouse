# 迷路探索シミュレータ（PC用）
# 使い方: make && ./sim mazes/practice01.maze

CC = gcc
CFLAGS = -I../Core/Inc -Wall -Wextra -O2 -g

SRC = sim_main.c \
      ../Core/Src/app/maze.c \
      ../Core/Src/app/solver.c

sim: $(SRC) $(wildcard ../Core/Inc/app/*.h ../Core/Inc/hw/*.h)
	$(CC) $(CFLAGS) $(SRC) -o $@

# 全ての迷路で探索〜二次走行が成功するか一括チェック（課題5の自動判定）
# mazes/real/ に実大会迷路を取得済みならそれもテスト対象になる（mazes/README.md参照）
test: sim
	@fail=0; for m in mazes/*.maze mazes/real/*.maze; do \
		[ -f "$$m" ] || continue; \
		if ./sim $$m > /dev/null; then echo "PASS: $$m"; \
		else echo "FAIL: $$m"; fail=1; fi; \
	done; exit $$fail

clean:
	rm -f sim

.PHONY: test clean
