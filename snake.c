#include "./snake.h"

template<class T>
struct Vec2
{
    T x;
    T y;
};

typedef enum Snake_Dir {
    UP,
    DOWN,
    LEFT,
    RIGHT,
    NONE,
} Snake_Dir;

typedef struct Arena {
    unsigned width;
    unsigned height;
    signed cell_size;
    std::deque<Vec2<unsigned>> *walls;
    std::deque<Vec2<unsigned>> *fruits; 
} Arena;

typedef struct Snake_Entity
{
    Vec2<unsigned> head;
    Snake_Dir dir;
    std::deque<Vec2<unsigned>> *body; 
} Snake_Entity;
