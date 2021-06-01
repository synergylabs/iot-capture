#ifndef DREAMCATCHER_H
#define DREAMCATCHER_H

#include <netinet/in.h>
#include <linux/types.h>
#include <linux/netfilter.h>            /* for NF_ACCEPT */
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>

#include "../protocols.h"
//#include "config.h"
#include "../dns.h"

#define QUEUE_NUM 4670


void orig_print_pkt (struct nfq_data *tb);
void ip_to_bytes(unsigned char* buf, __be32 addr);
void print_ipv4(struct ip* i);
void print_ipv6(struct ip6_hdr* i);
unsigned int get_src_vlan(struct nfq_data *tb);
unsigned int get_dst_vlan(struct nfq_data *tb);
void print_dns(dns_header* d);
//int add_rule(struct nfq_data *tb, u_int32_t* verdict);
void print_pkt (struct nfq_data *tb);
void reload_firewall();
void print_tcp(struct tcphdr* t);
void print_udp(struct udphdr* u);
void print_icmp(struct icmphdr* i);
//void alert_user(rule* r);
int cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg, struct nfq_data *nfa, void *data);
int main(int argc, char **argv);


#endif // DREAMCATCHER_H
