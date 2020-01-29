#pragma once

#include "PGPSignature.h"

/* Command copied from https://gitlab.com/tortoisegit/tortoisegit/blob/e1262ead5495ecc7902b61ac3e1f3da22294bb2d/src/version.h */
/* gpg --armor --export 587A279C | sed -e s/^/\"/ -e s/\$/\\\\n\"/ */


static const uint8_t endless_public_key_longid[8] = {
    0x9E, 0x0C, 0x12, 0x50, 0x58, 0x7A, 0x27, 0x9C
};

// Endless public key
static const uint8_t endless_public_key[] = {
    "-----BEGIN PGP PUBLIC KEY BLOCK-----\n"
    "\n"
    "mQINBFRZN4wBEADRr7nE5VFbWMryCUwSc41grfFMfnEkxgpKeRBz75Be2u2sMwP3\n"
    "byqGW+Ak252ZSL+9fENRLqACTeY6oOLLhPv3SOhCcoAjh7bY651VGqIJcIM2CqNo\n"
    "m5Iqq7rp2UDEYOoJiDno+yW69A6ZN6rNTLVgpgkKD8hwI5E8fct0svqKU7Rh1ndy\n"
    "hEVHuuFgjOHQ6gQEykYSYe2LNEDJcMgxWO/MlFq1z3qo3b6oeq3gAjQFXvGHXk7P\n"
    "8Q1Fkx7qcaGG58+CD5J3/eYnv5okTKdtRrBTHmr1P1WaTnmSBpSw38Hqv0vJFqlR\n"
    "UzE57SsSZFSRnjALNIkEjjNF3n7Y7Mm6Y4S0Wg6cpGlRB4ME3HbCzP2NdG6Qrc9I\n"
    "31dANkJ5jeIgHkthNOXLiTvz7F+7UCwL/oBW0Lp5IQBr5Mi1rV7dOLug0iPt/kcm\n"
    "92WTzV32zVoBo3xO7t+Bb+23bX20I9lDEm4erWO2YfpeAs8ex+UzSHVZfDm+atxn\n"
    "lFEwQikNos6X009+tjvfy9ZSjRE4J1PnXqpRbwZ7up8N1FJYtQN6ePjyJ6yJUVfz\n"
    "9McRBgZPcb2QP4zYClCLiR4oK1MxdMdGsmGTOE1RDTJ5KXwBmCwFlZQcLRNDocIu\n"
    "f2du4Hzz8sbiWKNfQa2p7/Q0cMKigeMVBKXpvO5aD/1P5xpjQyLKZpkOJQARAQAB\n"
    "tDhFbmRsZXNzIEltYWdlIFNpZ25lciAxIChFSVMxKSA8bWFpbnRhaW5lcnNAZW5k\n"
    "bGVzc20uY29tPokCVQQTAQIAPwIbAwYLCQgHAwIGFQgCCQoLBBYCAwECHgECF4AW\n"
    "IQTLUA97ySM/rTK05yCeDBJQWHonnAUCXby9LAUJHC+IoAAKCRCeDBJQWHonnKeA\n"
    "D/44uzwpM/4IFfGUKU6M7smd6OAa/TVuHYmYE519bUIOIu2a/7JkMIz2QNUQrBHZ\n"
    "GVxt0IrsQyyKNd098sBWEYYVtJYyTDCLucwLMK57qsdFD3wpmqIYahHVh0JE7Uif\n"
    "vKA68OwMIMZTkRmYFrn9so1dONN2WTcOVvQ3PtMc/G1jfFFPrGkw6O4qyD0a1rAi\n"
    "IrhHtfHoewzsCR3SLTve0n6gw0IdY9ik4XnhEfqJVd4ZycQj/RYeHk3qUK63GdUr\n"
    "gTVAhcWRhEp6QGXsANaE03vdjyzLzZF7tALa6rGBCrhCRkvXIMB2jk7r3RExTcFP\n"
    "gQSg+N8ZRX7zV6FJ3YCIMx/H02YSjJRsSWYQExMpaPeyThSrvV4l1XCy0gSpXTsM\n"
    "QDSJ/XJzycC6J82aHxqjqw0Q9EX7sygWjiiSmARjlN2pHGQ3hOQhbeuWukCCpQhS\n"
    "dp81bBUY1yeoxOuVWIxFBcStP/2PHL2wS0N+Ro/xxE3UWa3LWF1Q/8Tvu83G4cxf\n"
    "tzcoc8ykorRRf/t9qIGAfw+KqCFpZ7S/PP81paGfjsoKmm1XK6Ku4LEy1573ilos\n"
    "OFydYp+weAxFWbrsvOs1nWhKiPujybf2/woepsYJk4DsMNUAVVzzKOWT5lfugEo7\n"
    "TJqEIKx/c89XZ5M947KZPDL9rB0EQVNpjlV1Y1W4AJlFBw==\n"
    "=dJDN\n"
    "-----END PGP PUBLIC KEY BLOCK-----\n"
};
