#include "Modules/ModuleManager.h"

// The primary game module. DeepCore ships no content assets: the world, its lighting and its
// materials are all constructed in C++ at startup, so there is nothing here to load.
IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, DeepCore, "DeepCore");
