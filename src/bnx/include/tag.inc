/*
 * TAG, which turns a string into a FourCC, and the packing of color bytes.
 */

/* Returns the first four characters of a packed string as a FourCC cell. */
stock TAG(fourcc[])
{
    return swapchars(fourcc[0]);
}

/* Packs 16 byte values into 4 cells, the first byte of each four in the lowest byte. */
stock pack_color_bytes(packed[], bytes[])
{
    for (new i; i < 4; i++)
    {
        for (new j; j < 4; j++)
        {
            packed{i * 4 + 3 - j} = bytes[i * 4 + j];
        }
    }
}

