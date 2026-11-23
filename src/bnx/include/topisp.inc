/*
 * The ISP list of the top library.
 */

stock get_isp_count()
{
    return topIspGetCount();
}

/* Reads field (ISP_*) of the current ISP entry; returns 0 or -1. */
stock get_isp_value(index, field, value[], maxlen)
{
    index = index - 1; /* redundant as written */
    new key[32];
    switch (field)
    {
        case ISP_VER:
        {
            key = "VER";
        }
        case ISP_CATEGORY:
        {
            key = "CATEGORY";
        }
        case ISP_ID:
        {
            key = "ID";
        }
        case ISP_PRIORITY:
        {
            key = "PRIORITY";
        }
        case ISP_NAME:
        {
            key = "NAME";
        }
        case ISP_APPLICATION:
        {
            key = "APPLICATION";
        }
        case ISP_TYPE:
        {
            key = "TYPE";
        }
        case ISP_URL:
        {
            key = "URL";
        }
        case ISP_OPTION:
        {
            key = "OPTION";
        }
        case ISP_CONTENTS:
        {
            key = "CONTENTS";
        }
        default:
        {
            return -1;
        }
    }
    new ret = topIspGetValue(key, value, maxlen);
    printf("isp get data %s, val %s ret%d\n", key, value, ret);
    if (ret > 0)
    {
        return 0;
    }
    return -1;
}

