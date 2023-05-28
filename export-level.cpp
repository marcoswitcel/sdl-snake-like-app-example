#include "./export-level.h"

#define LEVEL_COMMAND_LIST(X_MACRO_NAME) \
    X_MACRO_NAME(BACKGROUD_COLOR) \
    X_MACRO_NAME(SNAKE_COLOR) \
    X_MACRO_NAME(FRUIT_COLOR) \
    X_MACRO_NAME(WALL_COLOR) \
    X_MACRO_NAME(SNAKE_ARENA_TICK) \
    X_MACRO_NAME(SNAKE_START_POSITION) \
    X_MACRO_NAME(ADD_WALL)


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

inline const char* const get_name(Level_Command command)
{
    return level_command_names[command];
}
