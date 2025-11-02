// VUT FIT Brno
// ISA 2025
// Filip Botlo - xbotlo01

#include "DNSResolver.hpp"
#include "CLIParser.hpp"
#include <iostream>
#include <exception>

int main(int argc, char* argv[]) {
    try {
        // Parsovanie argumentov prikazoveho riadka
        ProgramArguments args = CLIParser::parse(argc, argv);
        
        if (args.help) {
            CLIParser::printHelp();
            return 0;
        }
        
        // Validacia argumentov
        if (!CLIParser::validateArguments(args)) {
            return 1;
        }
        
        // Vytvorenie a spustenie DNS resolvera
        DNSResolver resolver(args.server, args.port, args.filterFile, args.verbose, args.stats);
        resolver.run();
        
        // Vypis statistik na konci behu
        if (args.stats) {
            resolver.printStatistics();
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Chyba: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
