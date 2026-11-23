/*
 * /opt0/bn/script/mailer/mailsort.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The sort menu of the mailer, opened by main: the four folders with their sort key and order; main opens the
 * order of the chosen folder (g_iSortMenu) with mailstedit.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "mailer.inc"

#include "tag.inc"

#include <float>

/* Same as get_tag_attr2 of text.inc. */
get_tag_attr(tag[], id[], attr[], value[], maxlen)
{
    if (0 == tag[0] || 0 == id[0])
    {
        return 0;
    }
    new node = domFindTag(domRoot(), tag, id, 0);
    if (node)
    {
        new len = domGetData(node, attr, value, maxlen);
        if (0 == strcmp(attr, "src") || 0 == strcmp(attr, "href"))
        {
            xmlMakeFullPath(value, maxlen);
        }
        return len;
    }
    return 0;
}

#include "text.inc"

/* Same as send_focus of dom.inc. */
send_focus(obj, target, z_offset, Float:sharp, Float:extra_w, Float:extra_h, Float:move_time, smoothing, blinking)
{
    if (!obj)
    {
        return;
    }
    SendMessage(obj, TAG("FOC0"), target, TAG(""), z_offset, sharp);
    SendMessage(obj, TAG("FOC1"), extra_w, extra_h, move_time, smoothing);
    SendMessage(obj, TAG("FOC2"), blinking, 0, 0, 0);
}

#include "misc.inc"

new Float:g_flTextRed = 0.7843;
new Float:g_flTextGreen = 0.7843;
new Float:g_flTextBlue = 0.7843;
new Float:g_flTitleRed = 0.902;
new Float:g_flTitleGreen = 0.902;
new Float:g_flTitleBlue = 0.215;
new Float:g_flDimAlpha = 0.2;
new g_iItemCount;
new g_szAnyId[1]; /* an empty id: the first element with the tag */
new g_szItemId[64];

/* Fills ids with the ids of the empty leaves of the first tag element; returns their number. */
GetItemIds(tag[], ids[][])
{
    g_iItemCount = 0;
    new node = domFindTag(domRoot(), tag, g_szAnyId, 0);
    CollectItemIds(node, ids);
    return g_iItemCount;
}

/* Appends to ids, from g_iItemCount on, the id of every element below node with no children and no text. */
CollectItemIds(node, ids[][])
{
    new tag_name[16];
    domGetTagName(node, tag_name, 16);
    g_szItemId[0] = 0;
    domGetData(node, "id", g_szItemId, 64);
    static text[256];
    text[0] = 0;
    domGetData(node, _, text, 256);
    if (0 == domFirstChild(node))
    {
        if (0 == strlen(text))
        {
            sprintf(ids[g_iItemCount++], "%s", g_szItemId);
        }
    }
    else
    {
        for (new child = domFirstChild(node); child; child = domNext(child))
        {
            CollectItemIds(child, ids);
        }
    }
}

new g_iItemIndex;
new g_szFindId[64];

/* Returns the index of item id among the empty leaves of the first tag element, 0 when it is not there. */
GetItemIndex(tag[], id[])
{
    g_iItemCount = 0;
    g_iItemIndex = 0;
    new node = domFindTag(domRoot(), tag, g_szAnyId, 0);
    FindItemIndex(node, id);
    return g_iItemIndex;
}

/* Counts the empty leaves below node like CollectItemIds; the one whose id is id sets g_iItemIndex. */
FindItemIndex(node, id[])
{
    new tag_name[16];
    domGetTagName(node, tag_name, 16);
    g_szFindId[0] = 0;
    domGetData(node, "id", g_szFindId, 64);
    static text[256];
    text[0] = 0;
    domGetData(node, _, text, 256);
    if (0 == domFirstChild(node))
    {
        if (0 == strlen(text))
        {
            if (!strcmp(g_szFindId, id))
            {
                g_iItemIndex = g_iItemCount;
            }
            g_iItemCount++;
        }
    }
    else
    {
        for (new child = domFirstChild(node); child; child = domNext(child))
        {
            FindItemIndex(child, id);
        }
    }
}

public g_manager; /* the mailer page: gets "SHWT" when the menu is ready */
public g_focus; /* the focus object */
public g_active; /* the menu reads the pad */
public g_blink; /* unused, but part of the layout */
public g_iSortMenu; /* the selected folder */
new g_objTitle;
new g_objFolderNames[4];
new g_objKeys[4];
new g_objOrders[4];

main()
{
    new Float:pos[3];
    new Float:key_pos[3];
    new Float:order_pos[3];
    new Float:line_h;
    new keys[4];
    new orders[4];
    new value[32];
    new ids[4][16];
    GetItemIds("SORT_TOP_ITEM", ids);
    for (new i; i < 4; i++)
    {
        new index = GetItemIndex("SORT_TOP_ITEM", ids[i]);
        if (!strcmp(ids[i], "MAILER_SORT_INBOX"))
        {
            mx_GetMsgOrder(FOLDER_INBOX, keys[index], orders[index]);
        }
        else if (!strcmp(ids[i], "MAILER_SORT_OUTBOX"))
        {
            mx_GetMsgOrder(FOLDER_OUTBOX, keys[index], orders[index]);
        }
        else if (!strcmp(ids[i], "MAILER_SORT_DRAFT"))
        {
            mx_GetMsgOrder(FOLDER_DRAFT, keys[index], orders[index]);
        }
        else if (!strcmp(ids[i], "MAILER_SORT_QUEUE"))
        {
            mx_GetMsgOrder(FOLDER_QUEUE, keys[index], orders[index]);
        }
    }
    get_group_tag_attr2("SORT", "HEAD", 0, "folder_x", value, 32);
    pos[0] = atof(value);
    get_group_tag_attr2("SORT", "HEAD", 0, "folder_y", value, 32);
    pos[1] = atof(value);
    get_group_tag_attr2("SORT", "HEAD", 0, "folder_z", value, 32);
    pos[2] = atof(value);
    get_tag_attr("TEXT", "msgmailer_sort_title", "value", value, 32);
    g_objTitle = CreateLabel(Self());
    SetColor(g_objTitle, g_flTitleRed, g_flTitleGreen, g_flTitleBlue, 0.0);
    SetPos(g_objTitle, pos[0], pos[1], pos[2], 0);
    SetOptionS(g_objTitle, TAG("UTF8"), value);
    SetOptionI(g_objTitle, TAG("HALN"), ALIGN_CENTER);
    SetOptionI(g_objTitle, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(g_objTitle, TAG("FSIZ"), 17);
    SetOptionI(g_objTitle, TAG("ZSRT"), ZSORT_OVERLAY);
    get_group_tag_attr2("SORT", "HEAD", 0, "item_x", value, 32);
    pos[0] = atof(value);
    get_group_tag_attr2("SORT", "HEAD", 0, "item_y", value, 32);
    pos[1] = atof(value);
    get_group_tag_attr2("SORT", "HEAD", 0, "item_z", value, 32);
    pos[2] = atof(value);
    get_group_tag_attr2("SORT", "HEAD", 0, "key_x", value, 32);
    key_pos[0] = atof(value);
    get_group_tag_attr2("SORT", "HEAD", 0, "key_y", value, 32);
    key_pos[1] = atof(value);
    get_group_tag_attr2("SORT", "HEAD", 0, "key_z", value, 32);
    key_pos[2] = atof(value);
    get_group_tag_attr2("SORT", "HEAD", 0, "order_x", value, 32);
    order_pos[0] = atof(value);
    get_group_tag_attr2("SORT", "HEAD", 0, "order_y", value, 32);
    order_pos[1] = atof(value);
    get_group_tag_attr2("SORT", "HEAD", 0, "order_z", value, 32);
    order_pos[2] = atof(value);
    get_group_tag_attr2("SORT", "HEAD", 0, "line_height", value, 32);
    line_h = atof(value);
    for (new i; i < 4; i++)
    {
        if (!strcmp(ids[i], "MAILER_SORT_INBOX"))
        {
            get_tag_attr("TEXT", "msgmailer_sort_item_inbox", "value", value, 32);
        }
        else if (!strcmp(ids[i], "MAILER_SORT_OUTBOX"))
        {
            get_tag_attr("TEXT", "msgmailer_sort_item_outbox", "value", value, 32);
        }
        else if (!strcmp(ids[i], "MAILER_SORT_DRAFT"))
        {
            get_tag_attr("TEXT", "msgmailer_sort_item_draft", "value", value, 32);
        }
        else if (!strcmp(ids[i], "MAILER_SORT_QUEUE"))
        {
            get_tag_attr("TEXT", "msgmailer_sort_item_queue", "value", value, 32);
        }
        g_objFolderNames[i] = CreateLabel(Self());
        SetColor(g_objFolderNames[i], 0.215, 0.708, 0.999, 0.0);
        SetPos(g_objFolderNames[i], pos[0], pos[1] + line_h * i, pos[2], 0);
        SetOptionS(g_objFolderNames[i], TAG("UTF8"), value);
        SetOptionI(g_objFolderNames[i], TAG("HALN"), ALIGN_RIGHT);
        SetOptionI(g_objFolderNames[i], TAG("VALN"), ALIGN_CENTER);
        SetOptionI(g_objFolderNames[i], TAG("FSIZ"), 15);
        SetOptionI(g_objFolderNames[i], TAG("ZSRT"), ZSORT_OVERLAY);
        if (keys[i] == SORT_KEY_NUMBER)
        {
            get_tag_attr("TEXT", "msgmailer_sort_item_key_none", "value", value, 32);
        }
        else if (keys[i] == SORT_KEY_DATE)
        {
            get_tag_attr("TEXT", "msgmailer_sort_item_key_date", "value", value, 32);
        }
        else if (keys[i] == SORT_KEY_FROM)
        {
            get_tag_attr("TEXT", "msgmailer_sort_item_key_from", "value", value, 32);
        }
        else if (keys[i] == SORT_KEY_TO)
        {
            get_tag_attr("TEXT", "msgmailer_sort_item_key_to", "value", value, 32);
        }
        else if (keys[i] == SORT_KEY_SUBJECT)
        {
            get_tag_attr("TEXT", "msgmailer_sort_item_key_subject", "value", value, 32);
        }
        g_objKeys[i] = CreateLabel(Self());
        SetColor(g_objKeys[i], g_flTextRed, g_flTextGreen, g_flTextBlue, 0.0);
        SetPos(g_objKeys[i], key_pos[0], key_pos[1] + line_h * i, key_pos[2], 0);
        SetOptionS(g_objKeys[i], TAG("UTF8"), value);
        SetOptionI(g_objKeys[i], TAG("HALN"), 4); /* not a horizontal alignment: the label is aligned left */
        SetOptionI(g_objKeys[i], TAG("VALN"), ALIGN_CENTER);
        SetOptionI(g_objKeys[i], TAG("FSIZ"), 15);
        SetOptionI(g_objKeys[i], TAG("ZSRT"), ZSORT_OVERLAY);
        if (SORT_ASCEND == orders[i])
        {
            get_tag_attr("TEXT", "msgmailer_sort_item_order_ascend", "value", value, 32);
        }
        else if (orders[i] == SORT_DESCEND)
        {
            get_tag_attr("TEXT", "msgmailer_sort_item_order_descend", "value", value, 32);
        }
        g_objOrders[i] = CreateLabel(Self());
        SetColor(g_objOrders[i], g_flTextRed, g_flTextGreen, g_flTextBlue, 0.0);
        SetPos(g_objOrders[i], order_pos[0], order_pos[1] + line_h * i, order_pos[2], 0);
        SetOptionS(g_objOrders[i], TAG("UTF8"), value);
        SetOptionI(g_objOrders[i], TAG("HALN"), 4); /* not a horizontal alignment: the label is aligned left */
        SetOptionI(g_objOrders[i], TAG("VALN"), ALIGN_CENTER);
        SetOptionI(g_objOrders[i], TAG("FSIZ"), 15);
        SetOptionI(g_objOrders[i], TAG("ZSRT"), ZSORT_OVERLAY);
    }
    SetOptionI(g_focus, TAG("ZSRT"), ZSORT_OVERLAY);
    sleep;
    SendMessage(Self(), TAG("FADE"), TAG("IN"), 0, 0, 0);
    SendMessage(Self(), TAG("FOCS"), TAG("IN"), 0, 0, 0);
    SendMessage(g_manager, TAG("SHWT"), 0, 0, 0, 0);
    for (; ; )
    {
        new held;
        new pressed;
        new released;
        new repeated;
        if (g_active)
        {
            PadState(held, pressed, released, repeated, -1);
        }
        static refocus;
        static last_tick;
        if (pressed & PAD_UP || (repeated & PAD_UP && tickcount() - last_tick > 70))
        {
            last_tick = tickcount();
            if (g_iSortMenu - 1 >= 0)
            {
                g_iSortMenu--;
                sndSePlay(SE_CURSOR);
                refocus = 1;
            }
        }
        else if (pressed & PAD_DOWN || (repeated & PAD_DOWN && tickcount() - last_tick > 70))
        {
            last_tick = tickcount();
            if (g_iSortMenu + 1 < 4)
            {
                g_iSortMenu++;
                sndSePlay(SE_CURSOR);
                refocus = 1;
            }
        }
        if (g_active && refocus)
        {
            send_label_focus2(g_focus, g_objFolderNames[g_iSortMenu], 0, 0.5, 60.0, 60.0, 70.0, 1, 1);
            refocus = 0;
        }
        new msg;
        new what;
        new arg;
        while (RecvMessage(msg, what, arg))
        {
            if (msg == TAG("FOCS"))
            {
                if (what == TAG("IN"))
                {
                    SetSystemParam(SYS_PADHELP, PADHELP_CIRCLE | PADHELP_CROSS);
                    g_active = 1;
                    refocus = 1;
                }
                else if (what == TAG("OUT"))
                {
                    g_active = 0;
                    send_focus(g_focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                }
            }
            else if (msg == TAG("BLNK"))
            {
            }
            else if (msg == TAG("FADE"))
            {
                if (what == TAG("IN"))
                {
                    FadeIn();
                }
                else if (what == TAG("OUT"))
                {
                    FadeOut();
                }
                else if (what == TAG("DIM"))
                {
                    FadeDim();
                }
            }
        }
        sleep;
    }
    return;
}

/* Fades the menu to alpha. */
FadeAll(Float:alpha, Float:fade_time, Float:delay)
{
    new Float:color[4];
    GetColor(g_objTitle, color[0], color[1], color[2], color[3]);
    AnimColor(g_objTitle, color[0], color[1], color[2], alpha, fade_time, delay, ANIM_FUNC_LINEAR);
    for (new i; i < 4; i++)
    {
        GetColor(g_objFolderNames[i], color[0], color[1], color[2], color[3]);
        AnimColor(g_objFolderNames[i], color[0], color[1], color[2], alpha, fade_time, delay, ANIM_FUNC_LINEAR);
        GetColor(g_objKeys[i], color[0], color[1], color[2], color[3]);
        AnimColor(g_objKeys[i], color[0], color[1], color[2], alpha, fade_time, delay, ANIM_FUNC_LINEAR);
        GetColor(g_objOrders[i], color[0], color[1], color[2], color[3]);
        AnimColor(g_objOrders[i], color[0], color[1], color[2], alpha, fade_time, delay, ANIM_FUNC_LINEAR);
    }
}

FadeIn()
{
    FadeAll(1.0, float(400), 0.0);
}

FadeOut()
{
    FadeAll(0.0, float(400), 0.0);
}

FadeDim()
{
    FadeAll(g_flDimAlpha, float(400), 0.0);
}

#include "mailsync.inc"
