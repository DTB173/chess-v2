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
            Perft::PerftRecord record = Perft::parse_record(argc, argv);
            Perft::measure_perft(record);
        }
        else if (mode == "testsuite") {
            Perft::runt_test_suite();
        }
        else {
            std::cerr << "Unknown mode: " << mode << "\n";
            std::cerr << "Usage: \n";
            std::cerr << "\t" << argv[0] << " cli\n";
            std::cerr << "\t" << argv[0] << " uci\n";
            std::cerr << "\t" << argv[0] << " testsuite\n";
            std::cerr << "\t" << argv[0] << " perft \"<fen>\" <depth> <expected nodes - optioanl>\n";
            return 1;
		}
    }
    else
		UCI::uci_loop();
    return 0;
}
