//
// Created by Han Zhang on 8/23/19.
//

#ifndef CAPTURE_CORE_CRYPTO_HPP
#define CAPTURE_CORE_CRYPTO_HPP

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include "logger.hpp"
#include "credential.h"

// Generate new pair of public and private RSA keys
// Returns 0 if success
int generate_key_pair(const char *public_key_file, const char *private_key_file);

void free_all(struct bio_st *bp_public, struct bio_st *bp_private,
              struct rsa_st *r, struct bignum_st *bne);

std::string gen_random(int len);

void gen_random(char *s, int len);

#endif //CAPTURE_CORE_CRYPTO_HPP
