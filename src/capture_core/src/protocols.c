#include <stddef.h>
#include "protocols.h"
#include "logger.hpp"

#define TAG "PROTOCOLS"

char* get_protocol_string(protocol proto) {
  switch (proto) {
    case TCP:
      //LOGV("TCP");
      return "tcp";
    case UDP:
      //LOGV("UDP");
      return "udp";
    case ICMP:
      //LOGV("ICMP");
      return "icmp";
  } 
  return NULL;
}
