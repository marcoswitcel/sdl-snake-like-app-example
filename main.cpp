#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#include "./dev-utils.c"
#include "./snake.c"

static constexpr int WIDTH = 600;
static constexpr int HEIGHT = 400;

typedef struct Context_Data {
  int32_t x;
  int32_t y;
  Snake_Entity snake;
} Context_Data;

static Context_Data context = { 0 };

void render_scene(SDL_Renderer *renderer, Context_Data *context)
{
  // Seta o fundo do renderer
  SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

  SDL_RenderClear(renderer);

  constexpr unsigned rect_size = 50;
  SDL_Rect rect = {
    .x = context->x - rect_size/2, .y = context->y - rect_size/2,
    .w = rect_size, .h = rect_size
  };

  SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
  SDL_RenderFillRect(renderer, &rect);

  // Renderiza o quadrado da posição da cabeça da cobra
  constexpr unsigned snake_rect_size = 10;
  SDL_Rect snake_rect = {
    .x = context->snake.head.x * snake_rect_size, .y = context->snake.head.y * snake_rect_size,
    .w = snake_rect_size, .h = snake_rect_size
  };

  SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
  SDL_RenderFillRect(renderer, &snake_rect);

  // Faz o swap do backbuffer com o buffer da tela?
  // @link https://wiki.libsdl.org/SDL2/SDL_RenderPresent
  SDL_RenderPresent(renderer);
}

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

  SDL_Event event;
  bool should_quit = false;

  // Não consegui fazer a initialização em uma linha
  context.snake.head.x = 3;
  context.snake.head.x = 5;
  context.snake.dir.x = true;
  context.snake.dir.y = true;

  while (!should_quit)
  {
    trace_timed("Entrando no loop");

    // Processa inputs
    while (SDL_PollEvent(&event))
    {
      trace("Processando evento:");
      printf("%d\n", event.type);
      switch (event.type)
      {
        case SDL_QUIT: should_quit = true; break;
        case SDL_KEYDOWN: trace("Keydown"); break;
        case SDL_MOUSEMOTION: {
          printf("motion: x%d, y%d\n", event.motion.x, event.motion.y);
          context.x = event.motion.x;
          context.y = event.motion.y;
        } break;
      }

    }

    // Lógica de atualização
    // @note temporário para testar a renderização
    context.snake.head.x++;

    // Renderiza
    render_scene(renderer, &context);

    // @note Solução temporária para aliviar a CPU e manter a lógica rodando na velocidade certa
    constexpr uint32_t TIMES_PER_SECOND = 8; 
    SDL_Delay(1000 / TIMES_PER_SECOND);
  }

  // Se chegar até aqui vai deixar a janela aberta por 5 segundos
  // SDL_Delay(5 * 1000);

  // Supostamente devo chamar `SDL_DestroyWindow` em algum momento
  SDL_DestroyWindow(window);
  SDL_Quit();


  return EXIT_SUCCESS;
}
