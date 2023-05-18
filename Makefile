# usando g++ não precisa do parâmetro -lstdc++
CC=g++

main: main.cpp
	$(CC) main.cpp -o target/main -Wall -Wextra -pedantic -std=c++2a -lSDL2 -DDEV_CODE_ENABLED
run: main
	@echo "cd ./target"
	@echo ./main
	@echo "================"
	@cd ./target && ./main
	@echo "\n================\n"
