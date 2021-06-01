#ifndef DNS_H
#define DNS_H

#include <unistd.h>
#include <sys/types.h>

// creating my own struct for the dns header because I can't find a good standard one
// http://www.networksorcery.com/enp/protocol/dns.htm
typedef struct dns_header {
  u_int16_t id;
  // flags
  //u_int16_t flags;
  u_int16_t qr:1,     // Query/Response (0 is query, 1 response)
            opcode:4, // Opcode
            aa:1,     // Authoritative Answer
            tc:1,     // Truncated
            rd:1,     // Recursion Desired
            ra:1,     // Recursion Available
            z:1,      //  ... my reference didn't specify what this was for
            ad:1,     // Authenticated Data
            cd:1,     // Checking Disabled
            rcode:4;  // Return Code
  u_int16_t questions;
  u_int16_t answer_rr;
  u_int16_t authority_rr;
  u_int16_t additional_rr;
} dns_header;

typedef enum {
  A = 1,
  PTR = 12,
  TXT = 16, 
  AAAA = 28,
  SRV = 33
} dns_type;

#endif // DNS_H
