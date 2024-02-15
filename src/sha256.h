#ifndef DEPINC_MINER_SHA256_H
#define DEPINC_MINER_SHA256_H

#include <openssl/evp.h>

class CSHA256
{
    EVP_MD_CTX* m_ctx;

public:
    static const int OUTPUT_SIZE = 256/8;

    CSHA256();

    CSHA256(CSHA256 const&) = delete;
    CSHA256& operator=(CSHA256 const&) = delete;

    CSHA256(CSHA256&&);
    CSHA256& operator=(CSHA256&&);

    ~CSHA256();

    void Write(void const* p, uint64_t size);

    void Finalize(uint8_t* pout);
};

#endif