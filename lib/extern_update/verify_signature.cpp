#ifdef MAIN_APP_LOG_LEVEL 
#define CORE_DEBUG_LEVEL MAIN_APP_LOG_LEVEL
#else
#define CORE_DEBUG_LEVEL ARDUHAL_LOG_LEVEL_INFO
#endif

#include "verify_signature.h"
#include "Arduino.h"
#include "mbedtls/md.h"
#include "mbedtls/rsa.h"
#include "mbedtls/pk.h"
#include "mbedtls/error.h"

static const char* TAG = "VSIG";

// Clave pública en formato PEM (cadena CR LF incluida)
// Copia aquí el contenido de tu public.pem como string C RAW
const char publicKeyPem[] = R"EOF(
-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEApRgFPcmrR6hhCG3PXp79
0A6eKBY9mzARDTxjKVgxGq4rZ7ALXuKif9iYZ2ZxnLk/f8sEcdtrvBzSC6Grakjj
M8XI0FJZhhJ7Ndv8vWbNX4c8AnJOnxhrWbJP2Z7oNKgbBt06N0ZCNbYRMWcbe75g
SGGmUhu9FRfUqVTfClIrj4wjQR6ZM/oYenk62YVeQjYZix8sK1zS8TpkpSLbwtaA
ucWAisPrE/DlvekfPnZWSraoMxTdGEgHl5aJ40eN9YoUnz1q8rWzV50woC1vlWFf
a88OdbIkSVS+Pfj79dhpYgqUy8bU9dlGSG2pxIBBql/w0xjFIKWrN9+E1HPB2+KS
eQIDAQAB
-----END PUBLIC KEY-----
)EOF";

bool verifySignature(const uint8_t* firmwareData, size_t firmwareLen,
  const uint8_t* sigData, size_t sigLen) {
  int ret;
  mbedtls_pk_context pk;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
  mbedtls_md_context_t md_ctx;
  unsigned char hash[32]; // SHA256 size

  mbedtls_pk_init(&pk);
  mbedtls_md_init(&md_ctx);

  ret = mbedtls_pk_parse_public_key(&pk, (const unsigned char*)publicKeyPem, sizeof(publicKeyPem));
  if (ret != 0) {
    ESP_LOGE(TAG, "Error parsing public key: %d", ret);
    goto cleanup;
  }

  ret = mbedtls_md(mbedtls_md_info_from_type(md_type), firmwareData, firmwareLen, hash);
  if (ret != 0) {
    ESP_LOGE(TAG, "Error hashing firmware: %d", ret);
    goto cleanup;
  }

  ret = mbedtls_pk_verify(&pk, md_type, hash, sizeof(hash), sigData, sigLen);
  if (ret != 0) {
    ESP_LOGE(TAG, "Signature verification failed: %d", ret);
  }

cleanup:
  mbedtls_md_free(&md_ctx);
  mbedtls_pk_free(&pk);
  return (ret == 0);
}