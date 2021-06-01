#ifndef DREAMCATCHER_PROTOCOLS_H
#define DREAMCATCHER_PROTOCOLS_H

#define MAX_PROTO_LEN 16

typedef enum {
  IP  = 0,                // IP    // internet protocol, pseudo protocol number
  HOPOPT  = 0,            // HOPOPT    // IPv6 Hop-by-Hop Option [RFC1883]
  ICMP  = 1,              // ICMP    // internet control message protocol
  IGMP  = 2,              // IGMP    // Internet Group Management
  GGP = 3,                // GGP   // gateway-gateway protocol
  IPENCAP = 4,            // IP-ENCAP  // IP encapsulated in IP (officially ``IP'')
  ST  = 5,                // ST    // ST datagram mode
  TCP = 6,                // TCP   // transmission control protocol
  EGP = 8,                // EGP   // exterior gateway protocol
  IGP = 9,                // IGP   // any private interior gateway (Cisco)
  PUP = 12,               // PUP   // PARC universal packet protocol
  UDP = 17,               // UDP   // user datagram protocol
  HMP = 20,               // HMP   // host monitoring protocol
  XNS_IDP = 22,           // XNS-IDP   // Xerox NS IDP
  RDP = 27,               // RDP   // "reliable datagram" protocol
  ISO_TP4 = 29,           // ISO-TP4   // ISO Transport Protocol class 4 [RFC905]
  DCCP  = 33,             // DCCP    // Datagram Congestion Control Prot. [RFC4340]
  XTP = 36,               // XTP   // Xpress Transfer Protocol
  DDP = 37,               // DDP   // Datagram Delivery Protocol
  IDPR_CMTP = 38,         // IDPR-CMTP // IDPR Control Message Transport
  IPV6  = 41,             // IPv6    // Internet Protocol, version 6
  IPV6_ROUTE = 43,        // IPv6-Route  // Routing Header for IPv6
  IPV6_FRAG = 44,         // IPv6-Frag // Fragment Header for IPv6
  IDRP  = 45,             // IDRP    // Inter-Domain Routing Protocol
  RSVP  = 46,             // RSVP    // Reservation Protocol
  GRE = 47,               // GRE   // General Routing Encapsulation
  ESP = 50,               // IPSEC-ESP // Encap Security Payload [RFC2406]
  AH  = 51,               // IPSEC-AH  // Authentication Header [RFC2402]
  SKIP  = 57,             // SKIP    // SKIP
  IPV6_ICMP = 58,         // IPv6-ICMP // ICMP for IPv6
  IPV6_NONXT = 59,        // IPv6-NoNxt  // No Next Header for IPv6
  IPV6_OPTS = 60,         // IPv6-Opts // Destination Options for IPv6
  RSPF  = 73,             // RSPF CPHB // Radio Shortest Path First (officially CPHB)
  VMTP  = 81,             // VMTP    // Versatile Message Transport
  EIGRP = 88,             // EIGRP   // Enhanced Interior Routing Protocol (Cisco)
  OSPF  = 89,             // OSPFIGP   // Open Shortest Path First IGP
  AX_25 = 93,             // AX.25   // AX.25 frames
  IPIP  = 94,             // IPIP    // IP-within-IP Encapsulation Protocol
  ETHERIP = 97,           // ETHERIP   // Ethernet-within-IP Encapsulation [RFC3378]
  ENCAP = 98,             // ENCAP   // Yet Another IP encapsulation [RFC1241]
  // = 99,                //      // any private encryption scheme
  PIM = 103,              // PIM   // Protocol Independent Multicast
  IPCOMP  = 108,          // IPCOMP    // IP Payload Compression Protocol
  VRRP  = 112,            // VRRP    // Virtual Router Redundancy Protocol [RFC5798]
  L2TP  = 115,            // L2TP    // Layer Two Tunneling Protocol [RFC2661]
  ISIS  = 124,            // ISIS    // IS-IS over IPv4
  SCTP  = 132,            // SCTP    // Stream Control Transmission Protocol
  FC  = 133,              // FC    // Fibre Channel
  MOBILITY_HEADER = 135,  // Mobility-Header // Mobility Support for IPv6 [RFC3775]
  UDPLITE = 136,          // UDPLite   // UDP-Lite [RFC3828]
  MPLS_IN_IP = 137,       // MPLS-in-IP  // MPLS-in-IP [RFC4023]
  MANET = 138,            //     // MANET Protocols [RFC5498]
  HIP = 139,              // HIP   // Host Identity Protocol
  SHIM6 = 140,            // Shim6   // Shim6 Protocol [RFC5533]
  WESP  = 141,            // WESP    // Wrapped Encapsulating Security Payload
  ROHC  = 142             // ROHC    // Robust Header Compression
} protocol;

char* get_protocol_string(protocol proto);

#endif // DREAMCATCHER_PROTOCOLS_H
