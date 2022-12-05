#pragma once

#include "../global.h"

struct bignum_ctx;
struct ec_key_st;
struct ec_group_st;

class CryptoContext
{
public:
    explicit CryptoContext();
    ~CryptoContext();

    bool Init();
    void CleanUp();

    const std::string& GetMyPublicKey() { return myPublicKeyStr_; }
    bool ComputeSecret(const std::string& PeerPubKeyStr);

    bool Encrypt(const uint8* body, int bodyLen, std::vector<uint8>& encBody,
        int& outEncryptedSize);

    bool Decrypt(const uint8* body, int bodyLen, std::vector<uint8>& decBody,
        int& outDecryptedSize);

private:
    bignum_ctx* bnCtx_;
    ec_key_st* myPublicKey_;
    ec_group_st* ecGroup_;

    std::string myPublicKeyStr_;
    std::vector<uint8> aesKey_;
};
