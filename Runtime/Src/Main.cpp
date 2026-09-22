#include <EngineCore/Log.h>
#include <EnginePlatform/Window.h>
#include <EnginePlatform/InputSystem.h>
#include <EngineScene/Scene.h>
#include <EngineScene/SceneSerializer.h>
#include <EngineScene/ScreenPrint.h>
#include <EngineAssets/AssetManager.h>
#include <EngineRenderer/Camera.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <filesystem>

void AddEngineLog(const std::string& category, const std::string& message, int level) {
    std::cout << "[" << category << "] " << message << std::endl;
}

int main(int argc, char** argv) {
    std::cout << "========================================================\n";
    std::cout << "[Eunoia Runtime] Initializing Standalone Game Runtime...\n";
    std::cout << "========================================================\n";

    // 1. Initialize Window
    EnginePlatform::WindowConfig winConfig;
    winConfig.title = "Eunoia Engine - Standalone Game";
    winConfig.width = 1280;
    winConfig.height = 720;
    winConfig.resizable = true;

    EnginePlatform::Window window;
    if (!window.Create(winConfig)) {
        std::cerr << "[Eunoia Runtime] Failed to create game window!\n";
        return -1;
    }

    // 2. Setup Input System
    InputSystem::Get().SetupDefaultActions();

    // 3. Setup Scene
    Scene scene;

    // Search for scenes in cooked content or local directories
    std::vector<std::string> searchPaths = {
        "Cooked/Scenes/Main.escene",
        "Cooked/Main.escene",
        "Scenes/Main.escene",
        "Content/Scenes/Main.escene",
        "Main.escene"
    };

    bool loaded = false;
    for (const auto& path : searchPaths) {
        if (std::filesystem::exists(path)) {
            std::cout << "[Eunoia Runtime] Loading scene: " << path << std::endl;
            if (SceneSerializer::LoadScene(scene, path)) {
                loaded = true;
                break;
            }
        }
    }

    if (!loaded) {
        std::cout << "[Eunoia Runtime] No packaged scene found; initializing default game scene." << std::endl;
        scene.LoadDefaultScene();
    }

    std::cout << "[Eunoia Runtime] Scene loaded with " << scene.objects.size() << " actors.\n";

    // Start play mode (initializes behaviours, resolves references, calls OnCreate)
    scene.StartPlayMode();

    std::cout << "[Eunoia Runtime] Entering game loop. Press ESC to quit.\n";

    auto lastTime = std::chrono::high_resolution_clock::now();

    // 4. Game Simulation Loop
    while (!window.ShouldClose()) {
        InputSystem::Get().BeginFrame();
        window.PollEvents();
        InputSystem::Get().Update(window.Handle());

        if (glfwGetKey(window.Handle(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            break;
        }

        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;
        if (deltaTime > 0.1f) deltaTime = 0.1f;

        // Update ScreenPrint system
        ScreenPrint::System::Get().Update(deltaTime);

        // Update Scene behaviours and physics
        scene.Update(deltaTime);

        // Frame timing throttle (~60 FPS)
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    // 5. Teardown
    scene.StopPlayMode();

    window.Destroy();
    glfwTerminate();

    std::cout << "[Eunoia Runtime] Game shut down cleanly.\n";
    return 0;
}
