# Configurações de compilação
## usando g++ não precisa do parâmetro -lstdc++
CC = g++
CFLAGS = -Wall -Wextra -pedantic -std=c++2a
LFLAGS = -lSDL2 -lSDL2_ttf
DEF = -DDEV_CODE_ENABLED

# Configurações gerais
BUILD_FOLDER_NAME=target

build-folder-setup:
	@ mkdir -p $(BUILD_FOLDER_NAME)

copy-assets:
	mkdir -p ./$(BUILD_FOLDER_NAME)/fonts && cp -r ./fonts/* ./$(BUILD_FOLDER_NAME)/fonts

main: build-folder-setup main.cpp  
	$(CC) main.cpp -o $(BUILD_FOLDER_NAME)/main $(CFLAGS) $(LFLAGS) $(DEF)
run: main
	@echo "cd ./$(BUILD_FOLDER_NAME)"
	@echo ./main
	@echo "================"
	@cd ./$(BUILD_FOLDER_NAME) && ./main
	@echo "\n================\n"
