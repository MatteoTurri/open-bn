/*
 * OP_rsa_pkcs1_public_key_decode: the OP_CTX program that decodes a PKCS #1 RSA public key.
 */

static unsigned int g_op_rsa_pub_dec_table[] = {
    0x00001060, 0x00010018, 0x00000260, 0x10104240,
    0x0001001a, 0x00000260, 0x11104240, 0x00000001,
    0x00000000, 0x00000000, 0x00000000, 0x00000000
};

void *OP_rsa_pkcs1_public_key_decode(void)
{
    return (void *)g_op_rsa_pub_dec_table;
}
