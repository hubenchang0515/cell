#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <SDL2/SDL.h>

enum CellState
{
    CELL_STATE_EMPTY            = 0,
    CELL_STATE_EMPTY_ALIVE      = 1,
    CELL_STATE_EMPTY_WILL_DIE   = 2,
    CELL_STATE_EMPTY_WILL_BIRTH = 3,
};

typedef struct CellWorld CellWorld;

struct CellWorld
{
    int* map;
    int width;
    int height;
};

typedef struct ViewPort ViewPort;

struct ViewPort
{
    // top-left
    float x;
    float y;

    float scaling;
};

CellWorld* newCellWorld(int width, int height);
void setCell(CellWorld* world, int x, int y, int value);
int getCell(CellWorld* world, int x, int y);
int neighbors(CellWorld* world, int x, int y);
void deduce(CellWorld* world);
void drawCellWorld(SDL_Renderer* renderer, CellWorld* world);
CellWorld* init(const char* config);
void zoomViewport(ViewPort* viewport, float factor, double x, double y);
void moveViewPort(ViewPort* viewport, int dx, int dy, int width, int height);

#define INVOKE_SDL_FUNC(EXPR) if (EXPR < 0) {SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s\n", SDL_GetError());}

int main(int argc, char* argv[])
{
    if (argc > 2 || (argc == 2 && strstr(argv[1], "help") != NULL))
    {
        printf("Usage: %s [FILE]\n", argv[0]);
        printf("Create a cell world with FILE ('cell.cfg' in current directory by default).\n");
        return EXIT_FAILURE;
    }

    const char* config = argc == 1 ? "cell.cfg" : argv[1];
    CellWorld* world = init(config);

    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    SDL_Window* window = SDL_CreateWindow("CELL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, world->width, world->height, SDL_WINDOW_SHOWN);
    if (window == NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s\n", SDL_GetError());
        SDL_Quit();
        return EXIT_FAILURE;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return EXIT_FAILURE;
    }

    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_TARGET, world->width, world->height);
    if (texture == NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return EXIT_FAILURE;
    }

    ViewPort viewport = {0, 0, 1.0};
    Uint32 ticks = SDL_GetTicks();
    SDL_Event event;
    while (1)
    {
        if (SDL_PollEvent(&event) > 0)
        {
            if (event.type == SDL_QUIT)
            {
                break;
            }
            else if (event.type == SDL_MOUSEWHEEL)
            {
                int x,y;
                SDL_GetMouseState(&x, &y);
                zoomViewport(&viewport, event.wheel.y < 0 ? 2.0 : 0.5, x, y);
            }
            else if (event.type == SDL_MOUSEMOTION && (event.motion.state & SDL_BUTTON_LEFT))
            {
                moveViewPort(&viewport, event.motion.xrel, event.motion.yrel, world->width, world->height);
            }

            continue;
        }

        if (SDL_GetTicks() - ticks > 500)
        {
            ticks = SDL_GetTicks();
            deduce(world);
        }

        INVOKE_SDL_FUNC(SDL_SetRenderTarget(renderer, texture));
        INVOKE_SDL_FUNC(SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255));
        INVOKE_SDL_FUNC(SDL_RenderClear(renderer));
        INVOKE_SDL_FUNC(SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255));
        drawCellWorld(renderer, world);
        INVOKE_SDL_FUNC(SDL_SetRenderTarget(renderer, NULL));
        SDL_Rect rect = {viewport.x, viewport.y, world->width * viewport.scaling, world->height * viewport.scaling};
        INVOKE_SDL_FUNC(SDL_RenderCopy(renderer, texture, &rect, NULL));
        SDL_RenderPresent(renderer);
        SDL_Delay(20);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return EXIT_SUCCESS;
}

CellWorld* newCellWorld(int width, int height)
{
    CellWorld* world = SDL_malloc(sizeof(CellWorld));
    if (world == NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s\n", SDL_GetError());
        return NULL;
    }

    world->map = SDL_malloc(sizeof(int) * width * height);
    if (world->map == NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s\n", SDL_GetError());
        SDL_free(world);
        return NULL;
    }

    memset(world->map, 0, sizeof(int) * width * height);
    world->width = width;
    world->height = height;

    return world;
}

void setCell(CellWorld* world, int x, int y, int value)
{
    SDL_assert(world != NULL);
    SDL_assert(x < world->width);
    SDL_assert(y < world->height);

    world->map[y * world->width + x] = value;
}


int getCell(CellWorld* world, int x, int y)
{
    SDL_assert(world != NULL);
    SDL_assert(x < world->width);
    SDL_assert(y < world->height);

    return world->map[y * world->width + x];
}


int neighbors(CellWorld* world, int x, int y)
{
    SDL_assert(world != NULL);
    SDL_assert(x < world->width);
    SDL_assert(y < world->height);

    int ymin = y == 0 ? y : y - 1;
    int ymax = y == world->height - 1 ? y : y + 1;

    int xmin = x == 0 ? x : x - 1;
    int xmax = x == world->width ? x : x + 1;

    int n = 0;
    for (int dy = ymin; dy <= ymax; dy++)
    {
        for (int dx = xmin; dx <= xmax; dx++)
        {
            if (dy == y && dx == x)
                continue;

            int state = getCell(world, dx, dy);
            if (state == CELL_STATE_EMPTY_ALIVE || state == CELL_STATE_EMPTY_WILL_DIE)
                n++;
        }
    }

    return n;
}

void deduce(CellWorld* world)
{
    SDL_assert(world != NULL);

    for (int y = 0; y < world->height; y++)
    {
        for (int x = 0; x < world->width; x++)
        {
            if (getCell(world, x, y) == CELL_STATE_EMPTY_ALIVE && (neighbors(world, x, y) < 2 || neighbors(world, x, y) > 3))
            {
                setCell(world, x, y, CELL_STATE_EMPTY_WILL_DIE);
            }
            else if(getCell(world, x, y) == CELL_STATE_EMPTY && neighbors(world, x, y) == 3)
            {
                setCell(world, x, y, CELL_STATE_EMPTY_WILL_BIRTH);
            }
        }
    }

    for (int y = 0; y < world->height; y++)
    {
        for (int x = 0; x < world->width; x++)
        {
            if (getCell(world, x, y) == CELL_STATE_EMPTY_WILL_BIRTH)
            {
                setCell(world, x, y, CELL_STATE_EMPTY_ALIVE);
            }
            else if(getCell(world, x, y) == CELL_STATE_EMPTY_WILL_DIE)
            {
                setCell(world, x, y, CELL_STATE_EMPTY);
            }
        }
    }
}

void drawCellWorld(SDL_Renderer* renderer, CellWorld* world)
{
    SDL_assert(renderer != NULL);
    SDL_assert(world != NULL);

    for (int y = 0; y < world->height; y++)
    {
        for (int x = 0; x < world->width; x++)
        {
            if (getCell(world, x, y) == CELL_STATE_EMPTY_ALIVE)
            {
                SDL_RenderDrawPoint(renderer, x, y);
            }
        }
    }
}


CellWorld* init(const char* config)
{
    SDL_assert(config != NULL);

    FILE* fp = fopen(config, "rb");
    if (fp == NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s\n", strerror(errno));
        return NULL;
    }

    int x, y;
    if ((fscanf(fp, "%d%*[ \r\t\n]%d%*[ \r\t\n]", &x, &y) == EOF))
    {
        fclose(fp);
        return NULL;
    }

    CellWorld* world = newCellWorld(x, y);
    if (world == NULL)
    {
        fclose(fp);
        return NULL;
    }

    do
    {
        if (fscanf(fp, "%d%*[ \r\t\n]%d%*[ \r\t\n]", &x, &y) == EOF)
            break;

        setCell(world, x, y, 1);
    }while(!feof(fp));

    fclose(fp);
    return world;
}

void zoomViewport(ViewPort* viewport, float factor, double x, double y)
{
    SDL_assert(viewport != NULL);

    if (viewport->scaling * factor > 1)
    {
        viewport->scaling = 1.0;
        viewport->x = 0;
        viewport->y = 0;
    }

    int oldX = x * viewport->scaling;
    int oldY = y * viewport->scaling;

    int newX = oldX * factor;
    int newY = oldY * factor;

    viewport->x = viewport->x + (oldX - newX);
    viewport->y = viewport->y + (oldY - newY);
    viewport->scaling *= factor;
}

void moveViewPort(ViewPort* viewport, int dx, int dy, int width, int height)
{
    viewport->x -= dx * viewport->scaling;
    viewport->y -= dy * viewport->scaling;

    if (viewport->x < 0) 
        viewport->x = 0;

    if (viewport->y < 0) 
        viewport->y = 0;

    if (viewport->x > width * (1 - viewport->scaling)) 
        viewport->x = width * (1 - viewport->scaling);

    if (viewport->y > height * (1 - viewport->scaling))
        viewport->y = height * (1 - viewport->scaling);
}