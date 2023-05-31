# usando g++ não precisa do parâmetro -lstdc++
CC = g++
CFLAGS = -Wall -Wextra -pedantic -std=c++2a
LFLAGS = -lSDL2 -lSDL2_ttf
DEF = -DDEV_CODE_ENABLED

main: main.cpp
	$(CC) main.cpp -o target/main $(CFLAGS) $(LFLAGS) $(DEF)
run: main
	@echo "cd ./target"
	@echo ./main
	@echo "================"
	@cd ./target && ./main
	@echo "\n================\n"
