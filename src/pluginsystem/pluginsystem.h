#pragma once

typedef struct {
    char* name;
    char* description;
    const void (*start)();
    const void (*stop)();
} IPlugin;
