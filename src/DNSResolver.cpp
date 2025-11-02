#include "DNSResolver.hpp"
#include <fstream>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <sstream>
#include <vector>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip6.h>
#include <netdb.h>
#include <unistd.h>
#include <iomanip>
#include <signal.h>
#include <csignal>

// Konstanty
#define DNS_PORT 53

// Static member initialization
DNSResolver* DNSResolver::instance_ = nullptr;

// Konstruktor - nacitanie filtra a inicializacia
DNSResolver::DNSResolver(const std::string& resolver, int port, const std::string& filterFile, bool verbose, bool stats)
    : clientSocket4_(-1), clientSocket6_(-1), resolverSocket_(-1), resolverAddress_(resolver), port_(port), verbose_(verbose), statsEnabled_(stats) {
    
    // Nastavenie singleton instance pre signal handling
    instance_ = this;
    
    if (!loadFilterFile(filterFile)) {
        throw std::runtime_error("Failed to load filter file: " + filterFile);
    }
}

// Destruktor - zatvaraju sa sockety
DNSResolver::~DNSResolver() {
    if (clientSocket4_ >= 0) {
        close(clientSocket4_);
    }
    if (clientSocket6_ >= 0) {
        close(clientSocket6_);
    }
    if (resolverSocket_ >= 0) {
        close(resolverSocket_);
    }
    
    // Vypisuje statistiky na konci
    if (statsEnabled_) {
        printStatistics();
    }
    
    // Resetuje singleton instance
    instance_ = nullptr;
}

// Nacitava subor s blokovanymi domenami
bool DNSResolver::loadFilterFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Cannot open filter file: " << filename << std::endl;
        return false;
    }
    

// Ukoncenia riadkov pre rozne OS: https://en.wikipedia.org/wiki/Newline#Representation
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    // Rozdeluje na riadky pre vsetky mozne ukoncenia riadkov
    std::vector<std::string> lines;
    std::string line;
    for (char c : content) {
        if (c == '\n' || c == '\r') {
            if (!line.empty()) {
                lines.push_back(line);
                line.clear();
            }
        } else {
            line += c;
        }
    }
    if (!line.empty()) {
        lines.push_back(line);
    }
    
    for (const std::string& currentLine : lines) {
        // Preskakuje prazdne riadky a komentare
        if (currentLine.empty() || currentLine[0] == '#') {
            continue;
        }
        
        // Normalizacia a pridanie do blokovanych domen
        std::string normalized = currentLine;
        // Konverzia na male pismena
        std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
        
        // Odstranuje koncove bodky, medzery a konce riadkov
        while (!normalized.empty() && (normalized.back() == '.' || normalized.back() == ' ' || 
               normalized.back() == '\r' || normalized.back() == '\n')) {
            normalized.pop_back();
        }
        
        if (!normalized.empty()) {
            blockedDomains_.push_back(normalized);
        }
    }
    
    file.close();
    return true;
}


// Vytvara client socket pre konkretnu IP verziu
// Referencia: https://man7.org/linux/man-pages/man2/socket.2.html
int DNSResolver::createClientSocket(int port, int family) {
    int sockfd = socket(family, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        return -1;
    }
    
    // Nastavuje SO_REUSEADDR
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(sockfd);
        return -1;
    }
    
    if (family == AF_INET6) {
        // Nastavenie IPV6_V6ONLY na 1 pre čistý IPv6
        int v6only = 1;
        if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &v6only, sizeof(v6only)) < 0) {
            close(sockfd);
            return -1;
        }
        
        // Bind na IPv6 adresu
        struct sockaddr_in6 addr6;
        memset(&addr6, 0, sizeof(addr6));
        addr6.sin6_family = AF_INET6;
        addr6.sin6_addr = in6addr_any;
        addr6.sin6_port = htons(port);
        
        if (bind(sockfd, (struct sockaddr*)&addr6, sizeof(addr6)) == 0) {
            return sockfd;
        } else {
            close(sockfd);
            return -1;
        }
    } else if (family == AF_INET) {
        // Bind na IPv4 adresu
        struct sockaddr_in addr4;
        memset(&addr4, 0, sizeof(addr4));
        addr4.sin_family = AF_INET;
        addr4.sin_addr.s_addr = INADDR_ANY;
        addr4.sin_port = htons(port);
        
        if (bind(sockfd, (struct sockaddr*)&addr4, sizeof(addr4)) == 0) {
            return sockfd;
        } else {
            close(sockfd);
            return -1;
        }
    }
    
    close(sockfd);
    return -1;
}

// Vytvorenie resolver socketu - pripojenie k DNS serveru prebrane z: https://man7.org/linux/man-pages/man2/socket.2.html
int DNSResolver::createResolverSocket(const std::string& resolver, int port) {
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    
    if (getaddrinfo(resolver.c_str(), std::to_string(port).c_str(), &hints, &result) != 0) {
        std::cerr << "Failed to resolve DNS server: " << resolver << std::endl;
        return -1;
    }
    
    int sockfd = -1;
    for (struct addrinfo* rp = result; rp != nullptr; rp = rp->ai_next) {
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd < 0) continue;
        
        if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;
        }
        
        close(sockfd);
        sockfd = -1;
    }
    
    freeaddrinfo(result);
    
    if (sockfd < 0) {
        std::cerr << "Failed to connect to DNS resolver: " << resolver << std::endl;
    }
    
    return sockfd;
}
// Inicializacia socketov - vytvorenie client a resolver socketov
bool DNSResolver::initialize() {
    // Vytvorenie IPv4 socketu
    clientSocket4_ = createClientSocket(port_, AF_INET);
    if (clientSocket4_ < 0) {
        if (verbose_) {
            std::cerr << "Varovanie: Nepodarilo sa vytvorit IPv4 socket" << std::endl;
        }
    }
    
    // Vytvara IPv6 socket
    clientSocket6_ = createClientSocket(port_, AF_INET6);
    if (clientSocket6_ < 0) {
        if (verbose_) {
            std::cerr << "Varovanie: Nepodarilo sa vytvorit IPv6 socket" << std::endl;
        }
    }
    
    // Aspon jeden socket musi fungovat
    if (clientSocket4_ < 0 && clientSocket6_ < 0) {
        std::cerr << "Chyba: Nepodarilo sa vytvorit ani IPv4 ani IPv6 socket" << std::endl;
        return false;
    }
    
    resolverSocket_ = createResolverSocket(resolverAddress_, DNS_PORT);
    if (resolverSocket_ < 0) {
        if (clientSocket4_ >= 0) close(clientSocket4_);
        if (clientSocket6_ >= 0) close(clientSocket6_);
        return false;
    }
    
    return true;
}


// Spracovanie prichadzajuceho DNS dotazu podla: https://tools.ietf.org/html/rfc1035
void DNSResolver::handleQuery(const char* buffer, int len, const struct sockaddr_storage& clientAddr, int clientSocket) {
    // Pouzitie DNSProtocol pre spracovanie dotazu
    DNSResponseCode responseCode = DNSProtocol::processQuery(buffer, len, clientAddr, blockedDomains_, verbose_, statsEnabled_ ? &stats_ : nullptr);
    
    if (responseCode != DNSResponseCode::NOERROR) {
        DNSProtocol::sendErrorResponse(clientSocket, buffer, len, clientAddr, responseCode);
        return;
    }
    
    // Kontrola ci je resolver dostupny
    if (resolverSocket_ < 0) {
        DNSProtocol::sendErrorResponse(clientSocket, buffer, len, clientAddr, DNSResponseCode::SERVFAIL);
        return;
    }
    
    // Presmerovava dotaz na resolver
    if (send(resolverSocket_, buffer, len, 0) < 0) {
        std::cerr << "Chyba pri odosielani dotazu na resolver" << std::endl;
        return;
    }
    
    // Aktualizuje statistiky pre preposlane dotazy (len ak sa dotaz úspešne odoslal)
    if (statsEnabled_) {
        stats_.forwardedQueries++;
    }
    
    // Prijatie odpovede od resolvera
    char responseBuffer[MAX_DNS_SIZE];
    int received = recv(resolverSocket_, responseBuffer, MAX_DNS_SIZE, 0);
    if (received < 0) {
        std::cerr << "Chyba pri prijimani odpovede od resolvera" << std::endl;
        return;
    }
    
    
    // Odoslanie odpovede klientovi
    socklen_t addrLen;
    if (clientAddr.ss_family == AF_INET) {
        addrLen = sizeof(struct sockaddr_in);
    } else if (clientAddr.ss_family == AF_INET6) {
        addrLen = sizeof(struct sockaddr_in6);
    } else {
        addrLen = sizeof(clientAddr);
    }
    
    if (sendto(clientSocket, responseBuffer, received, 0,
               (const struct sockaddr*)&clientAddr, addrLen) < 0) {
        std::cerr << "Chyba pri odosielani odpovede klientovi" << std::endl;
    }
}

// Vypisuje statistiky
void DNSResolver::printStatistics() const {
    if (!statsEnabled_) {
        return;
    }
    
    std::cout << "total queries: " << stats_.totalQueries << std::endl;
    std::cout << "blocked: " << stats_.blockedQueries << std::endl;
    std::cout << "forwarded: " << stats_.forwardedQueries << std::endl;
    std::cout << "notimp: " << stats_.otherTypeQueries << std::endl;
    std::cout << "blocked - exact matches: " << stats_.exactMatches << std::endl;
    std::cout << "blocked - subdomain matches: " << stats_.subdomainMatches << std::endl;
}

// Signal handler pre SIGINT (Ctrl+C)
void DNSResolver::signalHandler(int signal) {
    if (signal == SIGINT && instance_ != nullptr) {
        if (instance_->statsEnabled_) {
            std::cout << std::endl;
            instance_->printStatistics();
        }
        exit(0);
    }
}


// Hlavny server loop - cakanie na dotazy a ich spracovanie 
void DNSResolver::run() {
    if (!initialize()) {
        throw std::runtime_error("Failed to initialize DNS filter");
    }
    
    // Nastavenie signal handler pre SIGINT
    signal(SIGINT, signalHandler);
    
    if (verbose_) {
        std::cout << "dns filter started on port: " << port_ << std::endl;
        std::cout << "resolver: " << resolverAddress_ << std::endl;
    }
    
    char buffer[MAX_DNS_SIZE];
    struct sockaddr_storage clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    
    // Hlavny cyklus - cakanie na dotazy pomocou select
    while (true) {
        fd_set readfds;
        FD_ZERO(&readfds);
        
        int maxfd = -1;
        if (clientSocket4_ >= 0) {
            FD_SET(clientSocket4_, &readfds);
            maxfd = std::max(maxfd, clientSocket4_);
        }
        if (clientSocket6_ >= 0) {
            FD_SET(clientSocket6_, &readfds);
            maxfd = std::max(maxfd, clientSocket6_);
        }
        
        if (maxfd < 0) {
            std::cerr << "Ziadny socket nie je dostupny" << std::endl;
            break;
        }
        
        int result = select(maxfd + 1, &readfds, nullptr, nullptr, nullptr);
        if (result < 0) {
            std::cerr << "Chyba v select" << std::endl;
            continue;
        }
        
        // Spracovanie IPv4 socket
        if (clientSocket4_ >= 0 && FD_ISSET(clientSocket4_, &readfds)) {
            addrLen = sizeof(clientAddr);
            int n = recvfrom(clientSocket4_, buffer, MAX_DNS_SIZE, 0,
                            (struct sockaddr*)&clientAddr, &addrLen);
            
            if (n > 0) {
                handleQuery(buffer, n, clientAddr, clientSocket4_);
            }
        }
        
        // Spracovanie IPv6 socketu
        if (clientSocket6_ >= 0 && FD_ISSET(clientSocket6_, &readfds)) {
            addrLen = sizeof(clientAddr);
            int n = recvfrom(clientSocket6_, buffer, MAX_DNS_SIZE, 0,
                            (struct sockaddr*)&clientAddr, &addrLen);
            
            if (n > 0) {
                handleQuery(buffer, n, clientAddr, clientSocket6_);
            }
        }
    }
}
