main: main.cpp
	$(CC) main.cpp -o target/main -Wall -Wextra -pedantic -std=c++2a -lSDL2
run: main
	@echo ./target/main
	@echo "================"
	@./target/main
	@echo "\n================\n"
