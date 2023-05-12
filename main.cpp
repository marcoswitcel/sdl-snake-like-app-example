#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#include "./dev-utils.c"
#include "./snake.c"

static constexpr int WIDTH = 700;
static constexpr int HEIGHT = 700;

typedef struct Context_Data {
  int32_t mouse_x;
  int32_t mouse_y;
  Snake_Dir snake_dir_input;
  Snake_Entity snake {
    .head = { .x = 3, .y = 5, },
    .dir = NONE,
    .body = new std::deque<Vec2<unsigned>>(),
  };
  Arena arena {
    .width = 30,
    .height = 30,
    .cell_size = 20,
    .fruits = new std::deque<Vec2<unsigned>>(),
  };
} Context_Data;

static Context_Data context = { };

Vec2<unsigned> compute_next_snake_position(Context_Data *context)
{
  Vec2<unsigned> new_head_position = context->snake.head;
  if (context->snake.dir == LEFT || context->snake.dir == RIGHT)
  {
    auto newX = context->snake.head.x + (context->snake.dir == RIGHT ? 1 : -1);
    if (newX >= 0 && newX < context->arena.width)
    {
      new_head_position.x = newX;
    }
  }

  if (context->snake.dir == UP || context->snake.dir == DOWN)
  {
    auto newY = context->snake.head.y + (context->snake.dir == DOWN ? 1 : -1);
    if (newY >= 0 && newY < context->arena.height)
    {
      new_head_position.y = newY;
    } 
  }

  return new_head_position;
}

bool is_next_snake_move_valid(Context_Data *context, const Vec2<unsigned> &new_head_position)
{
  for (auto &it : *context->snake.body)
  {
    if (it.x == new_head_position.x && it.y == new_head_position.y)
    {
      return false;
    }
  }

  return true;
}

void handle_events_and_inputs(Context_Data *context, bool *should_quit)
{
  Snake_Dir snake_dir = NONE;
  SDL_Event event;
  
  // Processa eventos
  while (SDL_PollEvent(&event))
  {
    trace("Processando evento:");
    printf("%d\n", event.type);
    switch (event.type)
    {
      case SDL_QUIT: *should_quit = true; break;
      case SDL_KEYDOWN: {
        // Por hora apenas o keydown me interessa
        trace("Keydown");
        if (event.key.state == SDL_PRESSED) // Keydown deve sempre ser keypressed, mas...
        {
          if (!event.key.repeat) // Pula as repetições no futuro
          {
            switch (event.key.keysym.scancode)
            {
              case SDL_SCANCODE_W: { snake_dir = UP;    } break;
              case SDL_SCANCODE_S: { snake_dir = DOWN;  } break;
              case SDL_SCANCODE_A: { snake_dir = LEFT;  } break;
              case SDL_SCANCODE_D: { snake_dir = RIGHT; } break;
            }
          }
        }
      } break;
      case SDL_MOUSEMOTION: {
        printf("motion: x%d, y%d\n", event.motion.x, event.motion.y);
        context->mouse_x = event.motion.x;
        context->mouse_y = event.motion.y;
      } break;
    }
  }

  // Processa inputs
  context->snake_dir_input = snake_dir;
}

void update(Context_Data *context)
{
  // Atualiza direção da cobrinha seguindo algumas restrições
  switch (context->snake_dir_input)
  {
    case UP: {
      if (context->snake.dir != DOWN)
      {
        context->snake.dir = UP;
      }
    } break;
    case DOWN: {
      if (context->snake.dir != UP)
      {
        context->snake.dir = DOWN;
      }
    } break;
    case LEFT: {
      if (context->snake.dir != RIGHT)
      {
        context->snake.dir = LEFT;
      }
    } break;
    case RIGHT: {
      if (context->snake.dir != LEFT)
      {
        context->snake.dir = RIGHT;
      }
    } break;
  }

  Snake_Entity &snake = context->snake;
  Vec2<unsigned> new_head_position = compute_next_snake_position(context);
  bool is_heading_space_available = is_next_snake_move_valid(context, new_head_position);

  if ((snake.dir == LEFT && snake.head.x == 0) ||
      (snake.dir == RIGHT && snake.head.x == context->arena.width - 1) ||
      (snake.dir == UP && snake.head.y == 0) ||
      (snake.dir == DOWN && snake.head.y == context->arena.height - 1)||
      !is_heading_space_available) return;

  // Salva última localização da head
  context->snake.body->push_front(context->snake.head);

  #ifndef NO_TRACE
    printf("body size: %ld", context->snake.body->size());
  #endif

  // Movimento e espaço restringido é garantido aqui
  context->snake.head = new_head_position;

  // Checa se houve contato com um fruta, caso sim, a cobrinha vai crescer
  bool should_grow = false;
  int i = 0;
  for (auto &ref : *context->arena.fruits)
  {
    if (ref.x == context->snake.head.x && ref.y == context->snake.head.y)
    {
      should_grow = true;
      context->arena.fruits->erase(context->arena.fruits->begin() + i);
      break;
    }
    i++;
  }
  if (!should_grow)
  {
    context->snake.body->pop_back();
  }
}

void render_scene(SDL_Renderer *renderer, Context_Data *context)
{
  // Seta o fundo do renderer
  SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

  SDL_RenderClear(renderer);

  constexpr unsigned rect_size = 50;
  SDL_Rect rect = {
    .x = context->mouse_x - rect_size/2, .y = context->mouse_y - rect_size/2,
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
  context.arena.fruits->push_front(Vec2<unsigned> { .x = 3, .y = 6, });
  context.arena.fruits->push_front(Vec2<unsigned> { .x = 7, .y = 3, });

  bool should_quit = false;
  while (!should_quit)
  {
    trace_timed("Entrando no loop");

    // Processa eventos e inputs
    handle_events_and_inputs(&context, &should_quit);

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
