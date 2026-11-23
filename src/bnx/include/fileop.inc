/*
 * File operations with progress, the photo database, text lines, the background camera and card periods.
 */

/*
 * Copies the line of text that starts at pos into dest; returns the position of the next line, -1 at the end of
 * text.
 */
stock get_line(dest[], text[], pos, maxlen)
{
    if (0 == text{pos})
    {
        return -1;
    }
    new eol = find_any_char(text, "\n", pos);
    copy_substring(dest, 0, text, pos, eol < 0 ? eol : eol - pos, maxlen);
    return eol < 0 ? strlen(text) : eol + 1;
}

/* Returns the horizontal offset a page slides in from: none with bit 1, else 160, to the left with bit 0. */
stock Float:get_slide_offset(dir)
{
    new Float:offset = dir & 2 ? 0.0 : 160.0;
    if (dir & 1)
    {
        offset = -offset;
    }
    return offset;
}

/*
 * Waits for the file operation in progress, showing it from from to to percent on progress_win; returns 0, or -1 on
 * error.
 */
stock wait_file_op(&status, &error, progress_win, from, to)
{
    new percent;
    for (; ; )
    {
        status = fileSync(percent, error);
        if (progress_win)
        {
            SetPubVar(progress_win, "progress", from + percent * (to - from) / 100);
        }
        if (status < 0)
        {
            return -1;
        }
        if (status == FILESYNC_DONE)
        {
            if (0 != error)
            {
                return -1;
            }
            break;
        }
        sleep;
    }
    return 0;
}

#if !defined g_photo_db_name
new g_photo_db_name[] = "photo";
#endif
/*
 * Opens the photo album database (db_mode ALBUM_LIBRARY or ALBUM_FAVORITE of photodb.inc, 0 or 1) or a file list
 * (ALBUM_USB, 3) and waits for it; returns 0 or -1.
 */
stock open_photo_db(db_mode)
{
    new ret;
    if (0 == db_mode || db_mode == 1)
    {
        ret = datOpen(DAT_DATABASE, g_photo_db_name, DAT_ORDER_MTIME_DESC);
    }
    else if (db_mode == 3)
    {
        ret = datOpen(DAT_FILE_LIST, "", DAT_ORDER_NAME_ASC);
    }
    if (0 != ret)
    {
        return -1;
    }
    ret = wait_database();
    if (ret < 0)
    {
        return -1;
    }
    printf("album db open %d -----------\n", db_mode);
    return 0;
}

/* Moves the camera of the background scene back to its rest position. */
stock reset_bg_camera()
{
    new scene = wndGetScene(WND_BG);
    if (scene)
    {
        SendMessage(scene, TAG("CRCT"), Camera(0), 0, 0, 0);
    }
}

/*
 * From a date YYYYMM, makes the default card period (January of the next year) and the range of years, from that
 * year to ten years later.
 */
stock get_card_period_range(date_text[], period[], min_year[], max_year[])
{
    new year_num;
    new month_num;
    new year_text[8];
    new month_text[8];
    split_year_month(date_text, year_text, month_text, 8, 8);
    month_text = "01";
    year_num = atoi(year_text);
    month_num = atoi(month_text);
    sprintf(period, "%04d%02d", year_num + 1, month_num);
    sprintf(min_year, "%04d", year_num);
    sprintf(max_year, "%04d", year_num + 10);
}

