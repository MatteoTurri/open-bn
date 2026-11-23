/*
 * Checks of the fields of the edclient forms, and copies to and from the photo tmp directory.
 */

/* Returns 1 for 1 to 63 characters among letters, digits and .-_@#+!$:%*. */
stock is_valid_psbb_id(id[])
{
    if (!(strlen(id) > 0 && strlen(id) <= 63))
    {
        return 0;
    }
    new i;
    new c;
    i = 0;
    for (; id{i}; i++)
    {
        c = id{i};
        if (c >= '0' && c <= '9')
        {
            continue;
        }
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
        {
            continue;
        }
        if (c == '.' || c == '_' || c == '-' || c == '@' || c == '#' || c == '+' || c == '!' || c == '$' || c == ':'
                || c == '%' || c == '*')
        {
            continue;
        }
        return 0;
    }
    return 1;
}

/* Records in the MENUGROUP element group the group and the item to return to. */
stock set_menu_return(group[], ret_group[], ret_item[])
{
    new node = domFindTag(domRoot(), "MENUGROUP", group, 0);
    if (!node)
    {
        return 0;
    }
    domSetData(node, "retgroup", ret_group);
    domSetData(node, "retitem", ret_item);
    return 1;
}

#if !defined g_not_found
new g_not_found = -2;
#endif
#if !defined g_photo_tmp_dir
new g_photo_tmp_dir[] = "pfs:/__contents/contents/photo/tmp/";
#endif
/*
 * Copies src into the photo tmp directory and sets dest to the path of the copy; returns 1 if src is missing and
 * missing_ok, 0, or a negative error.
 */
stock copy_to_photo_tmp(dest[], src[], missing_ok, ignored, progress_win, from, to)
{
    printf("copy to filetmp\n");
    new st_mode;
    new st_size;
    new ret = fileLstat(src, st_mode, st_size);
    if (ret == g_not_found)
    {
        if (missing_ok)
        {
            return 1;
        }
        return -2;
    }
    new name[64];
    new slash = find_last_nocase(src, "/");
    copy_tail(name, src, slash + 1, 64);
    sprintf(dest, "%s%s", g_photo_tmp_dir, name, ignored);
    printf("COPY to TMP name = %s\n", src);
    new error;
    ret = fileCopy(src, dest);
    if (ret < 0)
    {
        return -2;
    }
    if (wait_file_op(ret, error, progress_win, from, to) < 0)
    {
        if (ret < 0)
        {
            return -2;
        }
        if (error == 2)
        {
            return -4;
        }
    }
    return 0;
}

stock is_not_empty(text[])
{
    if (strlen(text) > 0)
    {
        return 1;
    }
    return 0;
}

/* Moves src to dest; returns 1 if src is missing and missing_ok, 0, or -2 on error. */
stock move_from_photo_tmp(dest[], src[], missing_ok)
{
    printf("move from filetmp\n");
    new st_mode;
    new st_size;
    new ret = fileLstat(src, st_mode, st_size);
    if (ret == g_not_found)
    {
        if (missing_ok)
        {
            return 1;
        }
        return -2;
    }
    printf("MOVEfrom DST name = %s\n", dest);
    ret = fileMove(src, dest);
    if (ret < 0)
    {
        return -2;
    }
    if (wait_file_op(ret, st_mode, 0, 0, 100) < 0)
    {
        if (ret < 0)
        {
            return -2;
        }
    }
    return 0;
}

/* Returns 1 for 1 to 64 characters with an at sign and a dot. */
stock is_valid_mail_address(addr[])
{
    if (!(strlen(addr) > 0 && strlen(addr) <= 64))
    {
        return 0;
    }
    if (-1 != find_substring(addr, "@", 0))
    {
        if (-1 != find_substring(addr, ".", 0))
        {
            return 1;
        }
    }
    return 0;
}

