#include "../include/System_Components/Renderer.h"
#include "../include/System_Components/Physics.h"

class Application {
public: void run() {
    init();
    mainLoop();
    cleanup();
};

private:
    std::shared_ptr<Renderer> renderer;
    std::shared_ptr<Physics> physics; 

    void init() {
        renderer = std::make_shared<Renderer>();
        renderer->createRenderer();

        physics = std::make_shared<Physics>();
    };

    void mainLoop() {
        //Physics should be applied here
        //physics->updateComponents();

        //Draw function is applied - descriptors are updated and applied for meshes
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
    } catch (const std::out_of_range& e) {
        std::cerr << "Caught std::out_of_range: " << e.what() << '\n';
    } catch (const std::exception& e) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
};