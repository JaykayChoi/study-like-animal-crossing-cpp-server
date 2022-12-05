#include "crypto.h"

#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/ecdh.h>
#include <openssl/err.h>
#include <openssl/aes.h>

// Initialization vector.
const static unsigned char IV[16] = {
    0x00, 0x01, 0x02, 0x03,
    0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b,
    0x0c, 0x0d, 0x0e, 0x0f};

CryptoContext::CryptoContext()
    : bnCtx_(nullptr), myPublicKey_(nullptr), ecGroup_(nullptr)
{
}

CryptoContext::~CryptoContext()
{
    CleanUp();
}

bool CryptoContext::Init()
{
    CleanUp();

    bnCtx_ = BN_CTX_new();
    if (!bnCtx_)
    {
        Log("Failed to create bignum context! (%d)", ERR_get_error());
        return false;
    }

    myPublicKey_ = EC_KEY_new();
    if (!myPublicKey_)
    {
        Log("Failed to create my key! (%d)", ERR_get_error());
        return false;
    }

    ecGroup_ = EC_GROUP_new_by_curve_name(NID_secp224r1);
    if (!ecGroup_)
    {
        Log("Failed to create ec group! (%d)", ERR_get_error());
        return false;
    }

    if (EC_KEY_set_group(myPublicKey_, ecGroup_) != 1)
    {
        Log("Failed to set group! (%d)", ERR_get_error());
        return false;
    }

    if (EC_KEY_generate_key(myPublicKey_) != 1)
    {
        Log("Failed to generate key! (%d)", ERR_get_error());
        return false;
    }

    if (EC_KEY_check_key(myPublicKey_) != 1)
    {
        Log("Failed to check key! (%d)", ERR_get_error());
        return false;
    }

    const EC_POINT *ecPoint = EC_KEY_get0_public_key(myPublicKey_);
    ASSERT(ecPoint);

    char *myKeyHexBuf = EC_POINT_point2hex(ecGroup_, ecPoint, POINT_CONVERSION_UNCOMPRESSED, bnCtx_);
    ASSERT(myKeyHexBuf);

    myPublicKeyStr_ = std::string(myKeyHexBuf);
    OPENSSL_free(myKeyHexBuf);

    Log("MyPublicKey: %s", myPublicKeyStr_.c_str());

    return true;
}

void CryptoContext::CleanUp()
{
    if (bnCtx_)
    {
        BN_CTX_free(bnCtx_);
        bnCtx_ = nullptr;
    }

    if (myPublicKey_)
    {
        EC_KEY_free(myPublicKey_);
        myPublicKey_ = nullptr;
    }

    if (ecGroup_)
    {
        EC_GROUP_clear_free(ecGroup_);
        ecGroup_ = nullptr;
    }

    myPublicKeyStr_.clear();
    aesKey_.clear();
}

bool CryptoContext::ComputeSecret(const std::string &peerPubKeyStr)
{
    Log("Peer key: %s", peerPubKeyStr.c_str());

    EC_POINT *peerEcPoint = EC_POINT_new(ecGroup_);
    EC_POINT_hex2point(ecGroup_, peerPubKeyStr.c_str(), peerEcPoint, bnCtx_);
    if (!peerEcPoint)
    {
        Log("Failed to translate peer ec point! (%d)",
            ERR_get_error());
        return false;
    }

    int fieldSize = EC_GROUP_get_degree(EC_KEY_get0_group(myPublicKey_));

    std::vector<uint8> secret((fieldSize + 7) / 8);
    std::fill(std::begin(secret), std::end(secret), 0);

    int secretLen = ECDH_compute_key(secret.data(), secret.size(), peerEcPoint, myPublicKey_, nullptr);
    if (secretLen <= 0)
    {
        Log("Failed compute secret! (%d)", ERR_get_error());
        return false;
    }

    // Print secret.
    // std::string SecretStr;
    // for (int i = 0; i < Secret.Num(); i++)
    //{
    //	SecretStr += std::string::Printf(TEXT("%.2x"), Secret[i]);
    //}
    // Log("SecretStr: %s", *SecretStr);

    // Use the first 16 bytes as the AES key.
    aesKey_.assign(std::begin(secret), std::end(secret));

    return true;
}

bool CryptoContext::Encrypt(
    const uint8 *body,
    int bodyLen,
    std::vector<uint8> &encBody,
    int &outEncryptedSize)
{
    const static int MAX_PADDING_LEN = 16;

    EVP_CIPHER_CTX *cipherCtx = EVP_CIPHER_CTX_new();
    if (!cipherCtx)
    {
        Log("Failed create cipher context! (%d)", ERR_get_error());
        return false;
    }

    if (EVP_EncryptInit_ex(cipherCtx, EVP_aes_128_cbc(), nullptr, aesKey_.data(), IV) != 1)
    {
        Log("Failed init encryption! (%d)", ERR_get_error());
        EVP_CIPHER_CTX_free(cipherCtx);
        return false;
    }

    encBody.clear();
    // TODO reserve?
    encBody.resize(bodyLen + MAX_PADDING_LEN);

    int numBytesWritten = 0;
    if (EVP_EncryptUpdate(cipherCtx, encBody.data(), &numBytesWritten, body, bodyLen) != 1)
    {
        Log("Failed encrypt! (%d)", ERR_get_error());
        EVP_CIPHER_CTX_free(cipherCtx);
        return false;
    }

    int cipherTextLen = numBytesWritten;

    if (EVP_EncryptFinal_ex(cipherCtx, encBody.data() + numBytesWritten, &numBytesWritten) != 1)
    {
        Log("Failed finalize encryption! (%d)", ERR_get_error());
        EVP_CIPHER_CTX_free(cipherCtx);
        return false;
    }

    cipherTextLen += numBytesWritten;

    EVP_CIPHER_CTX_free(cipherCtx);

    outEncryptedSize = cipherTextLen;

    // Print original.
    // std::string OrgStr;
    // for (int i = 0; i < BodyLen; i++)
    //{
    //	OrgStr += std::string::Printf(TEXT("%.2x"), Body[i]);
    //}
    // Log("Encrypting...: %s", *OrgStr);

    // Print encrypted.
    // std::string TempStr;
    // for (int i = 0; i < EncBody.Num(); i++)
    //{
    //	TempStr += std::string::Printf(TEXT("%.2x"), EncBody[i]);
    //}
    // Log("Encrypted: %s", *TempStr);

    return true;
}

bool CryptoContext::Decrypt(
    const uint8 *body,
    int bodyLen,
    std::vector<uint8> &decBody,
    int &outDecryptedSize)
{
    const static int MAX_PADDING_LEN = 16;

    EVP_CIPHER_CTX *decipherCtx = EVP_CIPHER_CTX_new();
    if (!decipherCtx)
    {
        Log("Failed create decipher context! (%d)", ERR_get_error());
        return false;
    }

    if (EVP_DecryptInit_ex(decipherCtx, EVP_aes_128_cbc(), nullptr, aesKey_.data(), IV) != 1)
    {
        Log("Failed init decryption! (%d)", ERR_get_error());
        EVP_CIPHER_CTX_free(decipherCtx);
        return false;
    }

    decBody.clear();
    // TODO reserve?
    decBody.resize(bodyLen);

    int numBytesWritten = 0;
    if (EVP_DecryptUpdate(decipherCtx, decBody.data(), &numBytesWritten, body, bodyLen) != 1)
    {
        Log("Failed create decrypt! (%d)", ERR_get_error());
        return false;
    }

    int plainTextLen = numBytesWritten;

    if (EVP_DecryptFinal_ex(decipherCtx, decBody.data() + numBytesWritten, &numBytesWritten) != 1)
    {
        Log("Failed finalize encryption! (%d)", ERR_get_error());
        EVP_CIPHER_CTX_free(decipherCtx);
        return false;
    }

    plainTextLen += numBytesWritten;

    EVP_CIPHER_CTX_free(decipherCtx);

    outDecryptedSize = plainTextLen;

    // Print decrypted.
    // std::string TempStr;
    // for (int i = 0; i < DecBody.Num(); i++)
    //{
    //	TempStr += std::string::Printf(TEXT("%.2x"), DecBody[i]);
    //}
    // Log("Decrypted: %s", *TempStr);

    return true;
}
