import CLI;
import Perft;
#include <iostream>

int main() {
    //CLI::game_loop();
	for (int depth = 1; depth <= 6; ++depth) {
		std::cout << "=== PERFT Depth " << depth << " ===\n";
		Perft::measure_perft(depth, true);
		std::cout << "=====================\n";
	}
		
    return 0;
}
