/*
 * Waits for a string message and for the loads of the sound natives.
 */

/* Sleeps until a message msg with a string arrives. */
stock wait_message_str(msg, text[], maxlen)
{
    while (!FindMessageStr(msg, text, maxlen))
    {
        sleep;
    }
}

/* Waits until the music loaded by soundBgmLoadStart is ready; returns the result of soundBgmLoadSync. */
stock wait_bgm_load()
{
    for (; ; )
    {
        new status = soundBgmLoadSync();
        if (0 != status)
        {
            return status;
        }
        sleep;
    }
    return 0;
}

/* Waits until the sound effects loaded by soundSeLoadStart are ready; returns the result of soundSeLoadSync. */
stock wait_se_load()
{
    for (; ; )
    {
        new status = soundSeLoadSync();
        if (0 != status)
        {
            return status;
        }
        sleep;
    }
    return 0;
}

