/*
 * DNAS1 seed normalization: reduce each 4-bit nibble of id modulo 8; the top
 * nibble (the round count) defaults to 5 when its low three bits are zero.
 */
int _sce_dnas_pwrap_clean_id(int id)
{
	int t = id >> 28;
	int n = t & 0xf;
	int acc = 0;
	int hi;
	int i;

	if ((t & 7) != 0)
		hi = n % 8;
	else
		hi = 5;
	for (i = 0; i < 7; i++)
		acc |= (((id >> (i * 4)) & 0xf) % 8) << (i * 4);
	return (acc & 0x0FFFFFFF) | (hi << 28);
}
