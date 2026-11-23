/*
 * Checks of the credit card fields and of the save devices.
 */

/* Returns 1 for 10 to 16 digits. */
stock is_valid_card_number(number[])
{
    new i;
    new c;
    if (!(strlen(number) >= 10 && strlen(number) <= 16))
    {
        return 0;
    }
    i = 0;
    for (; number{i}; i++)
    {
        c = number{i};
        if (!(c >= '0' && c <= '9'))
        {
            return 0;
        }
    }
    return 1;
}

/* Returns 1 for 3 or 4 characters. */
stock is_valid_security_code(code[])
{
    new len;
    len = strlen(code);
    if (!(len >= 3 && len <= 4))
    {
        return 0;
    }
    return 1;
}

/*
 * Returns whether a save device (bit 0x8000 aside) can be used: the hard disk (0) always, memory card 1 or 2 when
 * status, the result of edsClientPoll, has a PS2 memory card in its slot.
 */
stock is_device_ready(device, status)
{
    device = device & ~0x8000;
    if (0 == device)
    {
        return 1;
    }
    if (device == 1)
    {
        if (status >> 4 & 3 == 2)
        {
            return 1;
        }
    }
    else if (device == 2)
    {
        if (status >> 0 & 3 == 2)
        {
            return 1;
        }
    }
    return 0;
}

