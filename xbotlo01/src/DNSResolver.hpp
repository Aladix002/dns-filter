#ifndef DNS_RESOLVER_HPP
#define DNS_RESOLVER_HPP

#include <string>
#include <vector>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdexcept>
#include <map>
#include <cstdint>
#include "DNSProtocol.hpp"

// Struktura pre ulozenie informacii o klientovi
struct ClientInfo {
    struct sockaddr_storage addr;
    socklen_t addrLen;
    int socket;
    int resolverSocket;  // Resolver socket pouzity pre tento dotaz
};

// Struktura pre unikatny kľúč dotazu (DNS ID + adresa klienta)
struct QueryKey {
    uint16_t dnsId;
    struct sockaddr_storage clientAddr;
    
    bool operator<(const QueryKey& other) const {
        if (dnsId != other.dnsId) {
            return dnsId < other.dnsId;
        }
        // Porovnanie adries
        if (clientAddr.ss_family != other.clientAddr.ss_family) {
            return clientAddr.ss_family < other.clientAddr.ss_family;
        }
        if (clientAddr.ss_family == AF_INET) {
            const struct sockaddr_in* a = (const struct sockaddr_in*)&clientAddr;
            const struct sockaddr_in* b = (const struct sockaddr_in*)&other.clientAddr;
            if (a->sin_addr.s_addr != b->sin_addr.s_addr) {
                return a->sin_addr.s_addr < b->sin_addr.s_addr;
            }
            return a->sin_port < b->sin_port;
        } else if (clientAddr.ss_family == AF_INET6) {
            const struct sockaddr_in6* a = (const struct sockaddr_in6*)&clientAddr;
            const struct sockaddr_in6* b = (const struct sockaddr_in6*)&other.clientAddr;
            int cmp = memcmp(&a->sin6_addr, &b->sin6_addr, sizeof(a->sin6_addr));
            if (cmp != 0) {
                return cmp < 0;
            }
            return a->sin6_port < b->sin6_port;
        }
        return memcmp(&clientAddr, &other.clientAddr, sizeof(clientAddr)) < 0;
    }
};

// Hlavna DNS Resolver trieda
class DNSResolver {
private:
    std::vector<std::string> blockedDomains_;       
    int clientSocket4_;                             // IPv4 client socket
    int clientSocket6_;                             // IPv6 client socket
    int resolverSocket4_;                           // IPv4 resolver socket
    int resolverSocket6_;                           // IPv6 resolver socket
    std::string resolverAddress_;                   
    int port_;                                      
    bool verbose_;                                   
    bool statsEnabled_;                             // Ci su zapnute statistiky
    static DNSResolver* instance_;                  // Singleton instance pre signal handling
    
    // Mapovanie QueryKey -> klient (pre paralelne spracovanie)
    // Pouziva kombinaciu DNS ID + adresa klienta pre unikatnost
    std::map<QueryKey, ClientInfo> pendingQueries_;
    
    // Statistiky
    Statistics stats_;
    
    // Privatne pomocne metody
    bool loadFilterFile(const std::string& filename);
    int createClientSocket(int port, int family);
    int createResolverSocket(const std::string& resolver, int port, int family);
    void handleQuery(const char* buffer, int len, const struct sockaddr_storage& clientAddr, int clientSocket);
    void handleResolverResponse(int resolverSocket);
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
