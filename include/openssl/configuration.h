/*
 * WARNING: do not edit!
 * Generated by configdata.pm from Configurations\common0.tmpl, Configurations\windows-makefile.tmpl
 * via makefile.in
 *
 * Copyright 2016-2021 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#ifndef OPENSSL_CONFIGURATION_H
# define OPENSSL_CONFIGURATION_H
# pragma once

# ifdef  __cplusplus
extern "C" {
# endif

# ifdef OPENSSL_ALGORITHM_DEFINES
#  error OPENSSL_ALGORITHM_DEFINES no longer supported
# endif

/*
 * OpenSSL was configured with the following options:
 */

# ifndef OPENSSL_SYS_WIN64A
#  define OPENSSL_SYS_WIN64A 1
# endif
# define OPENSSL_CONFIGURED_API 30000
# ifndef OPENSSL_RAND_SEED_OS
#  define OPENSSL_RAND_SEED_OS
# endif
# ifndef OPENSSL_THREADS
#  define OPENSSL_THREADS
# endif
# ifndef OPENSSL_NO_ACVP_TESTS
#  define OPENSSL_NO_ACVP_TESTS
# endif
# ifndef OPENSSL_NO_AFALGENG
#  define OPENSSL_NO_AFALGENG
# endif
# ifndef OPENSSL_NO_APPS
#  define OPENSSL_NO_APPS
# endif
# ifndef OPENSSL_NO_ARIA
#  define OPENSSL_NO_ARIA
# endif
# ifndef OPENSSL_NO_ASAN
#  define OPENSSL_NO_ASAN
# endif
# ifndef OPENSSL_NO_ASYNC
#  define OPENSSL_NO_ASYNC
# endif
# ifndef OPENSSL_NO_AUTOALGINIT
#  define OPENSSL_NO_AUTOALGINIT
# endif
# ifndef OPENSSL_NO_CAPIENG
#  define OPENSSL_NO_CAPIENG
# endif
# ifndef OPENSSL_NO_CMP
#  define OPENSSL_NO_CMP
# endif
# ifndef OPENSSL_NO_CRMF
#  define OPENSSL_NO_CRMF
# endif
# ifndef OPENSSL_NO_CRYPTO_MDEBUG
#  define OPENSSL_NO_CRYPTO_MDEBUG
# endif
# ifndef OPENSSL_NO_CRYPTO_MDEBUG_BACKTRACE
#  define OPENSSL_NO_CRYPTO_MDEBUG_BACKTRACE
# endif
# ifndef OPENSSL_NO_DEVCRYPTOENG
#  define OPENSSL_NO_DEVCRYPTOENG
# endif
# ifndef OPENSSL_NO_DSO
#  define OPENSSL_NO_DSO
# endif
# ifndef OPENSSL_NO_EC_NISTP_64_GCC_128
#  define OPENSSL_NO_EC_NISTP_64_GCC_128
# endif
# ifndef OPENSSL_NO_EGD
#  define OPENSSL_NO_EGD
# endif
# ifndef OPENSSL_NO_EXTERNAL_TESTS
#  define OPENSSL_NO_EXTERNAL_TESTS
# endif
# ifndef OPENSSL_NO_FIPS_SECURITYCHECKS
#  define OPENSSL_NO_FIPS_SECURITYCHECKS
# endif
# ifndef OPENSSL_NO_FUZZ_AFL
#  define OPENSSL_NO_FUZZ_AFL
# endif
# ifndef OPENSSL_NO_FUZZ_LIBFUZZER
#  define OPENSSL_NO_FUZZ_LIBFUZZER
# endif
# ifndef OPENSSL_NO_KTLS
#  define OPENSSL_NO_KTLS
# endif
# ifndef OPENSSL_NO_LOADERENG
#  define OPENSSL_NO_LOADERENG
# endif
# ifndef OPENSSL_NO_MD2
#  define OPENSSL_NO_MD2
# endif
# ifndef OPENSSL_NO_MSAN
#  define OPENSSL_NO_MSAN
# endif
# ifndef OPENSSL_NO_RC5
#  define OPENSSL_NO_RC5
# endif
# ifndef OPENSSL_NO_SCTP
#  define OPENSSL_NO_SCTP
# endif
# ifndef OPENSSL_NO_SSL3
#  define OPENSSL_NO_SSL3
# endif
# ifndef OPENSSL_NO_SSL3_METHOD
#  define OPENSSL_NO_SSL3_METHOD
# endif
# ifndef OPENSSL_NO_TESTS
#  define OPENSSL_NO_TESTS
# endif
# ifndef OPENSSL_NO_TRACE
#  define OPENSSL_NO_TRACE
# endif
# ifndef OPENSSL_NO_UBSAN
#  define OPENSSL_NO_UBSAN
# endif
# ifndef OPENSSL_NO_UI_CONSOLE
#  define OPENSSL_NO_UI_CONSOLE
# endif
# ifndef OPENSSL_NO_UNIT_TEST
#  define OPENSSL_NO_UNIT_TEST
# endif
# ifndef OPENSSL_NO_UPLINK
#  define OPENSSL_NO_UPLINK
# endif
# ifndef OPENSSL_NO_WEAK_SSL_CIPHERS
#  define OPENSSL_NO_WEAK_SSL_CIPHERS
# endif
# ifndef OPENSSL_NO_WHIRLPOOL
#  define OPENSSL_NO_WHIRLPOOL
# endif
# ifndef OPENSSL_NO_DYNAMIC_ENGINE
#  define OPENSSL_NO_DYNAMIC_ENGINE
# endif


/* Generate 80386 code? */
# undef I386_ONLY

/*
 * The following are cipher-specific, but are part of the public API.
 */
# if !defined(OPENSSL_SYS_UEFI)
#  undef BN_LLONG
/* Only one for the following should be defined */
#  undef SIXTY_FOUR_BIT_LONG
#  define SIXTY_FOUR_BIT
#  undef THIRTY_TWO_BIT
# endif

# define RC4_INT unsigned int

# ifdef  __cplusplus
}
# endif

#endif                          /* OPENSSL_CONFIGURATION_H */
