#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#include "./dev-utils.c"
#include "./snake.c"

static constexpr int WIDTH = 700;
static constexpr int HEIGHT = 700;

typedef struct Context_Data {
  int32_t x;
  int32_t y;
  Snake_Entity snake {
    .head = { .x = 3, .y = 5, },
    .dir = { .x = 0, .y = 0, },
    .body = new std::deque<Vec2<unsigned>>(),
  };
  Arena arena {
    .width = 30,
    .height = 30,
    .cell_size = 20,
    .fruits = new std::deque<Vec2<unsigned>>(),
  };
} Context_Data;

static Context_Data context = { 0 };

void update(Context_Data *context)
{
  context->snake.body->push_front(context->snake.head);
  context->snake.body->pop_back();
  #ifndef NO_TRACE
    printf("body size: %d", context->snake.body->size());
  #endif

  // Movimento e espaço restringido é garantido aqui
  if (context->snake.dir.x)
  {
    auto newX = context->snake.head.x + (context->snake.dir.x > 0 ? 1 : -1);
    if (newX >= 0 && newX < context->arena.width)
    {
      context->snake.head.x = newX;
    }
  }

  if (context->snake.dir.y)
  {
    auto newY = context->snake.head.y + (context->snake.dir.y > 0 ? 1 : -1);
    if (newY >= 0 && newY < context->arena.height)
    {
      context->snake.head.y = newY;
    } 
  }
}

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

  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_RenderFillRect(renderer, &rect);

  // Renderizando corpo @note em andamento
  for (auto &ref : *context->snake.body)
  {
    // Renderiza o quadrado da posição da cabeça da cobra
    const unsigned snake_rect_size = context->arena.cell_size;
    SDL_Rect snake_rect = {
      .x = ref.x * snake_rect_size, .y = ref.y * snake_rect_size,
      .w = snake_rect_size, .h = snake_rect_size
    };

    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderFillRect(renderer, &snake_rect);
  }

  // Renderiza o quadrado da posição da cabeça da cobra
  const unsigned snake_rect_size = context->arena.cell_size;
  SDL_Rect snake_rect = {
    .x = context->snake.head.x * snake_rect_size, .y = context->snake.head.y * snake_rect_size,
    .w = snake_rect_size, .h = snake_rect_size
  };

  SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
  SDL_RenderFillRect(renderer, &snake_rect);

  // Renderizando as frutas
  for (auto &ref : *context->arena.fruits)
  {
    const unsigned fruit_size = context->arena.cell_size;
    SDL_Rect fruit_rect = {
      .x = ref.x * fruit_size, .y = ref.y * fruit_size,
      .w = fruit_size, .h = fruit_size
    };

    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
    SDL_RenderFillRect(renderer, &fruit_rect);
  }

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

  context.arena.fruits->push_front(Vec2<unsigned> { .x = 0, .y = 0, });
  context.arena.fruits->push_front(Vec2<unsigned> { .x = 7, .y = 6, });

  SDL_Event event;
  bool should_quit = false;
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
        case SDL_KEYDOWN: {
          // Por hora apenas o keydown me interessa
          trace("Keydown");
          if (event.key.state == SDL_PRESSED) // Keydown deve sempre ser keypressed, mas...
          {
            if (!event.key.repeat) // Pula as repetições no futuro
            {
              context.snake.dir.x = 0;
              context.snake.dir.y = 0;
              switch (event.key.keysym.scancode)
              {
                case SDL_SCANCODE_W: { context.snake.dir.y = -1; } break;
                case SDL_SCANCODE_S: { context.snake.dir.y = 1; } break;
                case SDL_SCANCODE_A: { context.snake.dir.x = -1; } break;
                case SDL_SCANCODE_D: { context.snake.dir.x = 1; } break;
              }
            }
          }
        } break;
        case SDL_MOUSEMOTION: {
          printf("motion: x%d, y%d\n", event.motion.x, event.motion.y);
          context.x = event.motion.x;
          context.y = event.motion.y;
        } break;
      }

    }

    // Lógica de atualização
    update(&context);
   
    // Renderiza
    render_scene(renderer, &context);

    // @note Solução temporária para aliviar a CPU e manter a lógica rodando na velocidade certa
    constexpr uint32_t TIMES_PER_SECOND = 4; 
    SDL_Delay(1000 / TIMES_PER_SECOND);
  }

  // Se chegar até aqui vai deixar a janela aberta por 5 segundos
  // SDL_Delay(5 * 1000);

  // Supostamente devo chamar `SDL_DestroyWindow` em algum momento
  SDL_DestroyWindow(window);
  SDL_Quit();


  return EXIT_SUCCESS;
}
