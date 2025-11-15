#ifndef CLI_PARSER_HPP
#define CLI_PARSER_HPP

#include <string>
#include <iostream>

// Struktura pre ulozenie argumentov programu
struct ProgramArguments {
    std::string server;     
    int port = 53;          
    std::string filterFile;    
    bool verbose = false;     
    bool help = false;       
    bool stats = false;       
    bool invalidPort = false;  
    bool invalidArgs = false;  
};

// Trieda pre parsovanie argumentov prikazoveho riadka
class CLIParser {
public:
    static ProgramArguments parse(int argc, char* argv[]);
    static void printHelp();
    static bool validateArguments(const ProgramArguments& args);
};

#endif
