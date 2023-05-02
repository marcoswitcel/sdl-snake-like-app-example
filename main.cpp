#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>

#include "./dev-utils.c"

static constexpr int WIDTH = 600;
static constexpr int HEIGHT = 400;

int main(int argc, char **argv)
{
  printf("Olá mundo do SDL!\n");

  printf("Argumentos providos:\n[ ");
  for (int i = 0; i < argc; i++) printf("%s ", argv[i]);
  printf(" ]\n");

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

  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  if (!renderer)
  {
    fprintf(stderr, "O Renderer SDL não pode inicializar corretamente: %s\n", SDL_GetError());
    return EXIT_FAILURE;
  }

  // Seta o fundo do renderer
  SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

  SDL_RenderClear(renderer);

  SDL_Rect rect = {
    .x = 25, .y = 25,
    .w = 100, .h = 100
  };

  SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);

  SDL_RenderFillRect(renderer, &rect);

  SDL_RenderPresent(renderer);

  trace("aqui");
  // Se chegar até aqui vai deixar a janela aberta por 5 segundos
  SDL_Delay(5 * 1000);
  trace("aqui");

  // Supostamente devo chamar `SDL_DestroyWindow` em algum momento
  SDL_DestroyWindow(window);
  SDL_Quit();


  return EXIT_SUCCESS;
}