/*
 * Waits for the image decoder and the image trimmer.
 */

/* Waits until the image decoder is done; returns IMG_DONE or IMG_FAILED. */
stock wait_image_decode()
{
    for (; ; )
    {
        new status = imgDecSync();
        if (IMG_BUSY != status)
        {
            return status;
        }
        sleep;
    }
    return 0;
}

/* Returns IMG_DONE when the trim is done, else IMG_FAILED: imgTrimSync never answers IMG_BUSY. */
stock wait_image_trim()
{
    for (; ; )
    {
        new status = imgTrimSync();
        if (IMG_BUSY != status)
        {
            return status;
        }
        sleep;
    }
    return 0;
}

