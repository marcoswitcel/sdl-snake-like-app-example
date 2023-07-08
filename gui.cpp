#include "./gui.h"

typedef struct GUI_Globals {
  int32_t mouse_x;
  int32_t mouse_y;
  int32_t mouse_clicked_x;
  int32_t mouse_clicked_y;
  bool clicked_in_this_frame;
  uint32_t timestamp_last_updated;
} GUI_Globals;

static GUI_Globals gui_globals = {
  .mouse_x = 0,
  .mouse_y = 0,
  .mouse_clicked_x = 0,
  .mouse_clicked_y = 0,
  .clicked_in_this_frame = false,
  .timestamp_last_updated = 0,
};

void gui_update_mouse_position(int32_t mouse_x, int32_t mouse_y, uint32_t timestamp_last_updated)
{
  gui_globals.mouse_x = mouse_x;
  gui_globals.mouse_y = mouse_y;
  gui_globals.timestamp_last_updated = timestamp_last_updated;
}

void gui_update_mouse_clicked(int32_t mouse_clicked_x, int32_t mouse_clicked_y, uint32_t timestamp_last_updated)
{
  gui_globals.mouse_clicked_x = mouse_clicked_x;
  gui_globals.mouse_clicked_y = mouse_clicked_y;
  gui_globals.clicked_in_this_frame = true;
  gui_globals.timestamp_last_updated = timestamp_last_updated;
}

void gui_clear_mouse_clicked(void)
{
  gui_globals.clicked_in_this_frame = false;
}

typedef struct Button {
  const char *text;
  bool hover;
  bool active;
  SDL_Rect target_area;
  SDL_Color background_color;
  SDL_Color highlight_background_color;
  uint32_t timestamp_last_updated;
} Button;

inline Button create_button(const char *text, SDL_Rect target_area, SDL_Color background_color, SDL_Color highlight_background_color)
{
  return {
    .text = text,
    .hover = false,
    .active = false,
    .target_area = target_area,
    .background_color = background_color,
    .highlight_background_color = highlight_background_color,
    .timestamp_last_updated = 0,
  };
}

inline void set_button_to_initial_state(Button *button, const char *text, SDL_Rect target_area, SDL_Color background_color, SDL_Color highlight_background_color)
{
  button->text = text;
  button->hover = false;
  button->active = false;
  button->target_area = target_area;
  button->background_color = background_color;
  button->highlight_background_color = highlight_background_color;
  button->timestamp_last_updated = 0;
}

inline SDL_Color color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
  return { .r = r, .g = g, .b = b, .a = a, };
}

inline SDL_Rect rect(int x, int y, int w, int h)
{
  return { .x = x, .y = y, .w = w, .h = h, };
}

bool is_point_inside_rect(int pX, int pY, int rX, int rY, int rW, int rH)
{
  bool result = (pX >= rX && pX <= (rX + rW)) && (pY >= rY && pY <= (rY + rH));
  // tracef("========================== %d %d %d %d %d %d", pX, pY, rY, rW, rH, result);
  return result;
}

bool button_was_clicked(Button &button)
{
  if (!gui_globals.clicked_in_this_frame) return false;

  return is_point_inside_rect(
    gui_globals.mouse_clicked_x,
    gui_globals.mouse_clicked_y,
    button.target_area.x,
    button.target_area.y,
    button.target_area.w,
    button.target_area.h
  );
}

bool is_updated(Button &button)
{
  return button.timestamp_last_updated == gui_globals.timestamp_last_updated;
}

void render_text(SDL_Renderer *renderer, const char * text, SDL_Rect target_area, TTF_Font *default_font, SDL_Color default_text_color)
{
  // @todo João, renderizando tudo como UTF8, pode ser mais lento checar isso.
  // Função não utf8 `TTF_RenderText_Solid`
  SDL_Surface *text_area_surface = TTF_RenderUTF8_Blended(default_font, text, default_text_color);
  SDL_Texture *text_area_texture = SDL_CreateTextureFromSurface(renderer, text_area_surface);
  // @note O tipo SDL_Surface tem o tamanho do texto renderizado caso queira centralizar
  //target_area.w = text_area_surface->w;
  //target_area.h = text_area_surface->h;
  SDL_RenderCopy(renderer, text_area_texture, NULL, &target_area);

  // @todo João, lento mas sem leaks...
  SDL_FreeSurface(text_area_surface);
  SDL_DestroyTexture(text_area_texture);
}

void button_update_state(Button &button)
{
  if (is_updated(button)) return;

  int x = button.target_area.x,
    y = button.target_area.y,
    w = button.target_area.w,
    h = button.target_area.h,
    mouse_x = gui_globals.mouse_x,
    mouse_y = gui_globals.mouse_y;

  button.hover = is_point_inside_rect(mouse_x, mouse_y, x, y, w, h);
  button.active = button_was_clicked(button);
  button.timestamp_last_updated = gui_globals.timestamp_last_updated;
}

void draw_button(SDL_Renderer *renderer, Button &button, TTF_Font *default_font, SDL_Color default_text_color)
{
  const int margin = 5; // @todo João, deixar mais flexível isso aqui

  SDL_Rect overlay = button.target_area;
  SDL_Color background_color = (button.hover) ? button.highlight_background_color : button.background_color;
  if (button.active)
  {
    background_color.a *= 0.8; // @note Temporário para gerar um diferença quando clicado
  }
  SDL_SetRenderDrawColor(renderer, background_color.r, background_color.g, background_color.b, background_color.a);
  SDL_RenderFillRect(renderer, &overlay);

  SDL_Rect target_area = { .x = button.target_area.x + margin, .y = button.target_area.y + margin, .w = button.target_area.w - 2 * margin, .h = button.target_area.h - 2 * margin };
  render_text(renderer, button.text, target_area, default_font, default_text_color);
}

void update_and_draw(SDL_Renderer *renderer, Button &button, TTF_Font *default_font, SDL_Color default_text_color)
{
  button_update_state(button);
  draw_button(renderer, button, default_font, default_text_color);
}
