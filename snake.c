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
    signed cell_size;
    std::deque<Vec2<unsigned>> *fruits; 
} Arena;

typedef struct Snake_Entity
{
    Vec2<unsigned> head;
    Vec2<signed> dir;
    std::deque<Vec2<unsigned>> *body; 
} Snake_Entity;
