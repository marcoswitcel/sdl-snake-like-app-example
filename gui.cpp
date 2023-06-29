#include "./gui.h"

typedef struct Button {
    const char *text;
    bool hover;
    unsigned width;
    unsigned height;
    SDL_Color background_color;
    // bool active; @todo João, começar com o básico
} Button;

// @todo João, incompleto, apenas esboçando
// @work-in-progress
void draw_button(SDL_Renderer *renderer, const Button &button, TTF_Font *default_font, SDL_Color default_text_color)
{
    int xStart = 0, yStart = 0;

    SDL_Rect overlay = { .x = xStart, .y = yStart, .w = button.width, .h = button.height };
    SDL_Color background_color = button.background_color;
    SDL_SetRenderDrawColor(renderer, background_color.r, background_color.g, background_color.b, background_color.a);
    SDL_RenderFillRect(renderer, &overlay);

    SDL_Surface *text_area_surface = TTF_RenderUTF8_Blended(default_font, button.text, default_text_color);
    SDL_Texture *text_area_texture = SDL_CreateTextureFromSurface(renderer, text_area_surface);
    SDL_Rect target_area = { .x = xStart, .y = yStart, .w = button.width, .h = button.height };
    SDL_RenderCopy(renderer, text_area_texture, NULL, &target_area);
}
