#pragma once

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <cstdint>
#include <memory>
#include <vector>

// Android SyntheticPasswordCrypto framing: 12-byte nonce, ciphertext, 16-byte tag.
// Never release the decrypted secret unless AEAD authentication succeeds.
inline bool OpenCeInnerGcm(const uint8_t* key, const std::vector<uint8_t>& sealed,
                           std::vector<uint8_t>* plaintext) {
    constexpr size_t nonce_size = 12, tag_size = 16;
    if (!key || !plaintext || !plaintext->empty() ||
        sealed.size() <= nonce_size + tag_size || sealed.size() > 65536) return false;
    const int n = sealed.size() - nonce_size - tag_size;
    std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)> ctx(
        EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
    if (!ctx) return false;
    std::vector<uint8_t> tmp(n + EVP_MAX_BLOCK_LENGTH);
    int first = 0, last = 0;
    const bool ok = EVP_DecryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, key, sealed.data()) == 1 &&
        EVP_DecryptUpdate(ctx.get(), tmp.data(), &first, sealed.data()+nonce_size, n) == 1 &&
        first >= 0 && first <= n &&
        EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_TAG, tag_size,
                            const_cast<uint8_t*>(sealed.data()+sealed.size()-tag_size)) == 1 &&
        EVP_DecryptFinal_ex(ctx.get(), tmp.data()+first, &last) == 1 &&
        last >= 0 && first + last == n;
    if (ok) plaintext->assign(tmp.data(), tmp.data()+n);
    OPENSSL_cleanse(tmp.data(), tmp.size());
    return ok;
}
