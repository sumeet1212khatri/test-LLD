#include "Orderbook.h"
#include "Benchmark.h"
#include <iostream>
#include <string>
#include <cstring>

void printHelp(const char* prog) {
    std::cout << "Usage: " << prog << " [options]\n\n"
              << "Options:\n"
              << "  --orders N       Total orders (default: 20000000)\n"
              << "  --cancel-ratio R Cancel ratio (default: 0.20)\n"
              << "  --market-ratio R Market order ratio (default: 0.10)\n"
              << "  --json           Output results as JSON\n"
              << "  --quiet          Suppress verbose output\n";
}

int main(int argc, char** argv) {
    BenchmarkConfig cfg;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i],"--help") || !strcmp(argv[i],"-h")) { 
            printHelp(argv[0]); 
            return 0; 
        }
        else if (!strcmp(argv[i],"--orders") && i+1<argc) cfg.totalOrders = std::stoull(argv[++i]);
        else if (!strcmp(argv[i],"--cancel-ratio") && i+1<argc) cfg.cancelRatio = std::stod(argv[++i]);
        else if (!strcmp(argv[i],"--market-ratio") && i+1<argc) cfg.marketRatio = std::stod(argv[++i]);
        else if (!strcmp(argv[i],"--json")) cfg.jsonOutput = true;
        else if (!strcmp(argv[i],"--quiet")) cfg.verbose = false;
        else { 
            std::cerr << "Unknown arg: " << argv[i] << "\n"; 
            return 1; 
        }
    }

    try { 
        Benchmark{cfg}.Run(); 
        return 0; 
    } catch (const std::exception& e) { 
        std::cerr << "Fatal Error: " << e.what() << "\n"; 
        return 1; 
    }
}
