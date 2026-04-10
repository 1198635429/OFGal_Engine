// Copyright 2026 MrSeagull. All Rights Reserved.
#include "AppController.h"

int main(int argc, char* argv[]) {
    AppController app;
    const char* initialPath = (argc > 1) ? argv[1] : nullptr;
    return app.Run(initialPath);
}