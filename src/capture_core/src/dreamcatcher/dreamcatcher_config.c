#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <openssl/md5.h>
#include <arpa/inet.h>

//#include <uci.h>

#include "dreamcatcher_main.h"
#include "dreamcatcher_config.h"
#include "logger.hpp"
#include "../protocols.h"
#include "../dns.h"

#define TAG "CONFIG"

#define CONFIG_FILE "/etc/config/dreamcatcher"

#define MAX_TRIES 3

// initialize global lock references to 0
int lock_references = 0;

void set_message(rule* r) {
  switch (r->type) {
    case (UNICAST):
      snprintf(r->message, sizeof(r->message), "%d wants to send messages to %d", r->src_vlan, r->dst_vlan);
      break;
    case (BROADCAST):
      snprintf(r->message, sizeof(r->message), "%d wants to broadcast messages to your network", r->src_vlan);
      break;
    case (DISCOVER):
      snprintf(r->message, sizeof(r->message), "%d wants to discover services on your network", r->src_vlan);
      break;
    case (ADVERTISE):
      snprintf(r->message, sizeof(r->message), "%d wants to advertise itself on your network as %s", r->src_vlan, r->device_name);
      break;
    default:
      snprintf(r->message, sizeof(r->message), "Someone's trying to talk to someone. Please use advanced mode to view the specific details.");
      break;
  }
}

void print_uci_ptr(struct uci_ptr* p) {
  LOGV("uci_ptr package = %s",p->package);
  LOGV("uci_ptr section = %s",p->section);
  LOGV("uci_ptr option  = %s",p->option);
  LOGV("uci_ptr value   = %s",p->value);
  LOGV("uci_ptr.p       = %u",(unsigned int) p->p);
  LOGV("uci_ptr.s       = %u",(unsigned int) p->s);
  LOGV("uci_ptr.o       = %u",(unsigned int) p->o);
  LOGV("uci_ptr.last    = %u",(unsigned int) p->last);
}

char* get_verdict_string(verdict v) {
  switch (v) {
    case UNSPEC:
      LOGW("Verdict string requested for unspecified verdict.");
      return NULL;
    case ACCEPT:
      return "ACCEPT";
    case DROP:
      return "DROP";
    case REJECT:
      return "REJECT";
  } 
  return NULL;
}

// takes as input a rule
// generates as output a hash, which is written to the pointer supplied as the first argument
void hash_rule(rule* r) {
  MD5_CTX c;
  unsigned char hash_bytes[MD5_DIGEST_LENGTH];
  char hash_string[512] = "\0";
  *r->hash = '\0'; // zero out existing hash
  // create string to be hashed
  // required
  snprintf(hash_string, sizeof(hash_string)-1, "%stype%d", hash_string, r->type);
  //if (r->src_vlan != 0) {
  // required
  snprintf(hash_string, sizeof(hash_string)-1, "%ssrc_vlan%d", hash_string, r->src_vlan);
  //}
  if (r->dst_vlan != 0) {
    snprintf(hash_string, sizeof(hash_string)-1, "%sdst_vlan%d", hash_string, r->dst_vlan);
  }
  if (r->proto != 0) { // 0 is technically IP protocol, but we don't support IP protocol, so this is an okay check to see if proto is set
    snprintf(hash_string, sizeof(hash_string)-1, "%sproto%s", hash_string, get_protocol_string(r->proto));
  }
  if (strncmp(r->src_ip, "\0", IP_ADDR_LEN) != 0) {
    snprintf(hash_string, sizeof(hash_string)-1, "%ssrc_ip%s", hash_string, r->src_ip);
  }
  if (strncmp(r->dst_ip, "\0", IP_ADDR_LEN) != 0) {
    snprintf(hash_string, sizeof(hash_string)-1, "%sdst_ip%s", hash_string, r->dst_ip);
  }
  if (r->src_port != 0) {
    snprintf(hash_string, sizeof(hash_string)-1, "%ssrc_port%d", hash_string, r->src_port);
  }
  if (r->dst_port != 0) {
    snprintf(hash_string, sizeof(hash_string)-1, "%sdst_port%d", hash_string, r->dst_port);
  }
  if (strncmp(r->device_name, "\0", DEVICE_NAME_SIZE) != 0) {
    snprintf(hash_string, sizeof(hash_string)-1, "%sdevice_name%s", hash_string, r->device_name);
  }
  // take MD5 hash
  MD5(hash_string, strlen(hash_string), hash_bytes);
  // convert hash_bytes into hex
  for (int i=0; i<sizeof(hash_bytes); i++) { // iterate over hash_bytes and append them in hex to r->hash
    snprintf(r->hash, sizeof(r->hash), "%s%02x", r->hash, hash_bytes[i]);
  }
}

//void print_sections(struct uci_package* pkg) {
//  struct uci_element* e;
//  //e = list_to_element((pkg->sections).next);
//  //for (;&e->list != &pkg->sections; ) {
//  //  e = list_to_element(e->list.next);
//  uci_foreach_element(&pkg->sections, e) {
//    LOGV("UCI section: %s", e->name);
//  }
//}

// input: the rule template to be written to config file, less device name
// input: the device name to add to the rule. NOTE: this device name needs to be stripped of domain
// return: 0 if rule was NOT added (probably because device name already exists), 1 if rule was added
int make_advertise_rule(rule* r, unsigned char* dn) {
  int ret;
  char* dot;
  // create new rule from template and add device name
  rule new_rule;
  memcpy((void*) &new_rule, (void*) r, sizeof(new_rule));
  // add device_name to new_rule and strip domain drom device_name
  strncpy(new_rule.device_name, dn, DEVICE_NAME_SIZE); // make copy so we can strip it safely
  dot = strchr(new_rule.device_name, '.');
  if (dot == NULL) { // if there was no '.' character, quit
    LOGW("Malformed device name: \"%s\"", dn);
    return 0;
  }
  *dot = '\0'; // effectively strip trailing parts of device name

  // TODO: need to check all the existing advertisement rules to see if any of them contain this device name
  // we're skipping this step right now because it's a corner case (albeit, one that can occur fairly easily) and we're short on time
  // essentially, if we've accepted a rule with say, 3 device names, and then get a packet containing one of those device names and another device name not accepted (but not blocked)
  // then we'll end up processing a packet here in Dreamcatcher with those two device names, and since we can't find the accepted device name via hash matching, we'll create two new rules
  // then, we'll have an accept run and a block rule for the same device name. As long as the accept rule is above the reject rule, it'll work fine -- it's just ugly on the UI to have duplicates

  ret = write_rule(&new_rule);
  return (ret == 0);

}

// input: the rule to be written to config file, less device name(s)
// input: the dns header containing device names
// return: 0 on success, nonzero on failure
int write_rule_advertise(rule* r, dns_header* dns) {
  // if Dreamcatcher got this packet, it means that at least one of the names in the packet was NOT accepted and none of the names were REJECTed by another rule
  // parse dns_header and iterate over all advertisement names (call make_advertise_rule())
    // skip/filter all device names that are previously ACCEPTed
    // create new rules to REJECT the names that aren't previously ACCEPTed
  unsigned char* dns_raw = (unsigned char*) dns; // get raw byte stream we can use for parsing the payload
  int i;
  char buf[DEVICE_NAME_SIZE];
  char temp_buf[DEVICE_NAME_SIZE]; // used only in PTR record
  int name_length;
  u_int16_t type;
  u_int16_t data_len;
  unsigned char* ptr = dns_raw + sizeof(*dns); // pointer to start of question/answer section in DNS packet
  unsigned char* dot;
	int num_rules = 0;

  // skip over questions
  for (i=0; i < dns->questions; i++) {
    ptr = skip_question(ptr); // skips the question we're currently looking at
  }
  // after questions come Answer Resource Requests, which we want to match
  // against the approved names in our info struct
  for (i=0; i < dns->answer_rr + dns->authority_rr + dns->additional_rr; i++) { // iterate over the answers
    name_length = read_dns_name(dns_raw, ptr, buf); // put name in buf
    ptr += name_length; // move ptr forward to type section
    type = ntohs(*(u_int16_t*)ptr); // translate from network byte order to host byte order
    ptr += 8; // move ptr past the TYPE (2 bytes), CLASS (2 bytes), and TTL (4 bytes) fields
    data_len = ntohs(*(u_int16_t*)ptr); // translate from network byte order to host byte order
    ptr += 2; // move ptr past the RDLENGTH field
    // if type is A, AAAA, SRV, TXT, use response from read_dns_name as device name
    // if type is PTR, SRV, read the RDATA field for the device name (or 2nd device name in case of SRV)
    // special case for PTR records, if name starts with _services._dns-sd.*, we check domain for _<proto>._<proto>.*, and if so, completely ignore this answer
    switch (type) {
      case A :
      case TXT :
      case AAAA :
        num_rules += make_advertise_rule(r, buf);
        break;
      case PTR :
#define SERVICES_STR "_services._dns-sd." 
				if (strncmp(buf, SERVICES_STR, sizeof(SERVICES_STR)-1) == 0) { // if name matches services prefix, check for special case
					read_dns_name(dns_raw, ptr, buf);
					// check that first two segments of domain start with underscores, as the spec (https://tools.ietf.org/html/rfc6763#section-9) says they should
					dot = strchr(buf, '.'); // find first dot in domain name
					if (buf[0] == '_' && dot != NULL && dot[1] == '_') {
						break; // don't check this device name
					} else { // weird, name matches services prefix, but domain doesn't conform
            num_rules += make_advertise_rule(r, buf);
						break; 
					}
					break;
				}
        // if we get here, then this is a normal PTR record, not one starting with _services._dns-sd
        read_dns_name(dns_raw, ptr, buf);
        num_rules += make_advertise_rule(r, buf);
        break;
      case SRV :
        // check first device name from above
        num_rules += make_advertise_rule(r, buf);
        // check second device name as target field
        read_dns_name(dns_raw, ptr+6, buf); // first 6 bytes are priority, weight, port
        // http://www.tahi.org/dns/packages/RFC2782_S4-1_0_0/SV/SV_RFC2782_SRV_rdata.html
        num_rules += make_advertise_rule(r, buf);
        break;
      default :
        LOGW("Unsupported DNS answer type: %u", type);
        return false;
    }
    ptr += data_len; // move p past RDATA field, ready for next iteration
  }

  // if we've parsed the packet and created at least one new rule, then return 0, else return 1
  return (num_rules == 0);

}

  
// input: the rule to be written to config file
// return: 0 on success, nonzero on failure
int write_rule(rule* r) {
  int fd;
  int ret;
  int lock_ret;
  int tries = 0;

//  struct uci_context* ctx;
//  struct uci_package* pkg;

  // print out the rule to be written
  LOGV("New rule:");
  LOGV("type:        %u", r->type);
  LOGV("src_vlan:    %u", r->src_vlan);
  LOGV("dst_vlan:    %u", r->dst_vlan);
  LOGV("protonum:    %u", r->proto);
  LOGV("protocol:    %s", get_protocol_string(r->proto));
  LOGV("src_ip:      %s", r->src_ip);
  LOGV("dst_ip:      %s", r->dst_ip);
  LOGV("src_port:    %u", r->src_port);
  LOGV("dst_port:    %u", r->dst_port);
  LOGV("target:      %d", r->target);
  LOGV("device_name: %s", r->device_name);

  // calculate hash of rule for its id
  hash_rule(r); // now r->hash stores the unique id for this rule

  // lock the config file
  LOGV("Locking config file");
  fd = -1;
  for (tries = 0; fd == -1 && tries < MAX_TRIES; tries++) {
    fd = lock_open_config();
  }
  if (fd == -1) {
    LOGE("Could not open or lock config file.");
    exit(1);
  }

  // write the rule out to the config file
  // initialize
  LOGV("Initializing config file context");
  ctx = uci_alloc_context();
  if (!ctx) {
    LOGW("Didn't properly initialize context");
  }
  ret = uci_load(ctx, "dreamcatcher", &pkg); // config file loaded into pkg
  if (ret != UCI_OK) {
    LOGW("Didn't properly load config file");
    uci_perror(ctx,""); // TODO: replace this with uci_get_errorstr() and use our own logging functions
  }

  // PRINT OUT ALL SECTIONS IN PACKAGE
  //print_sections(pkg);

  // see if this rule already exists
  if (rule_exists(ctx, r->hash)) {
    LOGV("Rule for %s already exists.", r->hash);
    ret = -1;
    goto cleanup;
  }

  // create new entry/section
  add_new_named_rule_section(ctx, r->hash);
  // populate section
  //rule_uci_set_str(ctx, r->hash, "message", r->message); // required
  rule_uci_set_int(ctx, r->hash, "type", r->type); // required
  if (r->src_vlan != 0) { // optional
    rule_uci_set_int(ctx, r->hash, "src_vlan", r->src_vlan);
  }
  if (r->dst_vlan != 0) { // optional
    rule_uci_set_int(ctx, r->hash, "dst_vlan", r->dst_vlan);
  }
  if (r->proto != 0) { // optional
    rule_uci_set_str(ctx, r->hash, "proto", get_protocol_string(r->proto));
  }
  if (strncmp(r->src_ip, "\0", IP_ADDR_LEN) != 0) { // optional
    rule_uci_set_str(ctx, r->hash, "src_ip", r->src_ip);
  }
  if (strncmp(r->dst_ip, "\0", IP_ADDR_LEN) != 0) { // optional
    rule_uci_set_str(ctx, r->hash, "dst_ip", r->dst_ip);
  }
  if (r->src_port != 0) { // optional
    rule_uci_set_int(ctx, r->hash, "src_port", r->src_port);
  }
  if (r->dst_port != 0) { // optional
    rule_uci_set_int(ctx, r->hash, "dst_port", r->dst_port);
  }
  if (strncmp(r->device_name, "\0", DEVICE_NAME_SIZE) != 0) { // optional
    rule_uci_add_list_str(ctx, r->hash, "device_name", r->device_name);
  }
  rule_uci_set_str(ctx, r->hash, "verdict", get_verdict_string(r->target)); // required
  rule_uci_set_int(ctx, r->hash, "approved", 0); // required, always set to 0 because the user has not approved it yet

  // save and commit changes
  //LOGV("Saving changes to config file");
  //ret = uci_save(ctx, pkg);
  //if (ret != UCI_OK) {
  //  LOGW("Didn't properly save config file.");
  //  uci_perror(ctx,""); // TODO: replace this with uci_get_errorstr() and use our own logging functions
  //}
  LOGV("Committing changes to config file");
  ret = uci_commit(ctx, &pkg, false); // false should be true, library got it backwards
  if (ret != UCI_OK) {
    LOGW("Didn't properly commit config file.");
    uci_perror(ctx,""); // TODO: replace this with uci_get_errorstr() and use our own logging functions
  }
  LOGV("Done adding rule to config file");

cleanup:
  // unlock the config file
  LOGV("Unlocking config file");
  lock_ret = -1;
  for (tries = 0; lock_ret == -1 && tries < MAX_TRIES; tries++) {
    lock_ret = unlock_close_config(fd);
  }
  if (lock_ret == -1) {
    LOGE("Could not unlock or close config file.");
    return 1;
  }

  return ret;
}

// returns 1 if exists, 0 if not
int rule_exists(struct uci_context* ctx, const char* hash) {
  struct uci_ptr ptr;
  char ptr_string[128];
  snprintf(ptr_string, sizeof(ptr_string), "dreamcatcher.%s", hash);
  uci_lookup_ptr(ctx, &ptr, ptr_string, false);
  return (ptr.s != NULL); // true if the target exists (ptr.s having a value means there's a pointer to an actual section struct)
}

void add_new_named_rule_section(struct uci_context* ctx, const char* hash) {
  struct uci_ptr ptr;
  char ptr_string[128];
  snprintf(ptr_string, sizeof(ptr_string), "dreamcatcher.%s=rule", hash);
  uci_lookup_ptr(ctx, &ptr, ptr_string, false);
  uci_set(ctx, &ptr);
}

void rule_uci_set_int(struct uci_context* ctx, const char* hash, const char* option, const unsigned int value) {
  struct uci_ptr ptr;
  char ptr_string[128];
  snprintf(ptr_string, sizeof(ptr_string), "dreamcatcher.%s.%s=%d", hash, option, value);
  uci_lookup_ptr(ctx, &ptr, ptr_string, false);
  uci_set(ctx, &ptr);
}

void rule_uci_set_str(struct uci_context *ctx, const char* hash, const char* option, const char* value) {
  struct uci_ptr ptr;
  char ptr_string[128];
  snprintf(ptr_string, sizeof(ptr_string), "dreamcatcher.%s.%s=%s", hash, option, value);
  uci_lookup_ptr(ctx, &ptr, ptr_string, false);
  uci_set(ctx, &ptr);
}

void rule_uci_add_list_str(struct uci_context *ctx, const char* hash, const char* option, const char* value) {
  struct uci_ptr ptr;
  char ptr_string[128];
  snprintf(ptr_string, sizeof(ptr_string), "dreamcatcher.%s.%s=%s", hash, option, value);
  uci_lookup_ptr(ctx, &ptr, ptr_string, false);
  uci_add_list(ctx, &ptr);
}

// reads from ptr the dns name using dns's weird encoding
// uses payload in case ptr refers to a location of a previous substring
// if not passed in as NULL, buf is filled with the resulting string
// returns the length read, in bytes (Note: if we hit a pointer, it returns the number of bytes from the start to the end of the pointer, not the length of the name returned)
// NOTE: make sure this is the start of a name section, because this function cannot tell and will behave erratically if not
// NOTE: This ONLY reads the name until it gets to a null byte. It does not include, for instance, the 4 bytes for QTYPE/QCLASS fields in a question
unsigned int read_dns_name(unsigned char* payload, unsigned char* start, char* buf) {
  int num_bytes = 0; // return value; can be set at any time if we hit a reference, or if we get to end, will be set 
  unsigned char* ptr = start;
  u_int16_t offset; // offset into payload; used when we have a pointer
  if (buf != NULL) {
    *buf = '\0'; // clear buf, just in case
  }
  // iterate until we reach a null byte
  while (*ptr != 0) {
    // check if next segment is a reference 
    if ((*ptr & 192) == 192) { // it's a pointer if first two bits are set 0b11000000
      // pointer is two bytes, network order
      offset = ntohs(*(u_int16_t*)ptr); // get offset (including flag bits)
      offset &= 16383; // mask off flag bits (mask == 0b0011111111111111)
      if (num_bytes == 0) { // if num_bytes isn't set yet, aka this is the first pointer
        num_bytes = (ptr+2) - start; // how many bytes has ptr gone?
      }   
      ptr = payload + offset; // set ptr to next segment
    } else { // not a pointer, so add next segment
      if (buf != NULL) { 
        snprintf(buf, DEVICE_NAME_SIZE, "%s%.*s", buf, *ptr, (ptr+1)); // append next segment of device name
      }
      ptr += ((*ptr) + 1); // ptr points at the number of characters in this segment of the name, so move it to the end of the segment 
      if (buf != NULL && *ptr != 0) { // if we're going to add another segment
        snprintf(buf, DEVICE_NAME_SIZE, "%s.", buf); // append a '.' character between segments
      } 
    }   
  }
  if (num_bytes == 0) { // if num_chars hasn't been set yet, aka there were no pointers in this name
    // when ptr == '\0' to end the string, we need to move 1 byte more to get past it
    num_bytes = (ptr+1) - start; // how many bytes has ptr gone?
  }
  if (num_bytes < 0) { // this should never happen, even if packet is invalid -- it means our arithmetic is off
    LOGW("We have a bug! payload: 0x%x, start: 0x%x, ptr: 0x%x, buf: %s", payload, start, ptr, buf);
    exit(-1);
  }
  return (unsigned int) num_bytes;
}

// returns a pointer to after the dns question
unsigned char* skip_question(unsigned char* p) {
  unsigned char* ptr = p;
  while (*ptr != 0) {
    if ((*ptr & 192) == 192) { // it's a pointer if first two bits are set 0b11000000
      return ptr+2; // skip the two-byte pointer and we're done
    } else { // if it's not a pointer, read the next segment
      ptr += ((*ptr) + 1); // ptr points at the number of characters in this segment of the name, so move it to the end of the segment
    }
  }
  // after the name is done, there are the null byte ptr is pointing at, and QTYPE and QCLASS fields, each is 2 bytes
  return ptr+5;
}

// parses a dns payload, reads all the answers, puts all the answers in result (which must be freed later)
// result must be a pre-allocated char[2*N][M] where N is equal to dns->answer_rr and M is DEVICE_NAME_SIZE
// SRV records can have multiple device names (the "name" and the "target") so we need 2x buffers for them -- the extras will go unused
// returns 0 on success, -1 on failure
int parse_dns_answers(dns_header* dns, unsigned char* payload, char** result) {
  unsigned char* p = payload + sizeof(*dns); // pointer to start of question/answer section
  char buf[DEVICE_NAME_SIZE];
  int name_length;
  int ptr_found = 0;
  u_int16_t type;
  u_int16_t data_len;
  // skip over the questions
  for (int i = 0; i < dns->questions; i++) {
    p = skip_question(p); // moves p forward past all the questions to the answers
  } // p now points at the answer section of the dns record
  LOGV("Skipped over questions");
  // parse the answers and put them all in result (duplicates are fine for this rough draft, they just take up more time)
  for (int i = 0, j = 0; i < dns->answer_rr; i++) {
    name_length = read_dns_name(payload, p, buf); // put name in buf
    p += name_length; // move p forward to type section
    type = ntohs(*(u_int16_t*)p); // translate from network byte order to host byte order
    p += 8; // move p past the TYPE (2 bytes), CLASS (2 bytes), and TTL (4 bytes) fields
    data_len = ntohs(*(u_int16_t*)p); // translate from network byte order to host byte order
    p += 2; // move p past the RDLENGTH field
    // if type is A, AAAA, SRV, TXT, use response from read_dns_name as device name
    // if type is PTR, SRV, read the RDATA field for the device name (or 2nd device name in case of SRV)
    // special case for PTR records, if name starts with _services._dns-sd.*, we check domain for _<proto>._<proto>.*, and if so, completely ignore this answer (allowing the packet, if otherwise the case)
    switch (type) {
      case A :
      case TXT :
      case AAAA :
        result[j] = calloc(sizeof(char), DEVICE_NAME_SIZE);
        snprintf(result[j++], DEVICE_NAME_SIZE, "%s", strtok(buf, ".")); // this will mess up on strings that actually have an escaped '.' char
        break;
      case PTR :
        if (strncmp(buf, "_services._dns-sd.", 18) == 0) { // if name matches services prefix, check for special case
          read_dns_name(payload, p, buf); // first 6 bytes are priority, weight, port
          // check that first two segments of domain start with underscores, as the spec (https://tools.ietf.org/html/rfc6763#section-9) says they should
          if (strncmp(strtok(buf, "."), "_", 1) == 0 &&
              strncmp(strtok(NULL, "."), "_", 1) == 0) { // they do!
            break; // don't add this device name
          } else { // weird, name matches services prefix, but domain doesn't conform
            LOGI("Weird case. DNS name starts with _services._dns-sd., but domain doesn't start with underscores.");
            result[j] = calloc(sizeof(char), DEVICE_NAME_SIZE);
            snprintf(result[j++], DEVICE_NAME_SIZE, "%s", buf); // we already used strtok on buf, so it should already have a '\0' where the first '.' was and just return the first segment like we want
            break;
          }
        }
        read_dns_name(payload, p, buf); // first 6 bytes are priority, weight, port
        result[j] = calloc(sizeof(char), DEVICE_NAME_SIZE);
        snprintf(result[j++], DEVICE_NAME_SIZE, "%s", strtok(buf, ".")); // this will mess up on strings that actually have an escaped '.' char
        break;
      case SRV :
        // set first device name as buf from above
        result[j] = calloc(sizeof(char), DEVICE_NAME_SIZE);
        snprintf(result[j++], DEVICE_NAME_SIZE, "%s", strtok(buf, ".")); // this will mess up on strings that actually have an escaped '.' char
        // set second device name as target field
        read_dns_name(payload, p+6, buf); // first 6 bytes are priority, weight, port
        result[j] = calloc(sizeof(char), DEVICE_NAME_SIZE);
        snprintf(result[j++], DEVICE_NAME_SIZE, "%s", strtok(buf, ".")); // this will mess up on strings that actually have an escaped '.' char
        break;
      default :
        LOGW("Unsupported DNS answer type: %u", type);
        return -1;
    }
    p += data_len; // move p past RDATA field, ready for next iteration
  }
  return 0;
}

// frees our dynamically created array of strings
void free_names(char** names) {
  for (int i = 0; names[i] != 0; i++) {
    free(names[i]);
  }
  free(names);
}


// returns 1 if we create any new dpi_rules, -1 if we do not (returning 0 reloads the firewall, which we do not want to do)
// sets verdict to NF_ACCEPT if all applicable existing rules specify ACCEPT target
// sets rule.device_name for each device name (may create/check multiple rules) if a new ADVERTISE rule needs to be made
//int check_dpi_rule(rule* r, dns_header* dns, unsigned char* payload, u_int32_t* verdict) {
//  int fd;
//  int ret = -1; // default if we don't create any new rules
//  int load_ret;
//  int lock_ret;
//  int parse_ret;
//  int exists_ret;
//  int create_ret;
//  int tries = 0;
//  u_int32_t flag = 1; // gets set to 0 if packet should be blocked
//  u_int32_t temp_verdict;
//  char buf[DEVICE_NAME_SIZE];
//
//  // Allocate names struct
//  char** names = calloc(sizeof(char*), (dns->answer_rr*2)+1); // array of char*'s (individual char*'s will be allocated later). the +1 is so we are guaranteed to have a null pointer at the end
//
//  struct uci_context* ctx;
//  struct uci_package* pkg;
//  struct uci_element* e;
//
//  if (dns->qr == 1) { // only the ADVERTISE rules should have the device name included in the rule
//    r->type = ADVERTISE;
//    LOGV("DNS Response packet, parsing answers");
//    parse_ret = parse_dns_answers(dns, payload, (char**) names); // populates names with names for new rules, some names will be empty
//    if (parse_ret == -1) {
//      free_names(names);
//      return -1;
//    }
//  } else if (dns->qr == 0) { // if it's a discover packet
//    r->type = DISCOVER;
//    read_dns_name(payload, payload + sizeof(*dns), buf); // log the question name, just because I'm curious
//    LOGV("This dns packet refers to %s", buf);
//  }
//
//  // lock the config file
//  LOGV("Locking config file");
//  fd = -1;
//  for (tries = 0; fd == -1 && tries < MAX_TRIES; tries++) {
//    fd = lock_open_config();
//  }
//  if (fd == -1) {
//    LOGE("Could not open or lock config file.");
//    free_names(names);
//    return 1; // not sure what to do here, for now just say the rule exists (default block) and do nothing
//  }
//
//  // initialize uci context for dreamcatcher file
//  LOGV("Initializing config file context");
//  ctx = uci_alloc_context();
//  if (!ctx) {
//    LOGW("Didn't properly initialize context");
//  }
//  load_ret = uci_load(ctx, "dreamcatcher", &pkg); // config file loaded into pkg
//  if (load_ret != UCI_OK) {
//    LOGW("Didn't properly load config file");
//    uci_perror(ctx,""); // TODO: replace this with uci_get_errorstr() and use our own logging functions
//  }
//
//  LOGV("Checking existing DPI rules to see if they match this packet.");
//  if (r->type == DISCOVER) {
//    // (re)calculate hash of rule
//    hash_rule(r); // now r->hash stores the unique id for this rule
//    exists_ret = dpi_rule_exists(ctx, r->hash, verdict); // verdict is set if rule is found
//    LOGV("Rule already exists: %d", exists_ret);
//    if (exists_ret == 0) { // if rule doesn't exist yet
//      // create new rule
//      create_ret = write_rule(r);
//      if (create_ret == 0) {
//        LOGD("Alerting user");
//        alert_user();
//        ret = 1;
//      } else {
//        LOGV("Did not write rule to the config file.");
//      }
//    }
//  } else if (r->type == ADVERTISE) {
//    // iterate over all the device names, check if their rules ALL exist AND say ACCEPT
//    for (int i = 0; names[i] != NULL; i++) { // iterate over our list of names
//      strncpy(r->device_name, names[i], DEVICE_NAME_SIZE); // set device name to this name
//      LOGV("Checking if rule exists for device name %s", r->device_name);
//      // (re)calculate hash of rule
//      hash_rule(r); // now r->hash stores the unique id for this rule
//      // check if this rule exists
//      if (dpi_rule_exists(ctx, r->hash, &temp_verdict)) { // verdict is set if rule is found
//        LOGV("Device %s exists", r->device_name);
//        if (temp_verdict != NF_ACCEPT) { // if not all of the existing rules say ACCEPT
//          flag = 0;
//        }
//      } else { // rule does not already exist
//        LOGV("Device %s does not exist", r->device_name);
//        flag = 0; // we need to create at least one new rule because of this packet, so it should be blocked
//        // create new rule
//        create_ret = write_rule(r);
//        if (create_ret == 0) {
//          LOGD("Alerting user");
//          alert_user();
//          ret = 1;
//        } else {
//          LOGV("Did not write rule to the config file.");
//        }
//      }
//    }
//    if (flag == 1) { // if we never set flag to 0
//      LOGV("All devices existed in the dpi_rules and had ACCEPT rules, so we can ACCEPT this packet! Yay!");
//      *verdict = NF_ACCEPT; // then we can accept the packet! yay!
//    }
//  }
//
//  // unlock the config file
//  LOGV("Unlocking config file");
//  lock_ret = -1;
//  for (tries = 0; lock_ret == -1 && tries < MAX_TRIES; tries++) {
//    lock_ret = unlock_close_config(fd);
//  }
//  if (lock_ret == -1) {
//    LOGE("Could not unlock or close config file.");
//    free_names(names);
//    return 1; // not sure what to do here, for now just say the rule exists (default block) and do nothing
//  }
//
//  free_names(names);
//  return ret;
//}

// returns 1 if exists, 0 if not
// if rule exists, sets verdict
int dpi_rule_exists(struct uci_context* ctx, const char* hash, u_int32_t* verdict) {
  struct uci_ptr ptr;
  char ptr_string[128];
  struct uci_parse_option popt = {"verdict",0};
  struct uci_option* opt;

  snprintf(ptr_string, sizeof(ptr_string), "dreamcatcher.%s", hash);
  uci_lookup_ptr(ctx, &ptr, ptr_string, false);
  if (ptr.s == NULL) { // false if the target exists (ptr.s having a value means there's a pointer to an actual section struct)
    return 0;
  }
  // find verdict
  uci_parse_section(ptr.s, &popt, 1, &opt); // finds verdict and puts it in opt
  LOGV("existing rule: %s -> %s", opt->e.name, opt->v.string);
  if (strncmp("ACCEPT", opt->v.string, sizeof("ACCEPT")) == 0) { // if verdict is ACCEPT
    *verdict = NF_ACCEPT;
    LOGD("Accepting dpi rule.");
  } // else leave to default NF_DROP
  return 1;
}

// return fd if successful, -1 if failed
// block until we get a handle
int lock_open_config() {
  int fd;
  struct flock fl;

  // open file to get file descriptor
  fd = open(CONFIG_FILE, O_RDWR);
  if (fd == -1){
    LOGW("Can't open config file %s.", CONFIG_FILE);
    return -1;
  }

  if (lock_references > 0) { // if we already have a lock
    LOGV("We already have at least one lock");
    goto success_lock;
  }

  // prep file lock
  fl.l_type = F_WRLCK;
  fl.l_whence = SEEK_SET;
  fl.l_start = 0;
  fl.l_len = 0;

  // set a lock on the file and wait if it's already locked
  if (fcntl(fd, F_SETLKW, &fl) == -1) { // if the lock fails
    close(fd);
    LOGW("Locking config file failed.");
    return -1;
  } 

success_lock:
  // if it succeeds and we have a write lock
  lock_references += 1;
  return fd;
}

int unlock_close_config(int fd) { 
  struct flock fl;
  int retval = 0;

  if (lock_references > 1) { // if we have multiple lock_references
    LOGV("We have more than one lock to release");
    goto success_unlock;
  }

  // prep file unlock
  fl.l_type = F_UNLCK;
  fl.l_whence = SEEK_SET;
  fl.l_start = 0;
  fl.l_len = 0;

  // unlock file
  if (fcntl(fd, F_SETLKW, &fl) == -1) { 
    LOGW("Error unlocking config file.");
    retval = -1; 
  }

success_unlock:
  // close file descriptor
  close(fd);

  lock_references -= 1;
  return retval;
}

