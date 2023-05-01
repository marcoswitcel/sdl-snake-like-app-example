#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>

static constexpr int WIDTH = 600;
static constexpr int HEIGHT = 400;

int main(int argc, char **argv)
{
  printf("Olá mundo do SDL!\n");

  for (; *argv; argv++) printf("%s\n", *argv);

  SDL_Window *window = NULL;

  if (SDL_Init(SDL_INIT_VIDEO) != 0)
  {
    fprintf(stderr, "SDL não pode inicializar corretamente: %s\n", SDL_GetError());
  }

  window = SDL_CreateWindow(
    "Jogo da Cobrinha: SLD2",
    SDL_WINDOWPOS_CENTERED,
    SDL_WINDOWPOS_CENTERED,
    WIDTH,
    HEIGHT,
    0
  );

  if (!window)
  {
    fprintf(stderr, "Janela SDL não pode inicializar corretamente: %s\n", SDL_GetError());
    return EXIT_FAILURE;
  }

  // Se chegar até aqui vai deixar a janela aberta por 5 segundos
  SDL_Delay(5 * 1000);

  // Supostamente devo chamar `SDL_DestroyWindow` em algum momento
  SDL_DestroyWindow(window);
  SDL_Quit();


  return EXIT_SUCCESS;
}