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
    
    // Kontrola napovedy a stats
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        args.help = true;
        return args;
    }
    if (argc == 2 && strcmp(argv[1], "--stats") == 0) {
        args.stats = true;
        return args;
    }
    
    // Detekcia duplicitnych argumentov
    int s_count = 0, p_count = 0, f_count = 0, v_count = 0, h_count = 0, stats_count = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-s") == 0) s_count++;
        else if (strcmp(argv[i], "-p") == 0) p_count++;
        else if (strcmp(argv[i], "-f") == 0) f_count++;
        else if (strcmp(argv[i], "-v") == 0) v_count++;
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) h_count++;
        else if (strcmp(argv[i], "--stats") == 0) stats_count++;
    }
    
    // Kontrola duplicitnych argumentov
    if (s_count > 1) {
        std::cerr << "Duplicitny parameter -s" << std::endl;
        args.invalidArgs = true;
        return args;
    }
    if (p_count > 1) {
        std::cerr << "Duplicitny parameter -p" << std::endl;
        args.invalidArgs = true;
        return args;
    }
    if (f_count > 1) {
        std::cerr << "Duplicitny parameter -f" << std::endl;
        args.invalidArgs = true;
        return args;
    }
    if (v_count > 1) {
        std::cerr << "Duplicitny parameter -v" << std::endl;
        args.invalidArgs = true;
        return args;
    }
    if (h_count > 1) {
        std::cerr << "Duplicitny parameter -h" << std::endl;
        args.invalidArgs = true;
        return args;
    }
    if (stats_count > 1) {
        std::cerr << "Duplicitny parameter --stats" << std::endl;
        args.invalidArgs = true;
        return args;
    }

    // Parsovanie argumentov - kod pre getopt z: https://man7.org/linux/man-pages/man3/getopt.3.html
    int opt;
    int option_index = 0;
    while ((opt = getopt_long(argc, argv, "s:p:f:vh", long_options, &option_index)) != -1) {
        switch (opt) {
            case 's':
                args.server = optarg;
                break;
            case 'p': {
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
                args.filterFile = optarg;
                break;
            case 'v':
                args.verbose = true;
                break;
            case 'h':
                args.help = true;
                break;
            case 0:
                // Long option bez short equivalent
                if (strcmp(long_options[option_index].name, "stats") == 0) {
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
    
    if (args.stats) {
        // Pre stats mode kontroluje len ci su povinne parametre zadane
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
    
    if (args.invalidPort) {
        return false;
    }
    
    if (args.invalidArgs) {
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
