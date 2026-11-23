/*
 * Waits for the XML loader and for the software keyboard.
 */

/* Waits until the XML load ends; returns 1 when it is done, -1 when it failed. */
stock wait_xml_load()
{
    new status;
    for (; ; )
    {
        status = xmlLoadSync();
        if (0 != status)
        {
            return status;
        }
        sleep;
    }
    return 0;
}

/* Waits until the software keyboard closes; returns 1 when the text is confirmed, -1 when cancelled. */
stock wait_skb()
{
    new status;
    for (; ; )
    {
        status = skbSync();
        if (0 != status)
        {
            return status;
        }
        sleep;
    }
    return 0;
}

