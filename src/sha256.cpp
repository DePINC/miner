#include "sha256.h"

CSHA256::CSHA256()
    : m_ctx(EVP_MD_CTX_create())
{
    EVP_DigestInit(m_ctx, EVP_sha256());
}

CSHA256::~CSHA256()
{
    if (m_ctx != nullptr) {
        EVP_MD_CTX_free(m_ctx);
    }
}

CSHA256::CSHA256(CSHA256&& rhs)
    : m_ctx(rhs.m_ctx)
{
    rhs.m_ctx = nullptr;
}

CSHA256& CSHA256::operator=(CSHA256&& rhs)
{
    if (&rhs != this) {
        m_ctx = rhs.m_ctx;
        rhs.m_ctx = nullptr;
    }
    return *this;
}

void CSHA256::Write(void const* p, uint64_t size)
{
    EVP_DigestUpdate(m_ctx, p, size);
}

void CSHA256::Finalize(uint8_t* pout)
{
    EVP_DigestFinal(m_ctx, pout, nullptr);
}
