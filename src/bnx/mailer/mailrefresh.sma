/*
 * /opt0/bn/script/mailer/mailrefresh.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The send and receive menu of the mailer: receiving and sending as a two-item list; the mailer page
 * starts the one selected.
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

public g_manager; /* the mailer page: gets "SHWT" when the list is ready */
public g_focus; /* the focus object, moved onto the selected item */
public g_active; /* 1 while the list has the focus and takes the pad */
public g_blink; /* unused, but part of the layout */
public g_bFocus; /* 0 while a dialog is open ("DLG" IN): the focus is not moved */
public g_iLine; /* the selected item */
new g_objTitle;
new g_objItems[2];

main()
{
    new Float:pos[3];
    new Float:line_h;
    new text[32];
    get_group_tag_attr2("REFRESH", "HEAD", 0, "folder_x", text, 32);
    pos[0] = atof(text);
    get_group_tag_attr2("REFRESH", "HEAD", 0, "folder_y", text, 32);
    pos[1] = atof(text);
    get_group_tag_attr2("REFRESH", "HEAD", 0, "folder_z", text, 32);
    pos[2] = atof(text);
    get_tag_attr("TEXT", "msgmailer_sendrecv_title", "value", text, 32);
    g_objTitle = CreateLabel(Self());
    SetColor(g_objTitle, g_flTitleRed, g_flTitleGreen, g_flTitleBlue, 0.0);
    SetPos(g_objTitle, pos[0], pos[1], pos[2], 0);
    SetOptionS(g_objTitle, TAG("UTF8"), text);
    SetOptionI(g_objTitle, TAG("HALN"), ALIGN_CENTER);
    SetOptionI(g_objTitle, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(g_objTitle, TAG("FSIZ"), 17);
    SetOptionI(g_objTitle, TAG("ZSRT"), ZSORT_OVERLAY);
    new item_ids[2][16];
    GetItemIds("REFRESH_ITEM", item_ids);
    get_group_tag_attr2("REFRESH", "HEAD", 0, "item_x", text, 32);
    pos[0] = atof(text);
    get_group_tag_attr2("REFRESH", "HEAD", 0, "item_y", text, 32);
    pos[1] = atof(text);
    get_group_tag_attr2("REFRESH", "HEAD", 0, "item_z", text, 32);
    pos[2] = atof(text);
    get_group_tag_attr2("REFRESH", "HEAD", 0, "line_height", text, 32);
    line_h = atof(text);
    for (new i; i < 2; i++)
    {
        if (!strcmp(item_ids[i], "MAILER_REFRESH_RECV"))
        {
            get_tag_attr("TEXT", "msgmailer_sendrecv_item_recv", "value", text, 32);
        }
        else if (!strcmp(item_ids[i], "MAILER_REFRESH_SEND"))
        {
            get_tag_attr("TEXT", "msgmailer_sendrecv_item_send", "value", text, 32);
        }
        g_objItems[i] = CreateLabel(Self());
        SetColor(g_objItems[i], 0.215, 0.708, 0.999, 0.0);
        SetPos(g_objItems[i], pos[0], pos[1] + line_h * i, pos[2], 0);
        SetOptionS(g_objItems[i], TAG("UTF8"), text);
        SetOptionI(g_objItems[i], TAG("HALN"), ALIGN_CENTER);
        SetOptionI(g_objItems[i], TAG("VALN"), ALIGN_CENTER);
        SetOptionI(g_objItems[i], TAG("FSIZ"), 15);
        SetOptionI(g_objItems[i], TAG("ZSRT"), ZSORT_OVERLAY);
    }
    SetOptionI(g_focus, TAG("ZSRT"), ZSORT_OVERLAY);
    g_iLine = 0;
    g_bFocus = 1;
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
        /* the focus must move to the selected item */
        static refocus;
        if (pressed & PAD_UP && g_iLine - 1 >= 0)
        {
            g_iLine--;
            sndSePlay(SE_CURSOR);
            refocus = 1;
        }
        else if (pressed & PAD_DOWN && g_iLine + 1 < 2)
        {
            g_iLine++;
            sndSePlay(SE_CURSOR);
            refocus = 1;
        }
        if (g_bFocus && g_active && refocus)
        {
            send_label_focus2(g_focus, g_objItems[g_iLine], 0, 0.5, 60.0, 60.0, 70.0, 1, 1);
            refocus = 0;
        }
        new msg;
        new what;
        new arg;
        /* "FOCS" IN/OUT, "FADE" IN/OUT/DIM, "DLG" IN/OUT when a dialog opens and closes. */
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
            else if (msg == TAG("DLG"))
            {
                if (what == TAG("IN"))
                {
                    send_focus(g_focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                    g_active = 0;
                    g_bFocus = 0;
                }
                else if (what == TAG("OUT"))
                {
                    g_active = 1;
                    g_bFocus = 1;
                }
            }
        }
        sleep;
    }
    return;
}

/* Fades every label of the list to alpha. */
FadeItems(Float:alpha, Float:duration, Float:delay)
{
    new Float:color[4];
    GetColor(g_objTitle, color[0], color[1], color[2], color[3]);
    AnimColor(g_objTitle, color[0], color[1], color[2], alpha, duration, delay, ANIM_FUNC_LINEAR);
    for (new i; i < 2; i++)
    {
        GetColor(g_objItems[i], color[0], color[1], color[2], color[3]);
        AnimColor(g_objItems[i], color[0], color[1], color[2], alpha, duration, delay, ANIM_FUNC_LINEAR);
    }
}

FadeIn()
{
    FadeItems(1.0, float(400), 0.0);
}

FadeOut()
{
    FadeItems(0.0, float(400), 0.0);
}

FadeDim()
{
    FadeItems(g_flDimAlpha, float(400), 0.0);
}

#include "mailsync.inc"
