/*
 * The start of a top-level page and the filter of the detected disc types.
 */

/* Turns the illegal disc alert back on and, unless keep_history, clears the page history. */
stock start_top_page(keep_history)
{
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
    if (!keep_history)
    {
        hisClear();
    }
}

/* Returns type if the detection mode detect (DETECT_* of topmenu.inc) accepts it, else 0. */
stock filter_disc_type(detect, type)
{
    new ret;
    if (type == CDVD_ILLEGAL)
    {
        return 0;
    }
    switch (detect)
    {
        case 0:
        {
            ret = 0;
        }
        case DETECT_GAME:
        {
            if (type <= CDVD_PS2_DVD)
            {
                ret = type;
            }
        }
        case DETECT_MUSIC:
        {
            if (type <= CDVD_UNKNOWN || type == CDVD_PS1_CDDA || type == CDVD_PS2_CDDA || type == CDVD_CDDA)
            {
                ret = type;
            }
        }
        case DETECT_MOVIE:
        {
            if (type <= CDVD_UNKNOWN || type == CDVD_DVD_VIDEO || type == CDVD_DVD_VR)
            {
                ret = type;
            }
        }
        case DETECT_PHOTO:
        {
            ret = 0;
        }
        case DETECT_ALL:
        {
            ret = type;
        }
        default:
        {
            ret = 0;
        }
    }
    return ret;
}

