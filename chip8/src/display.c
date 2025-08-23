#include <SDL2/SDL.h>
// init, update, destroy


int main() {
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(64*10, 32*10, 0, &window, &renderer);
    SDL_RenderSetScale(renderer, 10, 10);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawPoint(renderer, 64/2, 32/2);

    SDL_RenderPresent(renderer);
    SDL_Delay(10000);
    SDL_Quit();

    return 0;
}
