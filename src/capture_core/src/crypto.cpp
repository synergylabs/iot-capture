//
// Created by Han Zhang on 8/23/19.
//

#include <pthread.h>
#include <stdlib.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <cstring>

#include "crypto.hpp"
#include "logger.hpp"

#define TAG "Crypto"
#define FAIL 1
#define SUCCESS 0

using std::string;

/*
 * - generate self public & private key pair
 */
int generate_key_pair(const char *public_key_file, const char *private_key_file) {
    int ret = 0;
    RSA *r = nullptr;
    BIGNUM *bne = nullptr;
    BIO *bp_public = nullptr, *bp_private = nullptr;

    const int bits = 2048;
    const unsigned long e = RSA_F4;

    // Check if key files are empty
    if (public_key_file == nullptr || private_key_file == nullptr) {
        LOGE("empty key file location");
    }

    // 1. generate rsa key
    bne = BN_new();
    ret = BN_set_word(bne, e);
    if (ret != 1) {
        free_all(bp_public, bp_private, r, bne);
        return FAIL;
    }

    r = RSA_new();
    ret = RSA_generate_key_ex(r, bits, bne, nullptr);
    if (ret != 1) {
        free_all(bp_public, bp_private, r, bne);
        return FAIL;
    }

    // 2. save public key
    bp_public = BIO_new_file(public_key_file, "w+");
    ret = PEM_write_bio_RSAPublicKey(bp_public, r);
    if (ret != 1) {
        free_all(bp_public, bp_private, r, bne);
        return FAIL;
    }

    // 3. save private key
    bp_private = BIO_new_file(private_key_file, "w+");
    ret = PEM_write_bio_RSAPrivateKey(bp_private, r, nullptr, nullptr, 0, nullptr, nullptr);

    // 4. free
    free_all(bp_public, bp_private, r, bne);

    return ret == 1 ? SUCCESS : FAIL;
}

void free_all(struct bio_st *bp_public, struct bio_st *bp_private,
              struct rsa_st *r, struct bignum_st *bne) {
    BIO_free_all(bp_public);
    BIO_free_all(bp_private);
    RSA_free(r);
    BN_free(bne);
}

string gen_random(const int len) {
    char* tmp = new char[static_cast<size_t >(len) + 1];

    gen_random(tmp, len);

    return string(tmp);
}

void gen_random(char *s, const int len) {
    static const char alphanum[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    memset(s, 0, (size_t) len);

    // Set random seed.
    srand((unsigned int) time(0));

    for (int i = 0; i < len; ++i) {
        s[i] = alphanum[(unsigned long) rand() % (sizeof(alphanum) - 1)];
    }

    s[len] = 0;
}
