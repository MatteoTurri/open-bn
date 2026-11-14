/*
 * OP_rsa_pkcs1_public_key_encode: the OP_CTX program that encodes a PKCS #1 RSA public key.
 */

static unsigned int g_op_rsa_pub_enc_table[] = {
    0x00100032, 0x00000011, 0x00101031, 0x00111031,
    0x00000001
};

void *OP_rsa_pkcs1_public_key_encode(void)
{
    return (void *)g_op_rsa_pub_enc_table;
}
