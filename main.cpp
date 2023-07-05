#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
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
#include "./gui.cpp"


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
  WINNER,
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
    .win_condition = { .type = NO_TYPE, .data = {}, },
    .next_level = NULL,
    .current_level_file_name = NULL,
    .loose_condition = LOOSE_ON_HIT_BODY | LOOSE_ON_HIT_BORDERS | LOOSE_ON_HIT_WALL,
  };
  bool pointer_activated = false;
  Game_State state = MENU;
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

bool try_parse_and_apply_boolean(bool &flag, std::istringstream &iss)
{
  trace("Tentando parsear boolean");
  bool boolean_value;

  iss >> boolean_value;
  if (iss.fail())
  {
    trace("Não conseguiu parsear a boolean");
    return false;
  };
  
  flag = boolean_value > 0;
  tracef("Boolean consumido e aplicado\n%d", boolean_value);

  return true;
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

bool try_parse_and_apply_unsigned(unsigned &number, std::istringstream &iss)
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

char * copy(const char *source)
{
  size_t size = strlen(source);
  char * copy = (char *) malloc(size + 1);
  memcpy(copy, source, size + 1);
  return copy;
}

bool try_parse_and_apply_file_name(const char **dest, std::istringstream &iss)
{
  std::string file_name;
  iss >> file_name;

  if (iss.fail())
  {
    trace("Não conseguiu ler o nome do arquivo de level");
    return false;
  }

  tracef("Nome do arquivo consumido de level: \n%s", file_name.c_str());
  
  char * copy = (char *) malloc(file_name.size() + 1);
  memcpy(copy, file_name.c_str(), file_name.size() + 1);
  *dest = copy;

  return true;
}

bool load_level_data(Context_Data &context, const char *file_name)
{
  std::ifstream file_handle(file_name, std::ios::in);

  if (!file_handle.good())
  {
    tracef("-- arquivo de level não encontrado, comando ignorado %s", get_name(STARTUP_LEVEL_COMMAND));
    return false;
  }

  // @todo João, precisamos de uma etapa dedicada a resetar os campos para o valor padrão
  // abaixo adicione alguns comandos que para resetar umas das estruturas para o valor padrão
  // talvez isso possa acontecer sempre aqui, mas precisa ser validado
  {
    // relacionadas ao level
    CURRENT_DEFAULT_WALLS.clear();
    free((void *) context.arena.next_level);
    context.arena.next_level = NULL;
    context.arena.win_condition.type = NO_TYPE;
    context.arena.loose_condition = LOOSE_ON_HIT_BODY | LOOSE_ON_HIT_BORDERS | LOOSE_ON_HIT_WALL;
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

      if (get_name(SNAKE_START_POSITION_COMMAND) == command) { try_parse_and_apply_vec2(SNAKE_START_POSITION, iss); }
      else if (get_name(ADD_WALL_COMMAND) == command) { try_parse_and_add_wall(iss); }
      else if (get_name(WIN_CONDITION_BY_GROWTH_COMMAND) == command) {
        if (try_parse_and_apply_unsigned(context.arena.win_condition.data.grow_number, iss))
        {
          context.arena.win_condition.type = BY_GROWING;
        }
      }
      else if (get_name(NEXT_LEVEL_COMMAND) == command) {
        const char *old_file_name = context.arena.next_level;
        if (try_parse_and_apply_file_name(&context.arena.next_level, iss))
        {
          free((void *) old_file_name);
        }
      } else if (get_name(LOOSE_ON_HIT_WALL_COMMAND) == command) {
        bool flag = false;
        if (try_parse_and_apply_boolean(flag, iss))
        {
          if (flag)
          {
            context.arena.loose_condition |= LOOSE_ON_HIT_WALL;
          }
          else
          {
            context.arena.loose_condition ^= LOOSE_ON_HIT_WALL;
          }
        }
      } else if (get_name(LOOSE_ON_HIT_BORDERS_COMMAND) == command) {
        bool flag = false;
        if (try_parse_and_apply_boolean(flag, iss))
        {
          if (flag)
          {
            context.arena.loose_condition |= LOOSE_ON_HIT_BORDERS;
          }
          else
          {
            context.arena.loose_condition ^= LOOSE_ON_HIT_BORDERS;
          }
        }
      } else if (get_name(LOOSE_ON_HIT_BODY_COMMAND) == command) {
        bool flag = false;
        if (try_parse_and_apply_boolean(flag, iss))
        {
          if (flag)
          {
            context.arena.loose_condition |= LOOSE_ON_HIT_BODY;
          }
          else
          {
            context.arena.loose_condition ^= LOOSE_ON_HIT_BODY;
          }
        }
      }
    } else {
      trace("linha ignorada");
    }
  }
  // salvando nome do level carregado
  {
    free((void *)context.arena.current_level_file_name);
    context.arena.current_level_file_name = copy(file_name);
  }
  return true;
}

bool try_parse_and_load_level(std::istringstream &iss)
{
  std::string file_name;
  iss >> file_name;

  if (iss.fail())
  {
    trace("Não conseguiu ler o nome do arquivo de level");
    return false;
  }

  tracef("Nome do arquivo consumido de level: \n%s", file_name.c_str());

  return load_level_data(context, file_name.c_str());
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
  // @note Imagino que poderia deixar o estado inalterado se não fosse WINNER ou GAME_OVER
  if (context->state != MENU) context->state = RUNNING;

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

  // @todo João, precisamos de uma etapa dedicada a resetar o estado para as cores padrões aqui

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

      // @todo João, sanitizar os valores lidos para `TIMES_PER_SECOND` e `TIMES_PER_SECOND`
      if (get_name(BACKGROUD_COLOR_COMMAND) == command) { try_parse_and_apply_color(BG_COLOR, iss); }
      else if (get_name(SNAKE_COLOR_COMMAND) == command) { try_parse_and_apply_color(SNAKE_COLOR, iss); }
      else if (get_name(FRUIT_COLOR_COMMAND) == command) { try_parse_and_apply_color(FRUIT_COLOR, iss); }
      else if (get_name(WALL_COLOR_COMMAND) == command) { try_parse_and_apply_color(WALL_COLOR, iss); }
      else if (get_name(SNAKE_ARENA_TICK_COMMAND) == command) { try_parse_and_apply_unsigned(TIMES_PER_SECOND, iss); }
      else if (get_name(UI_TICK_COMMAND) == command) { try_parse_and_apply_unsigned(UI_TICKS_PER_SECOND, iss); }
      else if (get_name(STARTUP_LEVEL_COMMAND) == command) { try_parse_and_load_level(iss); }
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

bool is_colliding_with_snake_body(Context_Data *context, const Vec2<unsigned> &new_head_position)
{
  for (auto &it : *context->snake.body)
  {
    if (it.x == new_head_position.x && it.y == new_head_position.y)
    {
      return true;
    }
  }

  return false;
}

bool is_colliding_with_walls(Context_Data *context, const Vec2<unsigned> &new_head_position)
{
  for (auto &it : *context->arena.walls)
  {
    if (it.x == new_head_position.x && it.y == new_head_position.y)
    {
      return true;
    }
  }

  return false;
}

bool is_next_position_valid(Context_Data *context, const Vec2<unsigned> &new_head_position, bool include_fruits = false)
{
  auto head = context->snake.head;
  if (new_head_position.x == head.x && new_head_position.y == head.y) return false;
  
  if (is_colliding_with_snake_body(context, new_head_position))
  {
    return false;
  }

  // @todo João, manter o olho aqui, essa estratégia não escala para outros tipos,
  // talvez fazer uma lista única com "objetos" com tipo e posição, ou um hash espacial
  if (is_colliding_with_walls(context, new_head_position))
  {
    return false;
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
  std::stringstream stream;

  stream << "# Arquivo gerado pelo jogo\n";

  // Exportando paredes
  stream << "\n## Paredes \n\n";
  for (auto &it : *context->arena.walls)
  {
    stream << get_name(ADD_WALL_COMMAND) << " " << it.x << " " << it.y << "\n";
  }

  // Exportando posição inicial
  stream << "\n## Posição inicial \n\n";
  stream << get_name(SNAKE_START_POSITION_COMMAND) << " " << SNAKE_START_POSITION.x << " " << SNAKE_START_POSITION.y << "\n";

  // Exportando condições de GAME_OVER
  stream << "\n## Condições de GAME_OVER \n\n";
  {
    bool flag_value = (context->arena.loose_condition & LOOSE_ON_HIT_WALL) > 0;
    stream << get_name(LOOSE_ON_HIT_WALL_COMMAND) << " " << flag_value << "\n";

    flag_value = (context->arena.loose_condition & LOOSE_ON_HIT_BORDERS) > 0;
    stream << get_name(LOOSE_ON_HIT_BORDERS_COMMAND) << " " << flag_value << "\n";

    flag_value = (context->arena.loose_condition & LOOSE_ON_HIT_BODY) > 0;
    stream << get_name(LOOSE_ON_HIT_BODY_COMMAND) << " " << flag_value << "\n";
  }

  // Exportando next level e condições de vitória
  stream << "\n## Next level e condições de vitória \n\n";
  if (context->arena.next_level)
  {
    stream << get_name(NEXT_LEVEL_COMMAND) << " " << context->arena.next_level << "\n";
  }
  if (context->arena.win_condition.type == BY_GROWING)
  {
    stream << get_name(WIN_CONDITION_BY_GROWTH_COMMAND) << " " << context->arena.win_condition.data.grow_number << "\n";
  }

  const char *file_name = context->arena.current_level_file_name ? context->arena.current_level_file_name : "level_output.lvl";
  tracef("Abrindo arquivo \"%s\"para escrita", file_name);
  std::ofstream level_file(file_name, std::ios::binary);

  if (level_file.bad()) {
    trace("-- arquivo não aberto");
    return false;
  }

  level_file << stream.str();

  level_file.close();

  return true;
}

void load_next_level_and_start(Context_Data *context)
{
  if (!context->arena.next_level) return;

  // @todo João, não curti ter que copiar para rodar o método, mas dentro do método ele usa 
  // o valor do file_name e faz o free do atributo next_level
  const char *copied_file_name = copy(context->arena.next_level);
  bool loaded = load_level_data(*context, copied_file_name);
  free((void *)copied_file_name);

  if (!loaded) return;

  reset_arena(context);
}

void handle_return(Context_Data *context)
{
  if (context->state == WINNER && context->arena.next_level)
  {
    load_next_level_and_start(context);
  }
}

void handle_events_and_inputs(Context_Data *context, bool *should_quit)
{
  Snake_Dir snake_dir = NONE;
  SDL_Event event;
  // @todo João, por hora apenas a função `update` tá consumindo esse valor, por isso ela reseta o valor para false
  // quando faz uso.
  //context->clicked = false;

  // @note Talvez mover esse trecho para outra parte do código, uma parte dedicada a resetar estado antes de processar os inputs?
  gui_clear_mouse_clicked();
  
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
              case SDL_SCANCODE_RETURN: { handle_return(context); } break;
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
          gui_update_mouse_clicked(event.button.x, event.button.y, event.common.timestamp);
        }
      } break;
      case SDL_MOUSEMOTION: {
        tracef("motion: x %d, y %d", event.motion.x, event.motion.y);
        context->mouse_x = event.motion.x;
        context->mouse_y = event.motion.y;
        // Duplicado para isolar @todo João, analisar se de fato faz sentido
        gui_update_mouse_position(event.motion.x, event.motion.y, event.common.timestamp);
      } break;
    }
  }
#pragma GCC diagnostic pop

  // Processa inputs
  // @note Por hora como a UI atualiza mais que lógica do jogo estou considerando que o não input `NONE`
  // simboliza que não teve input nesse tick e o valor antigo do input deve ser mantido
  if (context->state != MENU && snake_dir != NONE)
  {
    context->snake_dir_input = snake_dir;
  }
}

void check_win_condition(Context_Data * context)
{
  bool won = false;

  // implementações de cheque
  switch (context->arena.win_condition.type)
  {
    case BY_GROWING: {
      won = context->snake.body->size() >= context->arena.win_condition.data.grow_number;
    } break;
    case NO_TYPE: {} break;
    default: {} break;
  }

  if (won) context->state = WINNER;
}

bool remove_value_from_deque(std::deque<Vec2<unsigned int>> &deque, Vec2<unsigned> &compared_to)
{
  int i = 0;
  for (auto &it : deque)
  {
    if (it.x == compared_to.x && it.y == compared_to.y)
    {
      deque.erase(deque.begin() + i);
      return true;
    }
    i++;
  }

  return false;
}

void update(Context_Data *context)
{
  const unsigned arena_rect_size = context->arena.cell_size;

  check_win_condition(context);

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
        if (remove_value_from_deque(*context->arena.walls, wall_position))
        {
          trace("parede removida");
        }
        else
        {
          trace("parede não adicionada, espaço ocupado");
        }
      }
    }
    context->clicked = false;
  }

  if (context->state == PAUSED || context->state == GAME_OVER || context->state == WINNER) return;

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
  const Vec2<unsigned> new_head_position = compute_next_snake_position(context);
  bool is_in_same_space = new_head_position.x == snake.head.x && new_head_position.y == snake.head.y;
  bool collided_with_walls = is_colliding_with_walls(context, new_head_position);
  bool collided_with_snake_body = is_colliding_with_snake_body(context, new_head_position);
  bool collided_with_border = (snake.dir == LEFT && snake.head.x == 0) ||
    (snake.dir == RIGHT && snake.head.x == context->arena.width - 1) ||
    (snake.dir == UP && snake.head.y == 0) ||
    (snake.dir == DOWN && snake.head.y == context->arena.height - 1);

  // @todo João, falta detectar colisão com paredes num geral e com o próprio corpo
  // @todo João, outro ponto, esse conjunto de ifs controla se o fluxo deve ser interrompido
  // e se o estado deve ser mudado para game_over, mas são questões diferentes, talvez fosse
  // melhor separar.
  if (collided_with_border || collided_with_walls || collided_with_snake_body || is_in_same_space)
  {
    if (snake.dir != NONE)
    {
      if (collided_with_border)
      {
        if (context->arena.loose_condition & LOOSE_ON_HIT_BORDERS)
        {
          context->state = GAME_OVER;
        }
      } else if (collided_with_walls) {
        if (context->arena.loose_condition & LOOSE_ON_HIT_WALL)
        {
          context->state = GAME_OVER;
        }
      } else if (collided_with_snake_body) {
        if (context->arena.loose_condition & LOOSE_ON_HIT_BODY)
        {
          context->state = GAME_OVER;
        }
      }
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

  if (!remove_value_from_deque(*context->arena.fruits, context->snake.head))
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

// Cobre toda a imagem com uma sobreposição preta com transparência
static inline void draw_overlay(SDL_Renderer *renderer)
{
  SDL_Rect overlay = { .x = 0, .y = 0, .w = WIDTH, .h = HEIGHT, };
  SDL_Color overlay_color = { 0, 0, 0, 160, };
  SDL_SetRenderDrawColor(renderer, overlay_color.r, overlay_color.g, overlay_color.b, overlay_color.a);
  SDL_RenderFillRect(renderer, &overlay);
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
    draw_overlay(renderer);

    SDL_Surface *text_area_surface = TTF_RenderText_Solid(default_font, "Fim do jogo", default_text_color);
    SDL_Texture *text_area_texture = SDL_CreateTextureFromSurface(renderer, text_area_surface);
    SDL_Rect target_area = { .x = WIDTH / 2 - 80, .y = HEIGHT / 2 - 15, .w = 160, .h = 30 };
    SDL_RenderCopy(renderer, text_area_texture, NULL, &target_area);

    SDL_FreeSurface(text_area_surface);
    SDL_DestroyTexture(text_area_texture);
  }

  if (default_font && context->state == WINNER)
  {
    draw_overlay(renderer);

    // Mensagem de vitória
    {
      SDL_Surface *text_area_surface = TTF_RenderUTF8_Blended(default_font, "Você ganhou", default_text_color);
      SDL_Texture *text_area_texture = SDL_CreateTextureFromSurface(renderer, text_area_surface);
      SDL_Rect target_area = { .x = WIDTH / 2 - 80, .y = HEIGHT / 2 - 15, .w = 160, .h = 30 };
      SDL_RenderCopy(renderer, text_area_texture, NULL, &target_area);

      SDL_FreeSurface(text_area_surface);
      SDL_DestroyTexture(text_area_texture);
    }

    // apresenta qual o próximo nível
    if (context->arena.next_level)
    {
      char message_buffer[200]; // @note possível overflow aqui?
      sprintf(message_buffer, "Próximo level: %s", context->arena.next_level);

      SDL_Surface *text_area_surface = TTF_RenderUTF8_Blended(default_font, message_buffer, default_text_color);
      SDL_Texture *text_area_texture = SDL_CreateTextureFromSurface(renderer, text_area_surface);
      SDL_Rect target_area = { .x = WIDTH / 2 - 80, .y = HEIGHT / 2 + 15, .w = (int) strlen(context->arena.next_level) * 30, .h = 30 };
      SDL_RenderCopy(renderer, text_area_texture, NULL, &target_area);

      SDL_FreeSurface(text_area_surface);
      SDL_DestroyTexture(text_area_texture);
    }
    // GUI 
    if (context->arena.next_level)
    {
      Button button = {
        .text = "Próximo",
        .hover = false,
        .active = false,
        .target_area = {
          .x = 225,
          .y = 400,
          .w = 160,
          .h = 30,
        },
        .background_color = {
          .r = 0,
          .g = 100,
          .b = 0,
          .a = 255,
        },
        .highlight_background_color = {
          .r = 0,
          .g = 150,
          .b = 0,
          .a = 255,
        },
        .timestamp_last_updated = 0,
      };
      update_and_draw(renderer, button, default_font, default_text_color);

      if (button_was_clicked(button))
      {
        load_next_level_and_start(context);
      }
    }
  }

  // Faz o swap do backbuffer com o buffer da tela?
  // @link https://wiki.libsdl.org/SDL2/SDL_RenderPresent
  SDL_RenderPresent(renderer);
}

// @todo João, terminar de construir um menu aqui
// @work-in-progress
void update_and_draw_menu(SDL_Renderer *renderer, Context_Data *context, bool *should_quit)
{
  // Seta o fundo do renderer
  SDL_SetRenderDrawColor(renderer, BG_COLOR.r, BG_COLOR.g, BG_COLOR.b, BG_COLOR.a);
  SDL_RenderClear(renderer);
  constexpr int button_width = 160;
  constexpr int button_height = 30;
  constexpr int marginY = 15;
  int startY = HEIGHT / 2;
  int startX = WIDTH / 2;

  Button buttons[] = {
    create_button("Jogar", rect(0, 0, button_width, button_height), WALL_COLOR, color(WALL_COLOR.r, WALL_COLOR.g, WALL_COLOR.b, WALL_COLOR.a * 0.5)),
    create_button("Sair", rect(0, 0, button_width, button_height), WALL_COLOR, color(WALL_COLOR.r, WALL_COLOR.g, WALL_COLOR.b, WALL_COLOR.a * 0.5)),
  };
  Button &button_play = buttons[0];
  Button &button_quit = buttons[1];
  
  constexpr auto total_buttons = (sizeof(buttons) / sizeof(buttons[0]));
  constexpr auto buttons_total_height = total_buttons * button_height + total_buttons * marginY;
  for (auto &it : buttons)
  {
    it.target_area.x = startX - button_width / 2;
    it.target_area.y = startY - button_height / 2 - buttons_total_height / 2;

    startY += button_height + marginY;

    update_and_draw(renderer, it, default_font, default_text_color);
  }

  if (button_was_clicked(button_play))
  {
    context->state = RUNNING;
  }

  if (button_was_clicked(button_quit))
  {
    *should_quit = true;
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

    if (context.state == MENU)
    {
      update_and_draw_menu(renderer, &context, &should_quit);
    }
    else
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

      // Renderiza
      render_scene(renderer, &context);
    }
    
    SDL_Delay(1000 / UI_TICKS_PER_SECOND);
  }

  if (default_font)
  {
    trace("Fonte liberada");
    TTF_CloseFont(default_font);
  }
  TTF_Quit();
  trace("Extensão de fontes encerrada");

  SDL_DestroyWindow(window);
  SDL_Quit();
  trace("Aplicação encerrada com sucesso");

  return EXIT_SUCCESS;
}
