/*
 * The first line and the cursor row of the seven-line lists of the mailer.
 */

/*
 * Gets the first line and the cursor row that show message index of folder; with keep_row the cursor stays at
 * old_row.
 */
stock get_message_line(&first, &row, folder, index, keep_row, old_row)
{
    if (index < 0)
    {
        first = row = 0;
        return;
    }
    new count;
    mx_CountMsg(folder, count);
    if (index > count)
    {
        first = row = 0;
        return;
    }
    if (LIST_LINES > index)
    {
        first = 0;
        row = index;
    }
    else if (count - LIST_LINES < index)
    {
        first = count - LIST_LINES;
        row = index - (count - 1 - 6);
    }
    else if (0 == keep_row)
    {
        first = index;
        row = 0;
    }
    else
    {
        first = index - old_row;
        row = old_row;
    }
}

/* Gets the first line and the cursor row that show address index. */
stock get_address_line(&first, &row, index)
{
    if (index < 0)
    {
        first = row = 0;
        return;
    }
    new count;
    count = mx_CountAddr();
    if (index > count)
    {
        first = row = 0;
        return;
    }
    if (LIST_LINES > index)
    {
        first = 0;
        row = index;
    }
    else if (count - LIST_LINES < index)
    {
        first = count - LIST_LINES;
        row = index - (count - 1 - 6);
    }
    else
    {
        first = index;
        row = 0;
    }
}

