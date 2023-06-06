#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctime>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>


#include "./dev-utils.c"
#include "./export-level.cpp"
#include "./snake.c"


static constexpr int CELL_SIZE = 20;
static constexpr int ARENA_WIDTH = 30;
static constexpr int ARENA_HEIGHT = 30;
static constexpr int WIDTH = CELL_SIZE * ARENA_WIDTH;
static constexpr int HEIGHT = CELL_SIZE * ARENA_HEIGHT;

static unsigned UI_TICKS_PER_SECOND = 30; 
static unsigned TIMES_PER_SECOND = 4; 
static SDL_Color BG_COLOR    = { .r =  91, .g = 123, .b = 122, .a = 255 };
static SDL_Color SNAKE_COLOR = { .r =   0, .g = 255, .b =  46, .a = 255 };
static SDL_Color FRUIT_COLOR = { .r = 255, .g =  60, .b =  56, .a = 255 };
static SDL_Color WALL_COLOR  = { .r =  35, .g =  32, .b =  32, .a = 255 };
static std::deque<Vec2<unsigned>> CURRENT_DEFAULT_WALLS = std::deque<Vec2<unsigned>>();
static Vec2<unsigned> SNAKE_START_POSITION = { .x = 3, .y = 5, };

// Fontes
TTF_Font *default_font = NULL;
SDL_Color default_text_color = { 255, 255, 255, 255, };

// Cores
static const SDL_Color WHITE_COLOR = { .r = 255, .g = 255, .b = 255, .a = 255 };

typedef enum Game_State {
  PAUSED,
  RUNNING,
  MENU,
  GAME_OVER,
} Game_State;

typedef struct Context_Data {
  int32_t mouse_x = 0;
  int32_t mouse_y = 0;
  bool clicked = false;
  int32_t last_clicked_x = 0;
  int32_t last_clicked_y = 0;
  Snake_Dir snake_dir_input;
  Snake_Entity snake {
    .head = SNAKE_START_POSITION,
    .dir = NONE,
    .body = new std::deque<Vec2<unsigned>>(),
  };
  Arena arena {
    .width = ARENA_WIDTH,
    .height = ARENA_HEIGHT,
    .cell_size = CELL_SIZE,
    .walls = new std::deque<Vec2<unsigned>>(),
    .fruits = new std::deque<Vec2<unsigned>>(),
  };
  bool pointer_activated = false;
  Game_State state = RUNNING;
} Context_Data;

static Context_Data context = { };

bool try_parse_and_apply_color(SDL_Color &color, std::istringstream &iss)
{
  trace("Tentando parsear cores");
  int r, g, b, a;

  iss >> r;
  if (iss.fail() || r > SDL_MAX_UINT8) goto fail;
  
  iss >> g;
  if (iss.fail() || g > SDL_MAX_UINT8) goto fail;
  
  iss >> b;
  if (iss.fail() || b > SDL_MAX_UINT8) goto fail;
  
  iss >> a;
  if (iss.fail() || a > SDL_MAX_UINT8) goto fail;

  color.r = static_cast<uint8_t>(r);
  color.g = static_cast<uint8_t>(g);
  color.b = static_cast<uint8_t>(b);
  color.a = static_cast<uint8_t>(a);
  tracef("Cor consumida e aplicada\n%d %d %d %d", r, g, b, a);

  return true;

  fail: {
    trace("Não conseguiu aplicar a cor");
    return false;
  }
}

bool try_parse_and_apply_vec2(Vec2<unsigned> &position, std::istringstream &iss)
{
  trace("Tentando parsear cores");
  int x, y;

  iss >> x;
  if (iss.fail() || x > SDL_MAX_UINT8) goto fail;
  
  iss >> y;
  if (iss.fail() || y > SDL_MAX_UINT8) goto fail;
  
  trace("Vec2 consumido e aplicado");
  position.x = static_cast<uint8_t>(x);
  position.y = static_cast<uint8_t>(y);

  tracef("%d %d", x, y);

  return true;

  fail: {
    trace("Não conseguiu aplicar o Vec2");
    return false;
  }
}

bool try_parse_and_add_wall(std::istringstream &iss)
{
  Vec2<unsigned> position;

  if (try_parse_and_apply_vec2(position, iss)) {
    CURRENT_DEFAULT_WALLS.push_back(position);
    return true;
  }

  return false;
}

bool try_parse_and_apply_unsgined(unsigned &number, std::istringstream &iss)
{
  trace("Tentando parsear número");
  unsigned local_number;

  iss >> local_number;
  if (iss.fail()) {
    trace("Não conseguiu aplciar o número");
    return false;
  }
  
  trace("Número consumido e aplicado");
  number = local_number;

  tracef("%d", local_number);
  return true;
}

void toggle_pause_play(Context_Data *context)
{
  if (context->state == RUNNING) 
  {
    context->state = PAUSED;
  }
  else if (context->state == PAUSED) 
  {
    context->state = RUNNING;
  }
}

void reset_arena(Context_Data *context)
{
  context->state = RUNNING;

  // Esse é o estado da cobrinha quando inicia o nível
  context->snake_dir_input = NONE;
  context->snake.dir = NONE;
  context->snake.head = SNAKE_START_POSITION;
  context->snake.body->clear();

  // a lista de frutas inicia vazia
  context->arena.fruits->clear();

  // As paredes são copiadas da lista de paredes padrão para o level que está sendo executado
  context->arena.walls->clear();
  for (auto &it : CURRENT_DEFAULT_WALLS)
  {
    context->arena.walls->push_back(it);
  }
}

void load_ini_config()
{
  trace("Checando config.ini");

  std::ifstream file_handle("config.ini", std::ios::in);

  if (!file_handle.good())
  {
    trace("-- arquivo não encontrado, configurações padrão apenas")
    return;
  }

  // @todo João, precisamos de uma etapa dedicada a resetar os campos para o valor padrão
  // abaixom adicione um comando que resta uma das estruturas para o valor padrão, o mesmo
  // deve ser feito para cores e outros
  {
    CURRENT_DEFAULT_WALLS.clear();
  }

  std::string line;
  while (std::getline(file_handle, line))
  {
    trace("linha encontrada");
    std::istringstream iss(line);
    std::string command;
    iss >> command;

    bool success = !iss.fail();

    if (success) {
      trace("linha parseada");
      trace(command.c_str());

      // @todo João, finalizar, mais comandos aqui
      // @todo João, sanitizar os valores lidos para `TIMES_PER_SECOND` e `TIMES_PER_SECOND`
      if (get_name(BACKGROUD_COLOR_COMMAND) == command) { try_parse_and_apply_color(BG_COLOR, iss); }
      else if (get_name(SNAKE_COLOR_COMMAND) == command) { try_parse_and_apply_color(SNAKE_COLOR, iss); }
      else if (get_name(FRUIT_COLOR_COMMAND) == command) { try_parse_and_apply_color(FRUIT_COLOR, iss); }
      else if (get_name(WALL_COLOR_COMMAND) == command) { try_parse_and_apply_color(WALL_COLOR, iss); }
      else if (get_name(SNAKE_ARENA_TICK_COMMAND) == command) { try_parse_and_apply_unsgined(TIMES_PER_SECOND, iss); }
      else if (get_name(UI_TICK_COMMAND) == command) { try_parse_and_apply_unsgined(UI_TICKS_PER_SECOND, iss); }
      else if (get_name(SNAKE_START_POSITION_COMMAND) == command) { try_parse_and_apply_vec2(SNAKE_START_POSITION, iss); }
      else if (get_name(ADD_WALL_COMMAND) == command) { try_parse_and_add_wall(iss); }
    } else {
      trace("linha ignorada");
    }
  }

  file_handle.close();

  trace("Arquivo encontrado configurações carregadas");
}

Vec2<unsigned> index_to_arena_pos(unsigned index, Arena &arena)
{
  // @todo João, @note possivelmente bugado, não testado
  return Vec2<unsigned> {
    .x = index % arena.width,
    .y = index / arena.width,
  };
}

Vec2<unsigned> generate_new_fruit_position(Context_Data *context)
{
  Arena &arena = context->arena;
  Snake_Entity &snake = context->snake;
  
  int total_available_cells = arena.width * arena.height;
  total_available_cells -= snake.body->size() + 1;

  return index_to_arena_pos(rand() % total_available_cells, arena);
}

Vec2<unsigned> compute_next_snake_position(Context_Data *context)
{
  Vec2<unsigned> new_head_position = context->snake.head;
  if (context->snake.dir == LEFT || context->snake.dir == RIGHT)
  {
    signed newX = context->snake.head.x + (context->snake.dir == RIGHT ? 1 : -1);
    if (newX >= 0 && newX < (signed) context->arena.width)
    {
      new_head_position.x = newX;
    }
  }

  if (context->snake.dir == UP || context->snake.dir == DOWN)
  {
    signed newY = context->snake.head.y + (context->snake.dir == DOWN ? 1 : -1);
    if (newY >= 0 && newY < (signed) context->arena.height)
    {
      new_head_position.y = newY;
    } 
  }

  return new_head_position;
}

bool is_next_position_valid(Context_Data *context, const Vec2<unsigned> &new_head_position, bool include_fruits = false)
{
  auto head = context->snake.head;
  if (new_head_position.x == head.x && new_head_position.y == head.y) return false;
  
  for (auto &it : *context->snake.body)
  {
    if (it.x == new_head_position.x && it.y == new_head_position.y)
    {
      return false;
    }
  }

  // @todo João, manter o olho aqui, essa estratégia não escala para outros tipos,
  // talvez fazer uma lista única com "objetos" com tipo e posição, ou um hash espacial
  for (auto &it : *context->arena.walls)
  {
    if (it.x == new_head_position.x && it.y == new_head_position.y)
    {
      return false;
    }
  }

  if (include_fruits)
  {
    for (auto &it : *context->arena.fruits)
    {
      if (it.x == new_head_position.x && it.y == new_head_position.y)
      {
        return false;
      }
    }
  }

  return true;
}

bool export_current_arena_layout(Context_Data *context)
{
  // @todo João, terminar aqui
  // falta decidir como exportar a posição de começo do jogador
  // falta decidir se vai adicionar condições de vitórios
  // falta organizar em que arquivo, com que nome vai exportar
  std::stringstream stream;

  stream << "# Arquivo gerado pelo jogo\n";

  // Exportando paredes
  for (auto &it : *context->arena.walls)
  {
    stream << get_name(ADD_WALL_COMMAND) << " " << it.x << " " << it.y << "\n";
  }

  trace("Abrindo arquivo \"level_output.lvl\"para escrita");
  std::ofstream level_file("level_output.lvl", std::ios::binary);

  if (level_file.bad()) {
    trace("-- arquivo não aberto");
    return false;
  }

  level_file << stream.str();

  level_file.close();

  return true;
}

void handle_events_and_inputs(Context_Data *context, bool *should_quit)
{
  Snake_Dir snake_dir = NONE;
  SDL_Event event;
  // @todo João, por hora apenas a função `update` tá consumindo esse valor, por isso ela reseta o valor para false
  // quando faz uso.
  //context->clicked = false;
  
  // Processa eventos
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
  while (SDL_PollEvent(&event))
  {
    trace("Processando evento:");
    tracef("%d", event.type);
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
#ifdef DEV_CODE_ENABLED
              case SDL_SCANCODE_P: { toggle_pause_play(context); } break;
              case SDL_SCANCODE_R: { reset_arena(context); } break;
              case SDL_SCANCODE_E: { export_current_arena_layout(context); } break;
              case SDL_SCANCODE_L: { load_ini_config(); } break; // @todo João, avaliar se não há nenhum efeito negativo
              case SDL_SCANCODE_T: { context->pointer_activated = !context->pointer_activated; } break;
#endif
            }
          }
        }
      } break;
      case SDL_MOUSEBUTTONDOWN: {
        trace("Mouse button down");
        if (event.button.button == SDL_BUTTON_LEFT && event.button.state == SDL_PRESSED)
        {
          tracef("clicked: x %d, y %d", event.button.x, event.button.y);
          context->clicked = true;
          context->last_clicked_x = event.button.x;
          context->last_clicked_y = event.button.y;
        }
      } break;
      case SDL_MOUSEMOTION: {
        tracef("motion: x %d, y %d", event.motion.x, event.motion.y);
        context->mouse_x = event.motion.x;
        context->mouse_y = event.motion.y;
      } break;
    }
  }
#pragma GCC diagnostic pop

  // Processa inputs
  // @note Por hora como a UI atualiza mais que lógica do jogo estou considerando que o não input `NONE`
  // simboliza que não teve input nesse tick e o valor antigo do input deve ser mantido
  if (snake_dir != NONE)
  {
    context->snake_dir_input = snake_dir;
  }
}

void update(Context_Data *context)
{
  const unsigned arena_rect_size = context->arena.cell_size;

  if (context->arena.fruits->size() == 0)
  {
    // @todo João, acredito que não irá inserir frutas dentro de nenhum objeto,
    // porém conforme os espaços forem acabando esse método se tornará cada vez
    // mais custoso, ele está aqui só para deixar o gameplay inicial mais consistente
    // até implementar um algorítmo mais interessante para gerar a nova posição aletatória
    // sem precisar regerar a posição. 
    Vec2<unsigned> fruit_position = generate_new_fruit_position(context);
    while (!is_next_position_valid(context, fruit_position))
    {
      trace("[   >>>>> posição inválida regerando <<<<<<   ]");
      fruit_position = generate_new_fruit_position(context);
    }

    context->arena.fruits->push_front(fruit_position);
  }

  if (context->clicked)
  {
    if (context->pointer_activated)
    {
      Vec2<unsigned> wall_position = {
        .x = context->last_clicked_x / arena_rect_size,
        .y = context->last_clicked_y / arena_rect_size,
      };
      if (is_next_position_valid(context, wall_position, true))
      {
        trace("parede adicionada");
        context->arena.walls->push_front(wall_position);
      }
      else
      {
        trace("parede não adicionada, espaço ocupado");
      }
    }
    context->clicked = false;
  }

  if (context->state == PAUSED || context->state == GAME_OVER) return;

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
    case NONE: {} break;
  }

  Snake_Entity &snake = context->snake;
  Vec2<unsigned> new_head_position = compute_next_snake_position(context);
  bool is_heading_space_available = is_next_position_valid(context, new_head_position);

  if ((snake.dir == LEFT && snake.head.x == 0) ||
      (snake.dir == RIGHT && snake.head.x == context->arena.width - 1) ||
      (snake.dir == UP && snake.head.y == 0) ||
      (snake.dir == DOWN && snake.head.y == context->arena.height - 1)||
      !is_heading_space_available)
  {
    if (snake.dir != NONE)
    {
      context->state = GAME_OVER;
    }
    return;
  }

  // Salva última localização da head
  context->snake.body->push_front(context->snake.head);

  #ifndef NO_TRACE
    tracef("body size: %ld", context->snake.body->size());
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

static inline SDL_Rect makeSquare(const Vec2<unsigned> &pos, unsigned arena_rect_size)
{
  return {
    .x = (int) (pos.x * arena_rect_size),
    .y = (int) (pos.y * arena_rect_size),
    .w = (int) arena_rect_size,
    .h = (int) arena_rect_size,
  };
}

void render_scene(SDL_Renderer *renderer, Context_Data *context)
{
  const unsigned arena_rect_size = context->arena.cell_size;  

  // Seta o fundo do renderer
  SDL_SetRenderDrawColor(renderer, BG_COLOR.r, BG_COLOR.g, BG_COLOR.b, BG_COLOR.a);
  SDL_RenderClear(renderer);

  // Renderizando corpo
  unsigned i = 0;
  for (auto &ref : *context->snake.body)
  {
    // renderiza uma parte do corpo
    SDL_Rect snake_rect = makeSquare(ref, arena_rect_size);

    // @todo João, melhorar organização, fazer uma função que calcula uma porcentagem de um valor e retorna no mesmo tipo?
    if (i % 2) SDL_SetRenderDrawColor(renderer, SNAKE_COLOR.r, SNAKE_COLOR.g, SNAKE_COLOR.b, SNAKE_COLOR.a);
    else SDL_SetRenderDrawColor(renderer, (Uint8) ((float) SNAKE_COLOR.r * 0.96), (Uint8) ((float) SNAKE_COLOR.g * 0.96), (Uint8) ((float) SNAKE_COLOR.b * 0.96), SNAKE_COLOR.a);
 
    SDL_RenderFillRect(renderer, &snake_rect);
    i++;
  }

  // Renderiza o quadrado da posição da cabeça da cobra
  {
    SDL_Rect snake_rect = makeSquare(context->snake.head, arena_rect_size);

    SDL_SetRenderDrawColor(renderer, SNAKE_COLOR.r, SNAKE_COLOR.g, SNAKE_COLOR.b, SNAKE_COLOR.a);
    SDL_RenderFillRect(renderer, &snake_rect);
  }

  // Renderizando as frutas
  for (auto &ref : *context->arena.fruits)
  {
    SDL_Rect fruit_rect = makeSquare(ref, arena_rect_size);

    SDL_SetRenderDrawColor(renderer, FRUIT_COLOR.r, FRUIT_COLOR.g, FRUIT_COLOR.b, FRUIT_COLOR.a);
    SDL_RenderFillRect(renderer, &fruit_rect);
  }

  // Renderizando as paredes
  for (auto &ref : *context->arena.walls)
  {
    SDL_Rect fruit_rect = makeSquare(ref, arena_rect_size);

    SDL_SetRenderDrawColor(renderer, WALL_COLOR.r, WALL_COLOR.g, WALL_COLOR.b, WALL_COLOR.a);
    SDL_RenderFillRect(renderer, &fruit_rect);
  }

  // renderiza um quadrado na posição do mouse
  if (context->pointer_activated)
  {
    SDL_Rect rect = {
      .x = (int) ((context->mouse_x / arena_rect_size) * arena_rect_size), // isso só funciona por a divisão faz com que a parte fracionária seja perdida
      .y = (int) ((context->mouse_y / arena_rect_size) * arena_rect_size), // isso só funciona por a divisão faz com que a parte fracionária seja perdida
      .w = (int) arena_rect_size, .h = (int) arena_rect_size
    };

    SDL_SetRenderDrawColor(renderer, WHITE_COLOR.r, WHITE_COLOR.g, WHITE_COLOR.b, 50);
    SDL_RenderFillRect(renderer, &rect);
  }

  if (default_font)
  {
    char message_buffer[20];
    sprintf(message_buffer, "Pontos: %ld", context->snake.body->size());
    // @note João, esse processo é ineficiente, porém, por hora serve para testar
    // @todo Organizar um meio de lidar com texto e atualizar apenas quando a string muda, fazer um hash simples?
    // ou criar uma estrutura com flag `needs_update`?
    SDL_Surface *text_area_surface = TTF_RenderText_Solid(default_font, message_buffer, default_text_color);
    SDL_Texture *text_area_texture = SDL_CreateTextureFromSurface(renderer, text_area_surface);
    SDL_Rect    target_area = { .x = 4, .y = 0, .w = 140, .h = 30, };
    SDL_RenderCopy(renderer, text_area_texture, NULL, &target_area);

    SDL_FreeSurface(text_area_surface);
    SDL_DestroyTexture(text_area_texture);
  }

  if (default_font && context->state == GAME_OVER)
  {
    // Cobre toda a imagem com uma sobreposição preta com transparência
    {
      SDL_Rect overlay = { .x = 0, .y = 0, .w = WIDTH, .h = HEIGHT, };
      SDL_Color overlay_color = { 0, 0, 0, 160, };
      SDL_SetRenderDrawColor(renderer, overlay_color.r, overlay_color.g, overlay_color.b, overlay_color.a);
      SDL_RenderFillRect(renderer, &overlay);
    }

    SDL_Surface *text_area_surface = TTF_RenderText_Solid(default_font, "Fim do jogo", default_text_color);
    SDL_Texture *text_area_texture = SDL_CreateTextureFromSurface(renderer, text_area_surface);
    SDL_Rect target_area = { .x = WIDTH / 2 - 80, .y = HEIGHT / 2 - 15, .w = 160, .h = 30 };
    SDL_RenderCopy(renderer, text_area_texture, NULL, &target_area);
  }

  // Faz o swap do backbuffer com o buffer da tela?
  // @link https://wiki.libsdl.org/SDL2/SDL_RenderPresent
  SDL_RenderPresent(renderer);
}

int main(int argc, char **argv)
{
  printf("Olá mundo do SDL!\n");
  #ifdef DEV_CODE_ENABLED
  printf("=====================================\n");

  printf("|          DEV Build                |\n");
  printf("=====================================\n");
  #endif

  printf("Argumentos providos:\n[ ");
  for (int i = 0; i < argc; i++) printf("%s ", argv[i]);
  printf(" ]\n");

  time_t now = time(NULL);
  srand(now);
  printf("A seed é %ld\n", now);

  // Aplicar configurações caso existam
  load_ini_config();

  SDL_Window *window = NULL;

  if (SDL_Init(SDL_INIT_VIDEO) != 0)
  {
    fprintf(stderr, "SDL não pode inicializar corretamente: %s\n", SDL_GetError());
  }

  window = SDL_CreateWindow(
    "Jogo da Cobrinha: Feito com C++/SDL 2",
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

  if (TTF_Init() < 0)
  {
    fprintf(stderr, "A extensão TTF do SDL não pode inicializar corretamente: %s\n", TTF_GetError());
  }

  // @note Pode ser nulla por enquanto
  default_font = TTF_OpenFont("fonts/Roboto_Mono/static/RobotoMono-Medium.ttf", 28);

  if (default_font == NULL)
  {
    trace(">>> A fonte não pode ser carregada <<<");
  }

  // setup e configurações do SDL_Renderer
  // talvez existam motivos para por essa configuração dentro da função `render_scene`,
  // mas por hora dessa forma atende as necessidades
  {
    reset_arena(&context);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  }

  bool should_quit = false;
  uint32_t last_timestamp = 0;
  uint32_t accumulated_time = 0;
  while (!should_quit)
  {
    trace_timed("Entrando no loop");

    // Processa eventos e inputs
    handle_events_and_inputs(&context, &should_quit);

    {
      // @note João, não achei a versão 64 bits na minha instalação
      // https://wiki.libsdl.org/SDL2/SDL_GetTicks
      uint32_t current_timestamp = SDL_GetTicks();

      accumulated_time += current_timestamp - last_timestamp;
      last_timestamp = current_timestamp;

      if (accumulated_time > (1000 / TIMES_PER_SECOND))
      {
        accumulated_time = 0;
        // Lógica de atualização do jogo (regras)
        update(&context);
      }
    }
   
    // Renderiza
    render_scene(renderer, &context);
    
    // @note Solução temporária para mirar em atualizar 60 vezes por segundo
    SDL_Delay(1000 / UI_TICKS_PER_SECOND);
  }

  // Se chegar até aqui vai deixar a janela aberta por 5 segundos
  // SDL_Delay(5 * 1000);

  if (default_font)
  {
    trace("Fonte liberada");
    TTF_CloseFont(default_font);
  }
  TTF_Quit();
  trace("Extensão de fontes encerrada");

  // Supostamente devo chamar `SDL_DestroyWindow` em algum momento
  SDL_DestroyWindow(window);
  SDL_Quit();
  trace("Aplicação encerrada com sucesso");

  return EXIT_SUCCESS;
}
