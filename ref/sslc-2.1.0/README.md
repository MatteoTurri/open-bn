# ref/sslc-2.1.0 — the SSL/crypto library of the binary: RSA SSL-C 2.1.0

The library in `bn` is neither SSLeay nor OpenSSL: it is RSA Security's commercial fork, named
by the banners in `.rodata`:

| module in the binary | banner |
|---|---|
| LHASH, Big Number, DSA, EVP, MD2, MD5, RC4, SHA1 (`crypto/`) | "part of **RCOM 1.1.0**", 06-Mar-2001 … 30-Jan-2002 |
| SSLv2, SSLv3, TLSv1, SSLv2/3 compat, X509 (`ssl/`, `crypto/x509`) | "part of **SSL-C 2.1.0** 28-Nov-2001" |

SSL-C 2.1.0 is RSA's SSL product and RCOM 1.1.0 its crypto component. RSA's own code
(BSAFE style: `B_*`, `KIT_*`, `ALG_*`, `CMP_*`, ASN/BER/DER, `bn_me*`) is in `ref/rcom-1.1.0/`;
this tree holds the part derived from SSLeay/OpenSSL, with RSA's changes.

## Base versions

- **`crypto/`**: SSLeay 0.9.0b (`SHA1_version` still says "SSLeay 0.9.0b"), with structures
  extended by RSA.
- **`ssl/`**: a snapshot between OpenSSL 0.9.3a and 0.9.4 (23-slot `SSL_METHOD`,
  `ssl_renegotiate_check`, the error strings of each version), with features RSA removed
  (compression, `SSL_CTX_use_certificate_chain_file`, `SSL_add_*_cert_subjects_to_stack`).

## Build

All units at `-O0 -g` with asflags `-g2` (lines in `config/units.txt`). Headers follow the
SSLeay 0.9.0b layout: one copy of each, public ones in `crypto/<module>/` and `ssl/`, with relative
symlinks in `include/` (what `make links` makes), private ones (`*_locl.h`, `cryptlib.h`, ...) next
to their sources. Units of `crypto/` and `ssl/` use `-Iref/sslc-2.1.0/crypto -Iref/sslc-2.1.0/include`
as upstream (`-I.. -I../../include`); RCOM units add `-Iref/sslc-2.1.0/include`.
Many SSLeay files are split into several translation units, as the original build compiled them;
each file's header names the functions it holds and the file they come from.

## What the fork changes

- Error codes are renumbered: the `SSL_F_*`/`SSL_R_*`, `X509_*` and `RSA_*` values in the headers
  are the fork's, from the `*_str_functs`/`*_str_reasons` tables; the library number comes from
  `ERR_get_next_error_library()` at run time.
- Allocation is `R_malloc`/`R_realloc`/`R_free`, reference counts `R_locked_add`, time an
  `R_TIME_OBJ` (`R_time_*`, also in `ASN1_UTCTIME_set`), entropy `R_rand_add_entropy`.
- EVP digests, ciphers and HMAC are allocated objects that are their own context
  (`EVP_MD_CTX_new`, `HMAC_new`, `cipher_data`); `HMAC_*` return a status. The SSL keeps its
  digest context in `md_ctx`, the temporary buffers are 32 bytes.
- Certificates go through the `SSLCERT_*` layer, which takes its arguments by value and writes
  results through an out pointer, and `SSLCERT_get_flag` guards the certificate file functions;
  `SSL_feature_test` turns the `NO_DH`/`NO_DSA` compile options into run-time checks.
- `SSL_ctrl` and `SSL_CTX_ctrl` are dispatchers with RCOM commands (certificate list limit,
  buffer sizes, `debug`, `references`); some fields are read through the options instead
  (bits 0x20000 and 0x40000).
- I/O buffers are sized at run time (`rbuf_size`/`wbuf_size`, `ssl2_alloc_read_buf`,
  `ssl3_alloc_read_buf`); a (callback, arg) pair with a fallback on the context reports alerts.
- Some stock macros are functions (`SSL_want`, `EVP_CIPHER_CTX_*` accessors), and the integer
  packers write by index and advance once.
