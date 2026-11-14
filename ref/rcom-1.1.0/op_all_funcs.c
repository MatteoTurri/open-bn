/*
 * OP_all_funcs: the 16 handlers of the OP_CTX interpreter, indexed by the high nibble of an
 * instruction (ctx->funcs).
 */

extern int op_finish(), op_mark(), op_label(), op_asn1_encode(), op_call(), op_asn1_get();
extern int op_asn1_check(), op_if(), op_maths(), op_load(), op_push_pop(), op_undef();

static int (*g_op_all_funcs_table[16])() = {
    op_finish, op_mark, op_label, op_asn1_encode,
    op_call, op_asn1_get, op_asn1_check, op_if,
    op_maths, op_load, op_push_pop, op_undef,
    op_undef, op_undef, op_undef, op_undef
};

void *OP_all_funcs(void)
{
    return (void *)g_op_all_funcs_table;
}
