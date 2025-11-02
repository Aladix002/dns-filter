#ifndef DNS_PROTOCOL_HPP
#define DNS_PROTOCOL_HPP

#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

// Odpoveda a hlavicka zo zdroja: https://tools.ietf.org/html/rfc1035

// Definicia Statistics struktury
struct Statistics {
    uint64_t totalQueries = 0;                  // Celkovy pocet dotazov
    uint64_t blockedQueries = 0;               // Pocet blokovanych dotazov
    uint64_t forwardedQueries = 0;            // Pocet preposlanych dotazov
    uint64_t otherTypeQueries = 0;              // Pocet inych typov dotazov (NOTIMP) - vratane AAAA
    uint64_t exactMatches = 0;                 // Pocet presnych zhody s blokovanymi domenami
    uint64_t subdomainMatches = 0;             // Pocet zhody s poddomenami
};

enum class DNSResponseCode : uint8_t {
    NOERROR = 0,    // Bez chyby
    FORMERR = 1,    // Chyba formatu
    SERVFAIL = 2,   // Chyba servera
    NXDOMAIN = 3,   // Domeny neexistuje
    NOTIMP = 4,     // Funkcia nie je implementovana
    REFUSED = 5     // Odmietnute
};


typedef struct dns_headers {
    unsigned id :16;           // Identifikator dotazu
    
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    unsigned rd :1;            // Recursion desired
    unsigned tc :1;            // Truncated
    unsigned aa :1;            // Authoritative answer
    unsigned opcode :4;        // Operation code
    unsigned qr :1;            // Query/Response flag
    unsigned rcode :4;         // Response code
    unsigned reserved :3;      // Reserved bits
    unsigned ra :1;            // Recursion available
    #else
    unsigned qr :1;            // Query/Response flag
    unsigned opcode :4;        // Operation code
    unsigned aa :1;            // Authoritative answer
    unsigned tc :1;            // Truncated
    unsigned rd :1;            // Recursion desired
    unsigned ra :1;            // Recursion available
    unsigned reserved :3;      // Reserved bits
    unsigned rcode :4;         // Response code
    #endif

    unsigned qdcount :16;      // Pocet otazok
    unsigned ancount :16;      // Pocet odpovedi
    unsigned nscount :16;      // Pocet authority records
    unsigned arcount :16;      // Pocet additional records
} dns_header;

// DNS Question struktura
typedef struct dns_questions {
    int type :16;              // Typ zaznamu
    int qclass :16;            // Trieda zaznamu 
} dns_question;


#define MAX_DNS_SIZE 1024      // Maximalna velkost DNS paketu

// Trieda pre spracovanie DNS protokolu
class DNSProtocol {
public:
    // Parsovanie domeny z DNS otazky
    static char* parseDomain(char* questionStart, std::string& domain);
    
    // Odoslanie chybovej odpovede klientovi
    static void sendErrorResponse(int clientSocket, const char* buffer, int len, 
                                 const struct sockaddr_storage& clientAddr, DNSResponseCode code, bool verbose = false);
    
    // Ziskanie adresy klienta ako retazec
    static std::string getClientAddressString(const struct sockaddr_storage& clientAddr);
    
    // Kontrola ci je to dotaz
    static bool isQuery(const dns_header* header);
    
    // Spracovanie DNS dotazu - vrati response code ak je potrebne blokovat, NOERROR ak nie
    static DNSResponseCode processQuery(const char* buffer, int len, const struct sockaddr_storage& clientAddr,
                                      const std::vector<std::string>& blockedDomains, bool verbose, 
                                      struct Statistics* stats = nullptr);
};

#endif 
