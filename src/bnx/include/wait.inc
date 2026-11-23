/*
 * Waits measured in frames, and the absolute value.
 */

/* Returns the frames in ms milliseconds, at the frame rate. */
stock ms_to_frames(ms)
{
    return ms * FrameRate() / 1000;
}

stock abs(value)
{
    return value > 0 ? value : -value;
}

stock wait_ms(ms)
{
    new frames = ms * FrameRate() / 1000;
    for (new i; i < frames; i++)
    {
        sleep;
    }
}

/* Sleeps until the script is destroyed. */
stock wait_forever()
{
    for (; ; )
    {
        sleep;
    }
}

