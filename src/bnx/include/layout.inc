/*
 * The layout of the menu lists of the music and movie pages, in fractions of the screen; the page arrows and the
 * background fade of the edclient pages, and the image resources of a page.
 */

/* Gets the height of a list of count items: count - 1 rows, from 4 to 7, and the title. */
stock get_list_height(count, has_title, &Float:list_h)
{
    if (count <= 5)
    {
        list_h = 0.0583 * 4;
        if (has_title)
        {
            list_h = 0.0875 + list_h;
        }
    }
    else if (count >= 8)
    {
        list_h = 0.0583 * 7;
        if (has_title)
        {
            list_h = 0.0875 + list_h;
        }
    }
    else
    {
        list_h = 0.0583 * (count - 1);
        if (has_title)
        {
            list_h = 0.0875 + list_h;
        }
    }
}

/* Gets the position of a list on the left (side 0) or on the right (side 1, MENU_SIDE_* of stdmenu.inc). */
stock get_list_pos(side, count, has_title, &Float:x, &Float:y)
{
    new count_copy = count; /* redundant as written */
    new title_copy = has_title;
    count = count_copy;
    has_title = title_copy;
    if (0 == side)
    {
        x = 0.275;
    }
    else if (side == 1)
    {
        x = 0.74;
    }
    y = 0.5;
}

/* Gets the height of the items of a list: count - 1 rows, from 4 to 7. */
stock get_list_body_height(count, has_title, &Float:list_h)
{
    new title_copy = has_title; /* redundant as written */
    has_title = title_copy;
    if (count <= 5)
    {
        list_h = 0.0583 * 4;
    }
    else if (count >= 8)
    {
        list_h = 0.0583 * 7;
    }
    else
    {
        list_h = 0.0583 * (count - 1);
    }
}

/* Gets the position of the title of a list; returns -1 if it has none. */
stock get_list_title_pos(side, count, has_title, &Float:x, &Float:y)
{
    new count_copy = count; /* redundant as written */
    count = count_copy;
    if (!has_title)
    {
        return -1;
    }
    if (0 == side)
    {
        x = 0.275;
    }
    else if (side == 1)
    {
        x = 0.74;
    }
    new Float:list_h;
    get_list_height(count, has_title, list_h);
    y = 0.5 - 0.5 * list_h;
    return 0;
}

/* Gets the position of the items of a list, half a row lower with a title. */
stock get_list_body_pos(side, count, has_title, &Float:x, &Float:y)
{
    new count_copy = count; /* redundant as written */
    count = count_copy;
    if (0 == side)
    {
        x = 0.275;
    }
    else if (side == 1)
    {
        x = 0.74;
    }
    if (!has_title)
    {
        y = 0.5;
    }
    else
    {
        y = 0.5 + 0.5 * 0.0875;
    }
}

/* Gets the top of the items of a list. */
stock get_list_top(count, has_title, &Float:y)
{
    new Float:list_h;
    get_list_height(count, has_title, list_h);
    if (has_title)
    {
        y = 0.0875 + (0.5 - 0.5 * list_h);
    }
    else
    {
        y = 0.5 - 0.5 * list_h;
    }
}

/* Gets the position of the scroll bar of a list, at the edge of its side. */
stock get_scroll_bar_pos(side, count, has_title, &Float:x, &Float:y)
{
    new Float:list_x;
    get_list_body_pos(side, count, has_title, list_x, y);
    if (0 == side)
    {
        x = 0.047;
    }
    else if (side == 1)
    {
        x = 0.953;
    }
}

#if !defined PAGE_ARROW_NEXT
/* As in edpage.inc. */
const PAGE_ARROW_NEXT = 0;
const PAGE_ARROW_NEXT_LIT = 1;
const PAGE_ARROW_PREV = 2;
const PAGE_ARROW_PREV_LIT = 3;
#endif

/*
 * Shows the previous and next arrows of a page, plain or lit, or hides them; arrows holds the planes of the arrows,
 * by PAGE_ARROW_*.
 */
stock update_page_arrows(arrows[], show_prev, show_next, next_lit, prev_lit)
{
    if (show_next == 1)
    {
        if (next_lit == 1)
        {
            AnimColor(arrows[PAGE_ARROW_NEXT], 1.0, 1.0, 1.0, 0.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
            AnimColor(arrows[PAGE_ARROW_NEXT_LIT], 1.0, 1.0, 1.0, 1.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
        }
        else
        {
            AnimColor(arrows[PAGE_ARROW_NEXT], 1.0, 1.0, 1.0, 1.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
            AnimColor(arrows[PAGE_ARROW_NEXT_LIT], 1.0, 1.0, 1.0, 0.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
        }
    }
    else
    {
        AnimColor(arrows[PAGE_ARROW_NEXT], 1.0, 1.0, 1.0, 0.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
        AnimColor(arrows[PAGE_ARROW_NEXT_LIT], 1.0, 1.0, 1.0, 0.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
    }
    if (show_prev == 1)
    {
        if (prev_lit == 1)
        {
            AnimColor(arrows[PAGE_ARROW_PREV], 1.0, 1.0, 1.0, 0.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
            AnimColor(arrows[PAGE_ARROW_PREV_LIT], 1.0, 1.0, 1.0, 1.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
        }
        else
        {
            AnimColor(arrows[PAGE_ARROW_PREV], 1.0, 1.0, 1.0, 1.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
            AnimColor(arrows[PAGE_ARROW_PREV_LIT], 1.0, 1.0, 1.0, 0.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
        }
    }
    else
    {
        AnimColor(arrows[PAGE_ARROW_PREV], 1.0, 1.0, 1.0, 0.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
        AnimColor(arrows[PAGE_ARROW_PREV_LIT], 1.0, 1.0, 1.0, 0.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
    }
}

/* Fades the background of the edclient pages to target_a over duration ms. */
stock fade_ed_background(Float:target_a, Float:duration)
{
    new scene = wndGetScene(WND_BG);
    if (0 != scene)
    {
        SendMessage(scene, TAG("FADE"), target_a, duration, 0, 0);
    }
}

#if !defined g_any_child2
new g_any_child2[1];
#endif
#if !defined g_any_next2
new g_any_next2[1];
#endif
/* Decodes the image of every IMG element and records its texture and size in a new IMG_RESOURCE element. */
stock load_image_resources()
{
    new id[64];
    new buf[64];
    new node;
    new len;
    node = domFirstChild(domRoot(), g_any_child2);
    for (; node; node = domNext(node, g_any_next2))
    {
        domGetTagName(node, buf, 64);
        if (0 != strcmp("IMG", buf))
        {
            continue;
        }
        len = domGetData(node, "id", id, 64);
        if (0 == len)
        {
            continue;
        }
        len = domGetData(node, "src", buf, 64);
        if (0 == len)
        {
            continue;
        }
        imgDecStart(buf, 0, 0, -1, 100, 0);
        for (; ; )
        {
            new status = imgDecSync();
            switch (status)
            {
                case IMG_BUSY:
                {
                }
                case IMG_DONE:
                {
                    new tex;
                    new tex_w;
                    new tex_h;
                    new res;
                    tex = imgTexID();
                    imgKeepTex(tex);
                    tex_w = imgTexOrgWidth();
                    tex_h = imgTexOrgHeight();
                    res = domAddTag(domRoot(), "IMG_RESOURCE");
                    domSetData(res, "id", id);
                    sprintf(buf, "%d", tex);
                    domSetData(res, "texture", buf);
                    sprintf(buf, "%d", tex_w);
                    domSetData(res, "w", buf);
                    sprintf(buf, "%d", tex_h);
                    domSetData(res, "h", buf);
                    break;
                }
                case IMG_FAILED:
                {
                    printf("@@@ Error : image decode fail. [%s]\n", buf);
                    break;
                }
                default:
                {
                }
            }
            sleep;
        }
    }
}

