#include "Game.h"
#include <iostream> // For exception handling

int main() {
    try {
        Game game; // Constructor loads resources, sets initial state, calls rebuild
        game.run(); // Contains the main loop
    }
    catch (const std::exception& e) {
        std::cerr << "Unhandled Exception: " << e.what() << std::endl;
        // Consider adding a platform-specific message box here
        return EXIT_FAILURE;
    }
    catch (...) {
        std::cerr << "Unknown unhandled exception." << std::endl;
        // Consider adding a platform-specific message box here
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}