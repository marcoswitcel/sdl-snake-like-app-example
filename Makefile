main: main.cpp
	$(CC) main.cpp -o target/main -Wall -Wextra -pedantic -std=c++2a -lSDL2 -lstdc++
run: main
	@echo "cd ./target"
	@echo ./main
	@echo "================"
	@cd ./target && ./main
	@echo "\n================\n"
