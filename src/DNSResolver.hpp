#ifndef DNS_RESOLVER_HPP
#define DNS_RESOLVER_HPP

#include <string>
#include <vector>
#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdexcept>
#include "DNSProtocol.hpp"


// Hlavna DNS Resolver trieda
class DNSResolver {
private:
    std::vector<std::string> blockedDomains_;       
    int clientSocket4_;                             // IPv4 client socket
    int clientSocket6_;                             // IPv6 client socket
    int resolverSocket_;                             
    std::string resolverAddress_;                   
    int port_;                                      
    bool verbose_;                                   
    bool statsEnabled_;                             // Ci su zapnute statistiky
    static DNSResolver* instance_;                  // Singleton instance pre signal handling
    
    // Statistiky
    Statistics stats_;
    
    // Privatne pomocne metody
    bool loadFilterFile(const std::string& filename);
    int createClientSocket(int port, int family);
    int createResolverSocket(const std::string& resolver, int port);
    void handleQuery(const char* buffer, int len, const struct sockaddr_storage& clientAddr, int clientSocket);
    static void signalHandler(int signal);
    
public:
    DNSResolver(const std::string& resolver, int port, const std::string& filterFile, bool verbose = false, bool stats = false);
    ~DNSResolver();
    
    // Zakazanie kopirovania
    DNSResolver(const DNSResolver&) = delete;
    DNSResolver& operator=(const DNSResolver&) = delete;
    
    // Povolenie presuvania
    DNSResolver(DNSResolver&&) = default;
    DNSResolver& operator=(DNSResolver&&) = default;
    
    bool initialize();
    void run();
    void printStatistics() const;
};

#endif 
