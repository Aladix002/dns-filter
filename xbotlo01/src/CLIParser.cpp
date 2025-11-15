#include "CLIParser.hpp"
#include <getopt.h>
#include <cstdlib>
#include <cstring>

// Long options pre getopt_long
static struct option long_options[] = {
    {"help", no_argument, 0, 'h'},
    {"stats", no_argument, 0, 0},
    {0, 0, 0, 0}
};



ProgramArguments CLIParser::parse(int argc, char* argv[]) {
    ProgramArguments args;
    
    // Parsovanie argumentov - kod pre getopt z: https://man7.org/linux/man-pages/man3/getopt.3.html
    int opt;
    int option_index = 0;
    while ((opt = getopt_long(argc, argv, "s:p:f:vh", long_options, &option_index)) != -1) {
        switch (opt) {
            case 's':
                if (!args.server.empty()) {
                    std::cerr << "Duplicitny parameter -s" << std::endl;
                    args.invalidArgs = true;
                    return args;
                }
                args.server = optarg;
                break;
            case 'p': {
                if (args.port != 53) {
                    std::cerr << "Duplicitny parameter -p" << std::endl;
                    args.invalidArgs = true;
                    return args;
                }
                // Validacia portu - musi byt cislo
                char* end;
                long port = std::strtol(optarg, &end, 10);
                if (*end != '\0' || port < 0 || port > 65535) {
                    std::cerr << "Neplatne cislo portu: " << optarg << std::endl;
                    args.invalidPort = true;
                    return args;
                }
                args.port = static_cast<int>(port);
                break;
            }
            case 'f':
                if (!args.filterFile.empty()) {
                    std::cerr << "Duplicitny parameter -f" << std::endl;
                    args.invalidArgs = true;
                    return args;
                }
                args.filterFile = optarg;
                break;
            case 'v':
                if (args.verbose) {
                    std::cerr << "Duplicitny parameter -v" << std::endl;
                    args.invalidArgs = true;
                    return args;
                }
                args.verbose = true;
                break;
            case 'h':
                args.help = true;
                break;
            case 0:
                // Long option bez short equivalent
                if (strcmp(long_options[option_index].name, "stats") == 0) {
                    if (args.stats) {
                        std::cerr << "Duplicitny parameter --stats" << std::endl;
                        args.invalidArgs = true;
                        return args;
                    }
                    args.stats = true;
                }
                break;
            case ':':
                // Chybajuca hodnota pre povinny parameter
                std::cerr << "Prepinac -" << char(optopt) << " vyzaduje hodnotu" << std::endl;
                args.help = true;
                break;
            case '?':
                // Neznamy parameter
                std::cerr << "Neznamy prepinac -" << char(optopt) << std::endl;
                args.invalidArgs = true;
                return args;
        }
    }
    
    return args;
}

void CLIParser::printHelp() {
    std::cout << "Pouzitie: ./dns -s server [-p port] -f filter_file [-v] [--stats]\n\n";
    std::cout << "Popis parametrov:\n";
    std::cout << "Poradie parametrov je libovolne.\n";
    std::cout << "    -s: IP adresa alebo domenove meno DNS serveru (resolveru), kam sa ma zaslat dotaz.\n";
    std::cout << "    -p port: (volitelne) Cislo portu, na ktorom bude program ocekavat dotazy. Vychodzia je port 53.\n";
    std::cout << "    -f filter_file: Meno suboru obsahujuceho nezadouce domeny.\n";
    std::cout << "    -v: (volitelne) Verbose mod - vypisuje informacie o preklade.\n";
    std::cout << "    --stats: (volitelne) Na konci behu vypise statistiky.\n";
    std::cout << "    -h: Zobrazi tuto napovedu.\n";
}

bool CLIParser::validateArguments(const ProgramArguments& args) {
    if (args.help) {
        return false;
    }
    
    if (args.invalidPort || args.invalidArgs) {
        return false;
    }
    
    // Kontrola povinnych parametrov
    if (args.server.empty()) {
        std::cerr << "Parameter -s je povinny" << std::endl;
        return false;
    }
    
    if (args.filterFile.empty()) {
        std::cerr << "Parameter -f je povinny" << std::endl;
        return false;
    }
    
    if (args.port < 0 || args.port > 65535) {
        std::cerr << "Port musi byt v rozsahu 0-65535" << std::endl;
        return false;
    }
    
    return true;
}
