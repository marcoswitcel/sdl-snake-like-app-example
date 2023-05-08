#include "./snake.h"

template<class T>
struct Vec2
{
    T x;
    T y;
};

typedef struct Arena {
    unsigned width;
    unsigned height;
} Arena;

typedef struct Snake_Entity
{
    Vec2<unsigned> head;
    Vec2<signed> dir;
} Snake_Entity;
