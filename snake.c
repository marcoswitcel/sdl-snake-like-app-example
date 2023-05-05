#include "./snake.h"

template<class T>
struct Vec2
{
    T x;
    T y;
};

typedef struct Snake_Entity
{
    Vec2<unsigned> head;
    Vec2<bool> dir;
} Snake_Entity;
