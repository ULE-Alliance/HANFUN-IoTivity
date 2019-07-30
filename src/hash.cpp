#include "hash.h"

#include "log.h"
#ifdef WITH_POSIX
#include <openssl/sha.h>
#endif

void Hash(OCUUIdentity *id, const char *uid)
{
  LOG(LOG_DEBUG, "uid=%s", uid);

  static const OCUUIdentity ns =
  {
    0xeb, 0x82, 0x42, 0xfd,
    0x5d, 0x95,
    0x46, 0x92,
    0x80, 0xf6,
    0x79, 0x32, 0x20, 0x61, 0xb8, 0xff
  };
  uint8_t digest[20];
#ifdef WITH_POSIX
  SHA_CTX ctx;
  int ret = SHA1_Init(&ctx);
  if (ret)
  {
    ret = SHA1_Update(&ctx, ns.id, UUID_IDENTITY_SIZE);
  }
  if (ret && uid)
  {
    ret = SHA1_Update(&ctx, uid, strlen(uid));
  }
  if (ret)
  {
    ret = SHA1_Final(digest, &ctx);
  }
  if (!ret)
  {
    LOG(LOG_ERR, "SHA1 - %d", ret);
  }
#elif _WIN32
  BCRYPT_ALG_HANDLE hAlg = NULL;
  BCRYPT_HASH_HANDLE hHash = NULL;
  DWORD cbHashObject;
  uint8_t *pbHashObject = NULL;
  NTSTATUS status;
  status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA1_ALGORITM, NULL, 0);
  if (BCRYPT_SUCCESS(status))
  {
    DWORD cbResult;
    status = BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&cbHashObject, sizeof(DWORD), &cbResult, 0);
  }
  if (BCRYPT_SUCCESS(status))
  {
    pbHashObject = new uint8_t[cbHashObject];
    status = BCryptCreateHash(hAlg, &hHash, pbHashObject, cbHashObject, NULL, 0, 0);
  }
  if (BCRYPT_SUCCESS(status))
  {
    status = BCryptHashData(hHash, (PUCHAR)ns.id, UUID_IDENTITY_SIZE, 0);
  }
  if (BCRYPT_SUCCESS(status) && uid)
  {
    status = BCryptHashData(hHash, (PUCHAR)uid, strlen(uid), 0);
  }
  if (BCRYPT_SUCCESS(status))
  {
    status = BCryptFinishHash(hHash, digest, 20, 0);
  }
  if (!BCRYPT_SUCCESS(status))
  {
    LOG(LOG_ERR, "SHA1 - 0x%x", status);
  }
  if (hAlg)
  {
    BCryptCloseAlgorithmProvider(hAlg, 0);
  }
  if (hHash)
  {
    BCryptDestroyHash(hHash);
  }
  delete[] pbHashObject;
#else
#error Missing implementation of SHA-1 hash
#endif
  digest[7] = (digest[7] & 0x0f) | 0x50;
  digest[8] = (digest[8] & 0x3f) | 0x80;
  memcpy(id->id, digest, UUID_IDENTITY_SIZE);
}

void Hash(OCUUIdentity *id, uint8_t *ipui, uint8_t *emc)
{
  static const OCUUIdentity ns =
  {
    0xeb, 0x82, 0x42, 0xfd,
    0x5d, 0x95,
    0x46, 0x92,
    0x80, 0xf6,
    0x79, 0x32, 0x20, 0x61, 0xb8, 0xff
  };
  uint8_t digest[20];
#ifdef WITH_POSIX
  SHA_CTX ctx;
  int ret = SHA1_Init(&ctx);
  if (ret)
  {
    ret = SHA1_Update(&ctx, ns.id, UUID_IDENTITY_SIZE);
  }
  if (ret && ipui)
  {
    ret = SHA1_Update(&ctx, ipui, 5);
  }
  if (ret && emc)
  {
    ret = SHA1_Update(&ctx, emc, 2);
  }
  if (ret)
  {
    ret = SHA1_Final(digest, &ctx);
  }
  if (!ret)
  {
    LOG(LOG_ERR, "SHA1 - %d", ret);
  }
#elif _WIN32
  BCRYPT_ALG_HANDLE hAlg = NULL;
  BCRYPT_HASH_HANDLE hHash = NULL;
  DWORD cbHashObject;
  uint8_t *pbHashObject = NULL;
  NTSTATUS status;
  status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA1_ALGORITM, NULL, 0);
  if (BCRYPT_SUCCESS(status))
  {
    DWORD cbResult;
    status = BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&cbHashObject, sizeof(DWORD), &cbResult, 0);
  }
  if (BCRYPT_SUCCESS(status))
  {
    pbHashObject = new uint8_t[cbHashObject];
    status = BCryptCreateHash(hAlg, &hHash, pbHashObject, cbHashObject, NULL, 0, 0);
  }
  if (BCRYPT_SUCCESS(status))
  {
    status = BCryptHashData(hHash, (PUCHAR)ns.id, UUID_IDENTITY_SIZE, 0);
  }
  if (BCRYPT_SUCCESS(status) && ipui)
  {
    status = BCryptHashData(hHash, (PUCHAR)ipui, 5, 0);
  }
  if (BCRYPT_SUCCESS(status) && emc)
  {
    status = BCryptHashData(hHash, (PUCHAR)emc, 2, 0);
  }
  if (BCRYPT_SUCCESS(status))
  {
    status = BCryptFinishHash(hHash, digest, 20, 0);
  }
  if (!BCRYPT_SUCCESS(status))
  {
    LOG(LOG_ERR, "SHA1 - 0x%x", status);
  }
  if (hAlg)
  {
    BCryptCloseAlgorithmProvider(hAlg, 0);
  }
  if (hHash)
  {
    BCryptDestroyHash(hHash);
  }
  delete[] pbHashObject;
#else
#error Missing implementation of SHA-1 hash
#endif
  digest[7] = (digest[7] & 0x0f) | 0x50;
  digest[8] = (digest[8] & 0x3f) | 0x80;
  memcpy(id->id, digest, UUID_IDENTITY_SIZE);
}