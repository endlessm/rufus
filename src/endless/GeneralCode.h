#pragma once

#include <stdint.h>

// helper methods
#define TOSTR(value) case value: return _T(#value)

#define PRINT_ERROR_MSG(__ERROR_MSG__) uprintf("%s:%d %s (GLE=[%d])",__FILE__, __LINE__, __ERROR_MSG__, GetLastError())

#define IFFALSE_PRINTERROR(__CONDITION__, __ERRROR_MSG__) if(!(__CONDITION__)) { if(strlen(__ERRROR_MSG__)) PRINT_ERROR_MSG(__ERRROR_MSG__); }
#define IFFALSE_GOTOERROR(__CONDITION__, __ERRROR_MSG__) if(!(__CONDITION__)) { if(strlen(__ERRROR_MSG__)) PRINT_ERROR_MSG(__ERRROR_MSG__); goto error; }
#define IFFALSE_GOTO(__CONDITION__, __ERRROR_MSG__, __LABEL__) if(!(__CONDITION__)) { PRINT_ERROR_MSG(__ERRROR_MSG__); goto __LABEL__; }
#define IFFALSE_RETURN(__CONDITION__, __ERRROR_MSG__) if(!(__CONDITION__)) { PRINT_ERROR_MSG(__ERRROR_MSG__); return; }
#define IFFALSE_RETURN_VALUE(__CONDITION__, __ERRROR_MSG__, __RET__) if(!(__CONDITION__)) { PRINT_ERROR_MSG(__ERRROR_MSG__); return __RET__; }
#define IFFALSE_BREAK(__CONDITION__, __ERRROR_MSG__) if(!(__CONDITION__)) { PRINT_ERROR_MSG(__ERRROR_MSG__); break; }
#define IFFALSE_CONTINUE(__CONDITION__, __ERRROR_MSG__) if(!(__CONDITION__)) { PRINT_ERROR_MSG(__ERRROR_MSG__); continue; }

#define IFTRUE_GOTO(__CONDITION__, __ERRROR_MSG__, __LABEL__) if((__CONDITION__)) { PRINT_ERROR_MSG(__ERRROR_MSG__); goto __LABEL__; }

#define safe_closefile(__file__) if (__file__ != NULL) { fclose(__file__); __file__ = NULL; }

#define FUNCTION_ENTER uprintf("%s:%d %s", __FILE__, __LINE__, __FUNCTION__)

/* Command copied from https://gitlab.com/tortoisegit/tortoisegit/blob/e1262ead5495ecc7902b61ac3e1f3da22294bb2d/src/version.h */
/* gpg --armor --export 587A279C | sed -e s/^/\"/ -e s/\$/\\\\n\"/ */


static const uint8_t endless_public_key_longid[8] = {
    0x9E, 0x0C, 0x12, 0x50, 0x58, 0x7A, 0x27, 0x9C
};

// Endless public key
static const uint8_t endless_public_key[] = {
    "-----BEGIN PGP PUBLIC KEY BLOCK-----\n"
    "Version: GnuPG v1\n"
    "\n"
    "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\n"
    "=DASp\n"
    "-----END PGP PUBLIC KEY BLOCK-----\n"
};

// methods for adding our entry in BIOS EFI
bool EFIRequireNeededPrivileges();
bool EFICreateNewEntry(const wchar_t *drive, wchar_t *path, wchar_t *desc);


// TODO: remove these
// Hardcoded JSON files for internal testing release

static const char *eosinstaller_hardcoded_json = "{" \
"    \"images\": {" \
"        \"eos3.0 3.0.2 eosinstaller-amd64-amd64\": {" \
"            \"arch\": \"amd64\", " \
"            \"branch\": \"eos3.0\", " \
"            \"personalities\": [" \
"                \"base\"" \
"            ], " \
"            \"personality_images\": {" \
"                \"base\": {" \
"                    \"full\": {" \
"                        \"compressed_size\": 741724523, " \
"                        \"compression_type\": \"gz\", " \
"                        \"extracted_signature\": \"release/3.0.2/eosinstaller-amd64-amd64/base/eosinstaller-eos3.0-amd64-amd64.160828-102101.base.img.asc\", " \
"                        \"extracted_size\": 3041787904, " \
"                        \"file\": \"release/3.0.2/eosinstaller-amd64-amd64/base/eosinstaller-eos3.0-amd64-amd64.160828-102101.base.img.gz\", " \
"                        \"signature\": \"release/3.0.2/eosinstaller-amd64-amd64/base/eosinstaller-eos3.0-amd64-amd64.160828-102101.base.img.gz.asc\", " \
"                        \"torrents\": {}" \
"                    }" \
"                }" \
"            }, " \
"            \"platform\": \"amd64\", " \
"            \"product\": \"eosinstaller\", " \
"            \"release\": true, " \
"            \"version\": \"3.0.2\"" \
"        }" \
"    }, " \
"    \"version\": 1" \
"}";

static const char *eos_hardcoded_json = "{" \
"    \"images\": {" \
"        \"eos3.0 3.0.2 eos-amd64-amd64\": {" \
"            \"arch\": \"amd64\", " \
"            \"branch\": \"eos3.0\", " \
"            \"personalities\": [" \
"                \"bn\", " \
"                \"es_GT\", " \
"                \"fr\", " \
"                \"base\", " \
"                \"zh_CN\", " \
"                \"ar\", " \
"                \"pt_BR\", " \
"                \"en\", " \
"                \"es\"" \
"            ], " \
"            \"personality_images\": {" \
"                \"ar\": {" \
"                    \"full\": {" \
"                        \"compressed_size\": 7756147886, " \
"                        \"compression_type\": \"gz\", " \
"                        \"extracted_signature\": \"release/3.0.2/eos-amd64-amd64/ar/eos-eos3.0-amd64-amd64.160827-170148.ar.img.asc\", " \
"                        \"extracted_size\": 13577527296, " \
"                        \"file\": \"release/3.0.2/eos-amd64-amd64/ar/eos-eos3.0-amd64-amd64.160827-170148.ar.img.gz\", " \
"                        \"signature\": \"release/3.0.2/eos-amd64-amd64/ar/eos-eos3.0-amd64-amd64.160827-170148.ar.img.gz.asc\", " \
"                        \"torrents\": {}" \
"                    }" \
"                }, " \
"                \"base\": {" \
"                    \"full\": {" \
"                        \"compressed_size\": 2059633940, " \
"                        \"compression_type\": \"gz\", " \
"                        \"extracted_signature\": \"release/3.0.2/eos-amd64-amd64/base/eos-eos3.0-amd64-amd64.160827-104530.base.img.asc\", " \
"                        \"extracted_size\": 6061092864, " \
"                        \"file\": \"release/3.0.2/eos-amd64-amd64/base/eos-eos3.0-amd64-amd64.160827-104530.base.img.gz\", " \
"                        \"signature\": \"release/3.0.2/eos-amd64-amd64/base/eos-eos3.0-amd64-amd64.160827-104530.base.img.gz.asc\", " \
"                        \"torrents\": {}" \
"                    }" \
"                }, " \
"                \"bn\": {" \
"                    \"full\": {" \
"                        \"compressed_size\": 5533438034, " \
"                        \"compression_type\": \"gz\", " \
"                        \"extracted_signature\": \"release/3.0.2/eos-amd64-amd64/bn/eos-eos3.0-amd64-amd64.160827-161313.bn.img.asc\", " \
"                        \"extracted_size\": 11075596288, " \
"                        \"file\": \"release/3.0.2/eos-amd64-amd64/bn/eos-eos3.0-amd64-amd64.160827-161313.bn.img.gz\", " \
"                        \"signature\": \"release/3.0.2/eos-amd64-amd64/bn/eos-eos3.0-amd64-amd64.160827-161313.bn.img.gz.asc\", " \
"                        \"torrents\": {}" \
"                    }" \
"                }, " \
"                \"en\": {" \
"                    \"full\": {" \
"                        \"compressed_size\": 13633583801, " \
"                        \"compression_type\": \"gz\", " \
"                        \"extracted_signature\": \"release/3.0.2/eos-amd64-amd64/en/eos-eos3.0-amd64-amd64.160827-110311.en.img.asc\", " \
"                        \"extracted_size\": 20050870272, " \
"                        \"file\": \"release/3.0.2/eos-amd64-amd64/en/eos-eos3.0-amd64-amd64.160827-110311.en.img.gz\", " \
"                        \"signature\": \"release/3.0.2/eos-amd64-amd64/en/eos-eos3.0-amd64-amd64.160827-110311.en.img.gz.asc\", " \
"                        \"torrents\": {}" \
"                    }" \
"                }, " \
"                \"es\": {" \
"                    \"full\": {" \
"                        \"compressed_size\": 14728228655, " \
"                        \"compression_type\": \"gz\", " \
"                        \"extracted_signature\": \"release/3.0.2/eos-amd64-amd64/es/eos-eos3.0-amd64-amd64.160827-120936.es.img.asc\", " \
"                        \"extracted_size\": 21275070464, " \
"                        \"file\": \"release/3.0.2/eos-amd64-amd64/es/eos-eos3.0-amd64-amd64.160827-120936.es.img.gz\", " \
"                        \"signature\": \"release/3.0.2/eos-amd64-amd64/es/eos-eos3.0-amd64-amd64.160827-120936.es.img.gz.asc\", " \
"                        \"torrents\": {}" \
"                    }" \
"                }, " \
"                \"es_GT\": {" \
"                    \"full\": {" \
"                        \"compressed_size\": 15115224759, " \
"                        \"compression_type\": \"gz\", " \
"                        \"extracted_signature\": \"release/3.0.2/eos-amd64-amd64/es_GT/eos-eos3.0-amd64-amd64.160827-131911.es_GT.img.asc\", " \
"                        \"extracted_size\": 21768589312, " \
"                        \"file\": \"release/3.0.2/eos-amd64-amd64/es_GT/eos-eos3.0-amd64-amd64.160827-131911.es_GT.img.gz\", " \
"                        \"signature\": \"release/3.0.2/eos-amd64-amd64/es_GT/eos-eos3.0-amd64-amd64.160827-131911.es_GT.img.gz.asc\", " \
"                        \"torrents\": {}" \
"                    }" \
"                }, " \
"                \"fr\": {" \
"                    \"full\": {" \
"                        \"compressed_size\": 8598989467, " \
"                        \"compression_type\": \"gz\", " \
"                        \"extracted_signature\": \"release/3.0.2/eos-amd64-amd64/fr/eos-eos3.0-amd64-amd64.160827-175245.fr.img.asc\", " \
"                        \"extracted_size\": 14397366272, " \
"                        \"file\": \"release/3.0.2/eos-amd64-amd64/fr/eos-eos3.0-amd64-amd64.160827-175245.fr.img.gz\", " \
"                        \"signature\": \"release/3.0.2/eos-amd64-amd64/fr/eos-eos3.0-amd64-amd64.160827-175245.fr.img.gz.asc\", " \
"                        \"torrents\": {}" \
"                    }" \
"                }, " \
"                \"pt_BR\": {" \
"                    \"full\": {" \
"                        \"compressed_size\": 9677430107, " \
"                        \"compression_type\": \"gz\", " \
"                        \"extracted_signature\": \"release/3.0.2/eos-amd64-amd64/pt_BR/eos-eos3.0-amd64-amd64.160827-142834.pt_BR.img.asc\", " \
"                        \"extracted_size\": 15789797376, " \
"                        \"file\": \"release/3.0.2/eos-amd64-amd64/pt_BR/eos-eos3.0-amd64-amd64.160827-142834.pt_BR.img.gz\", " \
"                        \"signature\": \"release/3.0.2/eos-amd64-amd64/pt_BR/eos-eos3.0-amd64-amd64.160827-142834.pt_BR.img.gz.asc\", " \
"                        \"torrents\": {}" \
"                    }" \
"                }, " \
"                \"zh_CN\": {" \
"                    \"full\": {" \
"                        \"compressed_size\": 5572034139, " \
"                        \"compression_type\": \"gz\", " \
"                        \"extracted_signature\": \"release/3.0.2/eos-amd64-amd64/zh_CN/eos-eos3.0-amd64-amd64.160827-152635.zh_CN.img.asc\", " \
"                        \"extracted_size\": 11195711488, " \
"                        \"file\": \"release/3.0.2/eos-amd64-amd64/zh_CN/eos-eos3.0-amd64-amd64.160827-152635.zh_CN.img.gz\", " \
"                        \"signature\": \"release/3.0.2/eos-amd64-amd64/zh_CN/eos-eos3.0-amd64-amd64.160827-152635.zh_CN.img.gz.asc\", " \
"                        \"torrents\": {}" \
"                    }" \
"                }" \
"            }, " \
"            \"platform\": \"amd64\", " \
"            \"product\": \"eos\", " \
"            \"release\": true, " \
"            \"version\": \"3.0.2\"" \
"        }" \
"    }, " \
"    \"version\": 1" \
"}";