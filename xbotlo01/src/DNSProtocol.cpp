#include "DNSProtocol.hpp"
#include <iostream>
#include <algorithm>
#include <cctype>


char* DNSProtocol::parseDomain(char* questionStart, std::string& domain) {
    domain.clear();
    
    if (*questionStart == 0) {
        domain = "<root>";
        return questionStart + 1;
    }
    
    // Parsuje DNS domenu po labeloch
    while (*questionStart != 0) {
        int8_t labelLength = *questionStart;
        
        for (int8_t i = 0; i < labelLength; i++) {
            domain += questionStart[i + 1];
        }
        
        questionStart += labelLength + 1;
        domain += ".";
    }
    
    // Odstranuje koncovu bodku
    if (!domain.empty() && domain.back() == '.') {
        domain.pop_back();
    }
    
    return questionStart + 1;
}

// Odosiela chybovu odpoved klientovi
// Info pre implementaciu: https://datatracker.ietf.org/doc/html/rfc1035
void DNSProtocol::sendErrorResponse(int clientSocket, const char* buffer, int len, 
                                   const struct sockaddr_storage& clientAddr, DNSResponseCode code) {
    char errorBuffer[MAX_DNS_SIZE];
    memcpy(errorBuffer, buffer, len);
    
    dns_header* header = (dns_header*)errorBuffer;
    header->qr = 1;  // Odpoved
    header->rcode = static_cast<uint8_t>(code);
    
    socklen_t addrLen;
    if (clientAddr.ss_family == AF_INET) {
        addrLen = sizeof(struct sockaddr_in);
    } else if (clientAddr.ss_family == AF_INET6) {
        addrLen = sizeof(struct sockaddr_in6);
    } else {
        addrLen = sizeof(clientAddr);
    }
    
    if (sendto(clientSocket, errorBuffer, len, 0, 
               (const struct sockaddr*)&clientAddr, addrLen) < 0) {
        std::cerr << "Failed to send error response to client" << std::endl;
    }
}

// Kontroluje ci je to dotaz 
bool DNSProtocol::isQuery(const dns_header* header) {
    return ntohs(header->qr) == 0;
}

// Spracovava DNS dotaz - vracia response code ak je potrebne blokovat, NOERROR ak nie
DNSResponseCode DNSProtocol::processQuery(const char* buffer, int /*len*/, const struct sockaddr_storage& /*clientAddr*/,
                                         const std::vector<std::string>& blockedDomains, bool verbose, Statistics* stats) {
    dns_header* header = (dns_header*)buffer;
    
    if (!isQuery(header)) {
        return DNSResponseCode::NOERROR;
    }
    
    char* question = (char*)header + sizeof(dns_header);
    uint16_t qdcount = ntohs(header->qdcount);
    
    if (stats) {
        stats->totalQueries++;
    }
    
    for (uint16_t i = 0; i < qdcount; i++) {
        std::string domain;
        dns_question* qtype = (dns_question*)parseDomain(question, domain);
        uint16_t qtype_val = ntohs(qtype->type);
        
        // Kontroluje ci je domena blokovana
        std::string normalized = domain;
        std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
        
        // Odstranuje koncove bodky, medzery a konce riadkov
        while (!normalized.empty() && (normalized.back() == '.' || normalized.back() == ' ' || 
               normalized.back() == '\r' || normalized.back() == '\n')) {
            normalized.pop_back();
        }
        
        // Kontroluje typ dotazu
        if (qtype_val != 1) { 
            if (verbose) {
                std::cout << "query: " << domain << ", action: notimp" << std::endl;
            }
            if (stats) {
                stats->otherTypeQueries++; 
            }
            return DNSResponseCode::NOTIMP;
        }
        
        // Kontroluje presnu zhodu
        for (const auto& blocked : blockedDomains) {
            if (normalized == blocked) {
                if (verbose) {
                    std::cout << "query: " << domain << ", action: blocked" << std::endl;
                }
                if (stats) {
                    stats->blockedQueries++;
                    stats->exactMatches++;
                }
                return DNSResponseCode::REFUSED;
            }
        }
        
        // Kontrola subdomen
        for (const auto& blocked : blockedDomains) {
            if (normalized.length() > blocked.length() && 
                normalized.substr(normalized.length() - blocked.length() - 1) == "." + blocked) {
                if (verbose) {
                    std::cout << "query: " << domain << ", action: blocked" << std::endl;
                }
                if (stats) {
                    stats->blockedQueries++;
                    stats->subdomainMatches++;
                }
                return DNSResponseCode::REFUSED;
            }
        }
        
        if (verbose) {
            std::cout << "query: " << domain << ", action: forwarded" << std::endl;
        }
        
        question = (char*)qtype + sizeof(dns_question);
    }
    
    return DNSResponseCode::NOERROR;
}
