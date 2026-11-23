/*
 * A field of a photo, read from the album database or from a server object.
 */

/*
 * Reads field of photo index from the album database (album_kind picks the column; with another kind, -1, field is
 * the column of the query) or, with server, by "DGET" messages; the date follows the settings. Returns -1 on an error.
 */
stock get_photo_field(index, field, text[], album_kind, server, maxlen)
{
    new ret;
    new column;
    if (ALBUM_LIBRARY == album_kind || album_kind == ALBUM_FAVORITE)
    {
        switch (field)
        {
            case FIELD_URL:
            {
                column = COL_CONTENT_URI;
            }
            case FIELD_ALBUM_NAME:
            {
                column = COL_ALBUM_NAME;
            }
            case FIELD_ALBUM_URI:
            {
                column = COL_ALBUM_URI;
            }
            case FIELD_DATE:
            {
                column = COL_CREATE_DATE;
            }
            case FIELD_OID:
            {
                column = COL_OID;
            }
            default:
            {
                return -1;
            }
        }
    }
    else if (album_kind == PHOTOS_LIBRARY || album_kind == PHOTOS_FAVORITE)
    {
        switch (field)
        {
            case FIELD_URL:
            {
                column = COL_CONTENT_URI;
            }
            case FIELD_ALBUM_NAME:
            {
                column = COL_ALBUM_NAME;
            }
            case FIELD_TITLE:
            {
                column = COL_TITLE;
            }
            case FIELD_DATE:
            {
                column = COL_CREATE_DATE;
            }
            case FIELD_OID:
            {
                column = COL_OID;
            }
            default:
            {
                return -1;
            }
        }
    }
    else if (album_kind == ALBUM_USB || album_kind == PHOTOS_USB)
    {
        switch (field)
        {
            case FIELD_URL:
            {
                column = USB_COL_PATH;
            }
            case FIELD_ALBUM_NAME:
            {
                get_tag_attr("TEXT", "msgphoto_menu_camera", "value", text, maxlen);
                return 0;
            }
            case FIELD_TITLE:
            {
                column = USB_COL_NAME;
            }
            default:
            {
                return -1;
            }
        }
    }
    else
    {
        column = field;
    }
    if (server)
    {
        SendMessage(server, TAG("DGET"), index, column, Self(), 0);
        for (; ; )
        {
            sleep;
            if (FindMessageStr(TAG("DERR"), text, maxlen))
            {
                ret = -1;
                break;
            }
            if (FindMessageStr(TAG("DSTR"), text, maxlen))
            {
                break;
            }
        }
    }
    else
    {
        ret = datGetData(index, column, text, maxlen);
    }
    if ((album_kind == ALBUM_USB || album_kind == PHOTOS_USB) && column == USB_COL_NAME)
    {
    }
    if (field == FIELD_DATE)
    {
        new time_notation = GetSystemParam(SYS_TIME_NOTATION);
        new date_notation = GetSystemParam(SYS_DATE_NOTATION);
        photoDateGetFromDB(text, maxlen, date_notation, time_notation);
    }
    return ret;
}

