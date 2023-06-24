#include "./export-level.h"

#define LEVEL_COMMAND_LIST(X_MACRO_NAME) \
    X_MACRO_NAME(BACKGROUD_COLOR) \
    X_MACRO_NAME(SNAKE_COLOR) \
    X_MACRO_NAME(FRUIT_COLOR) \
    X_MACRO_NAME(WALL_COLOR) \
    X_MACRO_NAME(UI_TICK) \
    X_MACRO_NAME(SNAKE_ARENA_TICK) \
    X_MACRO_NAME(SNAKE_START_POSITION) \
    X_MACRO_NAME(STARTUP_LEVEL) \
    X_MACRO_NAME(ADD_WALL) \
    X_MACRO_NAME(WIN_CONDITION_BY_GROWTH) \
    X_MACRO_NAME(NEXT_LEVEL) \
    X_MACRO_NAME(LOOSE_ON_HIT_WALL) \
    X_MACRO_NAME(LOOSE_ON_HIT_BORDERS) \
    X_MACRO_NAME(LOOSE_ON_HIT_BODY) 


typedef enum Level_Command {
#define ENUM_ENTRY(name) name ## _COMMAND,
    LEVEL_COMMAND_LIST(ENUM_ENTRY)
#undef ENUM_ENTRY
} Level_Command;

static const char* const level_command_names[] = {
#define MAKE_STRING_ENTRY(name) #name,
    LEVEL_COMMAND_LIST(MAKE_STRING_ENTRY)
#undef MAKE_STRING_ENTRY
};

inline const char* get_name(Level_Command command)
{
    return level_command_names[command];
}
