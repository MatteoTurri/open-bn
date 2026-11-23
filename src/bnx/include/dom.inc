/*
 * Lookups of child elements in the document, textures, focus messages and Base64 digits.
 */

/* Decodes an image and returns its texture, 0 on failure. */
stock load_texture(url[], tex_w, tex_h, dec_mode)
{
    if (0 == strlen(url))
    {
        return 0;
    }
    new path[256];
    sprintf(path, url);
    imgDecStart(path, tex_w, tex_h, dec_mode, 100, 0);
    new status = wait_image_decode();
    if (IMG_DONE != status)
    {
        return 0;
    }
    return imgTexID();
}

/* Sends the focus parameters to a focus object ("FOC0" to "FOC2"), with no class. */
stock send_focus(obj, target, z_offset, Float:sharp, Float:extra_w, Float:extra_h, Float:move_time, smoothing, blinking)
{
    if (!obj)
    {
        return;
    }
    SendMessage(obj, TAG("FOC0"), target, TAG(""), z_offset, sharp);
    SendMessage(obj, TAG("FOC1"), extra_w, extra_h, move_time, smoothing);
    SendMessage(obj, TAG("FOC2"), blinking, 0, 0, 0);
}

/*
 * Gets the position of the display point (screen_x, screen_y) in the space of rel (by default the parent, else this
 * object).
 */
stock map_screen_point(&Float:x, &Float:y, &Float:z, Float:screen_x, Float:screen_y, rel)
{
    if (!rel)
    {
        rel = Parent(0);
    }
    if (!rel)
    {
        rel = Self();
    }
    GetOffPos(Disp2D(0), x, y, z, screen_x, screen_y, 0.0, rel);
}

new g_any_child[1]; /* empty: any element */
new g_any_next[1]; /* empty: any element */
/*
 * Returns the first tag child of parent whose attribute attr is value (any, when both are empty), 0 if there is
 * none.
 */
stock find_child_tag(parent, tag[], attr[], value[])
{
    new name[64];
    new node;
    new len;
    node = domFirstChild(parent, g_any_child);
    for (; node; node = domNext(node, g_any_next))
    {
        domGetTagName(node, name, 64);
        if (0 != strcmp(tag, name))
        {
            continue;
        }
        if (0 == strlen(attr) && 0 == strlen(value))
        {
            return node;
        }
        len = domGetData(node, attr, name, 64);
        if (0 == len)
        {
            continue;
        }
        if (0 == strcmp(value, name))
        {
            return node;
        }
    }
    return 0;
}

/* Returns the Base64 digit of a 6-bit value. */
stock get_base64_char(value)
{
    if (0 <= value <= 25)
    {
        return value - 0 + 'A';
    }
    if (26 <= value <= 51)
    {
        return value - 26 + 'a';
    }
    if (52 <= value <= 61)
    {
        return value - 52 + '0';
    }
    if (value == 62)
    {
        return '+';
    }
    if (value == 63)
    {
        return '/';
    }
    return 0;
}

#if !defined g_any_child2
new g_any_child2[1]; /* empty: any element */
#endif
#if !defined g_any_next2
new g_any_next2[1]; /* empty: any element */
#endif
/* Same as find_child_tag, with globals of its own. */
stock find_child_tag2(parent, tag[], attr[], value[])
{
    new name[64];
    new node;
    new len;
    node = domFirstChild(parent, g_any_child2);
    for (; node; node = domNext(node, g_any_next2))
    {
        domGetTagName(node, name, 64);
        if (0 != strcmp(tag, name))
        {
            continue;
        }
        if (0 == strlen(attr) && 0 == strlen(value))
        {
            return node;
        }
        len = domGetData(node, attr, name, 64);
        if (0 == len)
        {
            continue;
        }
        if (0 == strcmp(value, name))
        {
            return node;
        }
    }
    return 0;
}

/* Returns the index-th tag child of parent, 0 if there is none. */
stock find_nth_child_tag(parent, tag[], index)
{
    new count;
    new node;
    new name[64];
    node = domFirstChild(parent, g_any_child2);
    for (; node; node = domNext(node, g_any_next2))
    {
        domGetTagName(node, name, 64);
        if (0 != strcmp(tag, name))
        {
            continue;
        }
        if (count == index)
        {
            return node;
        }
        count++;
    }
    return 0;
}

#if !defined g_any_child3
new g_any_child3[1]; /* empty: any element */
#endif
#if !defined g_any_next3
new g_any_next3[1]; /* empty: any element */
#endif
/* Same as find_nth_child_tag, with globals of its own. */
stock find_nth_child_tag2(parent, tag[], index)
{
    new count;
    new node;
    new name[64];
    node = domFirstChild(parent, g_any_child3);
    for (; node; node = domNext(node, g_any_next3))
    {
        domGetTagName(node, name, 64);
        if (0 != strcmp(tag, name))
        {
            continue;
        }
        if (count == index)
        {
            return node;
        }
        count++;
    }
    return 0;
}

/* Waits, from the next frame, until the pending XML load ends. */
stock wait_xml_idle()
{
    for (new status; ; )
    {
        sleep;
        status = xmlLoadSync();
        if (0 != status)
        {
            break;
        }
    }
}

