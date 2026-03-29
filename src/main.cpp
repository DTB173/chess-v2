#include "CLI.h"
#include "UCI.h"
#include "Perft.h"

#include <iostream>

int main(int argc, char* argv[]) {
    if (argc > 1) {
        std::string mode = argv[1];
        if (mode == "cli") {
            CLI::game_loop();
        }
        else if (mode == "uci") {
            UCI::uci_loop();
        }
        else if (mode == "perft") {
            for (int i{}; i <= 6; ++i)
                Perft::measure_perft(i, true);
        }
        else {
            std::cerr << "Unknown mode: " << mode << "\n";
            std::cerr << "Usage: " << argv[0] << " [cli|uci|perft]\n";
            return 1;
		}
    }
    else
		UCI::uci_loop();
    return 0;
}
