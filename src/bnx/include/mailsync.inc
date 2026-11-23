/*
 * Mailer requests that wait for the folder thread (mx_SyncFolder) and return its result.
 */

/* Deletes a mail; returns 0, ERR_NOSPACE, or 1 on another error or when the request could not start. */
stock remove_message(folder, index)
{
    new ret;
    SystemCommand(SYSCMD_BUSY, 1);
    if (!mx_RemoveMsgStart(folder, index))
    {
        SystemCommand(SYSCMD_BUSY, 0);
        return 1;
    }
    sleep;
    for (; ; )
    {
        if (mx_SyncFolder(ret))
        {
            break;
        }
        sleep;
    }
    SystemCommand(SYSCMD_BUSY, 0);
    return ret;
}

/* Sorts folder; index, a mail before, becomes its index after. Returns 0, ERR_NOSPACE or 1, as remove_message. */
stock sort_messages(folder, key, order, &index)
{
    new ret;
    SystemCommand(SYSCMD_BUSY, 1);
    if (!mx_SortMsgStart(folder, key, order, index))
    {
        SystemCommand(SYSCMD_BUSY, 0);
        return 1;
    }
    sleep;
    for (; ; )
    {
        if (mx_SyncFolder(ret))
        {
            break;
        }
        sleep;
    }
    SystemCommand(SYSCMD_BUSY, 0);
    return ret;
}

/* Reads the list of folder again; returns the result of the folder thread. */
stock update_message_list(folder)
{
    new ret;
    SystemCommand(SYSCMD_BUSY, 1);
    mx_UpdateListStart(folder);
    sleep;
    for (; ; )
    {
        if (mx_SyncFolder(ret))
        {
            break;
        }
        sleep;
    }
    SystemCommand(SYSCMD_BUSY, 0);
    return ret;
}

/*
 * Saves a mail to folder (mode: the WRITE_* mode; src_folder, src_index: the mail replied to or forwarded); returns
 * the index of the new mail, -1 if it was not saved. A failed save sets error to its errno (ERR_NOSPACE).
 */
stock save_message(folder, index, to[], cc[], bcc[], subject[], body[], mode, src_folder, src_index, &error)
{
    new new_index;
    SystemCommand(SYSCMD_BUSY, 1);
    if (!mx_SetMsgStart(folder, index, to, cc, bcc, subject, body, mode, src_folder, src_index, error))
    {
        SystemCommand(SYSCMD_BUSY, 0);
        return -1;
    }
    sleep;
    for (; ; )
    {
        if (mx_SyncFolder(new_index))
        {
            break;
        }
        sleep;
    }
    SystemCommand(SYSCMD_BUSY, 0);
    return new_index;
}

/* Reads the body of a mail; returns 0, 1 if it was cut to maxlen bytes, -1 if it could not be read. */
stock read_message_body(folder, index, body[], maxlen)
{
    new ret;
    if (!mx_GetMsgBodyStart(folder, index, body, maxlen))
    {
        return -1;
    }
    sleep;
    for (; ; )
    {
        if (mx_SyncFolder(ret))
        {
            break;
        }
        sleep;
    }
    return ret;
}

/*
 * The same with each line prefixed by quote_mark, between head and tail (without a quote_mark: head, the body, a
 * newline, tail); folder -1 takes the text of mx_SetMsgBody.
 */
stock read_quoted_body(folder, index, body[], maxlen, quote_mark[], head[], tail[])
{
    new ret;
    if (!mx_GetMsgBodyStart(folder, index, body, maxlen, quote_mark, head, tail))
    {
        return -1;
    }
    sleep;
    for (; ; )
    {
        if (mx_SyncFolder(ret))
        {
            break;
        }
        sleep;
    }
    return ret;
}

