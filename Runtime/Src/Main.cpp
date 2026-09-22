#include <EngineCore/Log.h>
#include <EnginePlatform/Window.h>
#include <iostream>

int main(int argc, char** argv) {
    std::cout << "[Eunoia Runtime] Initializing standalone runtime (headless/game)..." << std::endl;
    // Runtime entry point without Editor/ImGui dependencies.
    // Loads cooked assets and runs scene simulation.
    std::cout << "[Eunoia Runtime] Standalone runtime stub initialized successfully." << std::endl;
    return 0;
}
