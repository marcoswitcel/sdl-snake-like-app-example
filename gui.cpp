#include "./gui.h"

typedef struct GUI_Globals {
  int32_t mouse_x;
  int32_t mouse_y;
  uint32_t timestamp_last_updated;
} GUI_Globals;

static GUI_Globals gui_globals = {
    .mouse_x = 0,
    .mouse_y = 0,
};

void gui_update_mouse_position(int32_t mouse_x, int32_t mouse_y, uint32_t timestamp_last_updated)
{
    gui_globals.mouse_x = mouse_x;
    gui_globals.mouse_y = mouse_y;
    gui_globals.timestamp_last_updated = timestamp_last_updated;
}

typedef struct Button {
  const char *text;
  bool hover;
  int width;
  int height;
  SDL_Color background_color;
  SDL_Color highlight_background_color;
  uint32_t timestamp_last_updated;
  // bool active; @todo João, começar com o básico
} Button;

bool point_inside_rect(int pX, int pY, int rX, int rY, int rW, int rH)
{
    bool result = (pX >= rX && pX <= (rX + rW)) && (pY >= rY && pY <= (rY + rH));
    // tracef("========================== %d %d %d %d %d %d", pX, pY, rY, rW, rH, result);
    return result;
}

bool is_updated(Button &button)
{
    return button.timestamp_last_updated == gui_globals.timestamp_last_updated;
}

void button_update_state(Button &button)
{
    if (is_updated(button)) return;

    button.hover = point_inside_rect(gui_globals.mouse_x, gui_globals.mouse_y, 0, 0, button.width, button.height);
}

// @todo João, incompleto, apenas esboçando
// @work-in-progress
void draw_button(SDL_Renderer *renderer, Button &button, TTF_Font *default_font, SDL_Color default_text_color)
{
  int xStart = 0, yStart = 0;
  button_update_state(button);

  SDL_Rect overlay = { .x = xStart, .y = yStart, .w = button.width, .h = button.height };
  SDL_Color background_color = (button.hover) ? button.highlight_background_color : button.background_color;
  SDL_SetRenderDrawColor(renderer, background_color.r, background_color.g, background_color.b, background_color.a);
  SDL_RenderFillRect(renderer, &overlay);

  SDL_Surface *text_area_surface = TTF_RenderUTF8_Blended(default_font, button.text, default_text_color);
  SDL_Texture *text_area_texture = SDL_CreateTextureFromSurface(renderer, text_area_surface);
  SDL_Rect target_area = { .x = xStart, .y = yStart, .w = button.width, .h = button.height };
  SDL_RenderCopy(renderer, text_area_texture, NULL, &target_area);
}
