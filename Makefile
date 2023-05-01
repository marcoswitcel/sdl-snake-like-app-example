main: main.cpp
	$(CC) main.cpp -o bin/main -Wall -Wextra -pedantic -lSDL2
run: main
	@echo ./bin/main
	@echo "================"
	@./bin/main
	@echo "\n================\n"
