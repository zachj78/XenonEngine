#include "../include/Renderer.h"

class Application {
public: void run() {
    init();
    mainLoop();
    cleanup();
};

private:
    std::shared_ptr<Renderer> renderer;

    void init() {
        renderer = std::make_shared<Renderer>();
        renderer->createRenderer();
    };

    void mainLoop() {
        renderer->draw();
    };

    void cleanup() {
        std::cout << "Clean up function called" << std::endl;
        renderer->cleanup();
    };
};

int main() {
    Application app;

    try {
        app.run();
    }
    catch (const std::exception& e) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
};