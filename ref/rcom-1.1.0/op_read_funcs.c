/*
 * OP_read_funcs: the handler table of OP_CTX_init_read, without asn1_encode (slot 3) and push_pop
 * (slot 10).
 */

extern int op_finish(), op_mark(), op_label(), op_asn1_encode(), op_call(), op_asn1_get();
extern int op_asn1_check(), op_if(), op_maths(), op_load(), op_push_pop(), op_undef();

static int (*g_op_read_funcs_table[16])() = {
    op_finish, op_mark, op_label, op_undef,
    op_call, op_asn1_get, op_asn1_check, op_if,
    op_maths, op_load, op_undef, op_undef,
    op_undef, op_undef, op_undef, op_undef
};

void *OP_read_funcs(void)
{
    return (void *)g_op_read_funcs_table;
}
