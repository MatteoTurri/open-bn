/*
 * /opt0/bn/script/mailer/mailgbl.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The main menu of the mailer (write, address book, send and receive, settings, sort, quit), shown over
 * the dimmed page; the mailer page runs the item selected.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "tag.inc"

#include <float>

/* Returns whether str is a packed string. */
is_packed(str[])
{
    return str[0] <= 0 || str[0] > 255;
}

/* Copies the packed or unpacked string src into dest, at most maxlen cells. */
copy_string(dest[], src[], maxlen)
{
    new len = strlen(src) + 1;
    if (is_packed(src))
    {
        len = (len + 3) / 4;
    }
    len = min(len, maxlen);
    for (new i; i < len; i++)
    {
        dest[i] = src[i];
    }
}

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

new Float:g_flBackdropRed = 0.25;
new Float:g_flBackdropGreen = 0.25;
new Float:g_flBackdropBlue = 0.25;
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

public g_manager; /* the mailer page: holds the background texture and gets "SHWT" when the menu is ready */
public g_focus; /* the focus object, moved onto the selected item */
public g_active; /* 1 while the list has the focus and takes the pad */
public g_iGblMenu; /* the selected item */
new g_objBackdrop;
new g_objBg;
new g_objItems[6];
new g_szItemNames[6][16];
new g_szItemIds[6][16];

main()
{
    new text[32];
    new Float:pos[3];
    new Float:line_h;
    new bg_tex = GetPubVar(g_manager, "g_txBgGbl");
    new Float:bg_w = GetPubVar(g_manager, "g_BgGblWidth");
    new Float:bg_h = GetPubVar(g_manager, "g_BgGblHeight");
    GetItemIds("GBL_ITEM", g_szItemIds);
    for (new i; i < 6; i++)
    {
        copy_string(g_szItemNames[i], "", 64);
        if (!strcmp("MAILER_GBL_NEW", g_szItemIds[i]))
        {
            get_tag_attr("TEXT", "msgmailer_gbl_write", "value", g_szItemNames[i], 16);
        }
        else if (!strcmp("MAILER_GBL_ADDR", g_szItemIds[i]))
        {
            get_tag_attr("TEXT", "msgmailer_gbl_addredit", "value", g_szItemNames[i], 16);
        }
        else if (!strcmp("MAILER_GBL_REFRESH", g_szItemIds[i]))
        {
            get_tag_attr("TEXT", "msgmailer_gbl_sendrecv", "value", g_szItemNames[i], 16);
        }
        else if (!strcmp("MAILER_GBL_SETTINGS", g_szItemIds[i]))
        {
            get_tag_attr("TEXT", "msgmailer_gbl_set", "value", g_szItemNames[i], 16);
        }
        else if (!strcmp("MAILER_GBL_SORT", g_szItemIds[i]))
        {
            get_tag_attr("TEXT", "msgmailer_gbl_sort", "value", g_szItemNames[i], 16);
        }
        else if (!strcmp("MAILER_GBL_FINISH", g_szItemIds[i]))
        {
            get_tag_attr("TEXT", "msgmailer_gbl_quit", "value", g_szItemNames[i], 16);
        }
    }
    g_objBackdrop = CreateDefocus(Self());
    SetOptionI(g_objBackdrop, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionI(g_objBackdrop, TAG("LEVL"), 3);
    SetColor(g_objBackdrop, g_flBackdropRed, g_flBackdropGreen, g_flBackdropBlue, 0.0);
    get_group_tag_attr2("GBL", "HEAD", 0, "tex_x", text, 32);
    pos[0] = atof(text);
    get_group_tag_attr2("GBL", "HEAD", 0, "tex_y", text, 32);
    pos[1] = atof(text);
    get_group_tag_attr2("GBL", "HEAD", 0, "tex_z", text, 32);
    pos[2] = atof(text);
    g_objBg = CreatePlane(Self());
    SetPos(g_objBg, pos[0], pos[1], pos[2], 0);
    SetColor(g_objBg, 1.0, 1.0, 1.0, 0.0);
    SetScale(g_objBg, bg_w, bg_h, 0.0);
    SetOptionI(g_objBg, TAG("ZSRT"), ZSORT_OVERLAY);
    SetTexture(g_objBg, bg_tex, 0, -1);
    get_group_tag_attr2("GBL", "HEAD", 0, "item_x", text, 32);
    pos[0] = atof(text);
    get_group_tag_attr2("GBL", "HEAD", 0, "item_y", text, 32);
    pos[1] = atof(text);
    get_group_tag_attr2("GBL", "HEAD", 0, "item_z", text, 32);
    pos[2] = atof(text);
    get_group_tag_attr2("GBL", "HEAD", 0, "line_height", text, 32);
    line_h = atof(text);
    for (new i; i < 6; i++)
    {
        g_objItems[i] = CreateLabel(g_objBackdrop);
        SetPos(g_objItems[i], pos[0], pos[1] + line_h * i, pos[2], 0);
        SetColor(g_objItems[i], 0.215, 0.708, 0.999, 0.0);
        SetOptionS(g_objItems[i], TAG("UTF8"), g_szItemNames[i]);
        SetOptionI(g_objItems[i], TAG("HALN"), ALIGN_CENTER);
        SetOptionI(g_objItems[i], TAG("VALN"), ALIGN_CENTER);
        SetOptionI(g_objItems[i], TAG("FSIZ"), 15);
        SetOptionI(g_objItems[i], TAG("ZSRT"), ZSORT_OVERLAY);
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
        /* refocus: the focus must move to the selected item; last_repeat: time of the last auto-repeat step */
        static refocus;
        static last_repeat;
        if (pressed & PAD_UP || (repeated & PAD_UP && tickcount() - last_repeat > 70))
        {
            last_repeat = tickcount();
            if (g_iGblMenu - 1 >= 0)
            {
                g_iGblMenu--;
                sndSePlay(SE_CURSOR);
                refocus = 1;
            }
        }
        else if (pressed & PAD_DOWN || (repeated & PAD_DOWN && tickcount() - last_repeat > 70))
        {
            last_repeat = tickcount();
            if (g_iGblMenu + 1 < 6)
            {
                g_iGblMenu++;
                sndSePlay(SE_CURSOR);
                refocus = 1;
            }
        }
        if (g_active && refocus)
        {
            send_label_focus2(g_focus, g_objItems[g_iGblMenu], 0, 0.5, 60.0, 60.0, 70.0, 1, 1);
            refocus = 0;
        }
        new msg;
        new what;
        new arg;
        /* "FOCS" IN/OUT, "FADE" IN/OUT. */
        while (RecvMessage(msg, what, arg))
        {
            if (msg == TAG("FOCS"))
            {
                if (what == TAG("IN"))
                {
                    ShowPadHelp();
                    g_active = 1;
                    refocus = 1;
                }
                else if (what == TAG("OUT"))
                {
                    g_active = 0;
                    send_focus(g_focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                }
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
            }
        }
        sleep;
    }
    return;
}

/* Fades the backdrop and the items to alpha. */
FadeItems(Float:alpha, Float:duration, Float:delay)
{
    new Float:color[4];
    GetColor(g_objBackdrop, color[0], color[1], color[2], color[3]);
    AnimColor(g_objBackdrop, color[0], color[1], color[2], alpha, duration, delay, ANIM_FUNC_LINEAR);
    for (new i; i < 6; i++)
    {
        GetColor(g_objItems[i], color[0], color[1], color[2], color[3]);
        AnimColor(g_objItems[i], color[0], color[1], color[2], alpha, duration, delay, ANIM_FUNC_LINEAR);
    }
}

FadeIn()
{
    AnimColor(g_objBg, 1.0, 1.0, 1.0, 1.0, float(400), 0.0, ANIM_FUNC_EXPONENT);
    FadeItems(1.0, float(500), 0.0);
}

FadeOut()
{
    AnimColor(g_objBg, 1.0, 1.0, 1.0, 0.0, float(400), 0.0, ANIM_FUNC_EXPONENT);
    FadeItems(0.0, float(500), 0.0);
}

/* Shows the circle and cross buttons in the pad help, the circle labelled "enter". */
ShowPadHelp()
{
    new label[8];
    get_tag_attr("TEXT", "msgmailer_btn_enter", "value", label, 8);
    SetSystemParamStr(SYS_STR_PADHELP_RR, label);
    SetSystemParam(SYS_PADHELP, PADHELP_CIRCLE | PADHELP_CROSS);
}
