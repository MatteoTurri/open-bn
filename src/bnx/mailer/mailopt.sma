/*
 * /opt0/bn/script/mailer/mailopt.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The option menu of the mailer (triangle), opened by main over a page: the items of the page, the disabled ones
 * dimmed; main reads the chosen one from g_iOptMenu.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "mailer.inc"

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

new Float:g_flTitleRed = 0.902;
new Float:g_flTitleGreen = 0.902;
new Float:g_flTitleBlue = 0.215;
new Float:g_flItemRed = 0.215;
new Float:g_flItemGreen = 0.708;
new Float:g_flItemBlue = 0.999;
new Float:g_flDisabledAlpha = 0.35;

/* Returns the bit of item id of the option menu, for the flags argument of mailopt. */
OptBit(id[])
{
    new index = GetItemIndex("OPT_ITEM", id);
    return 1 << index;
}

new g_iItemCount;
new g_szAnyId[1]; /* an empty id: the first element with the tag */
new g_szItemId[64];
new g_szIdAttr[] = "id";
new g_szItemText[256];

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
    domGetData(node, g_szIdAttr, g_szItemId, 64);
    g_szItemText[0] = 0;
    domGetData(node, _, g_szItemText, 256);
    if (0 == domFirstChild(node))
    {
        if (0 == strlen(g_szItemText))
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

public g_manager; /* the mailer page: holds the textures and the items to enable, gets "SHWT" when ready */
public g_focus; /* the focus object */
public g_active; /* the menu reads the pad */
public g_blink; /* unused, but part of the layout */
public g_iOptMenu; /* the selected item */
new g_objTitle;
new g_objBg;
new g_objSeparator;
new g_objItems[12];
new g_szItemLabels[12][16]; /* empty for the items the page does not have */
new g_bItemEnabled[12]; /* the item can be chosen */

main()
{
    new flags;
    new hist;
    new last;
    new value[32];
    new Float:pos[3];
    new Float:width;
    new Float:height;
    new Float:line_h;
    if (GetArg("flags", value, 32))
    {
        flags = atoi(value);
    }
    if (GetArg("hist", value, 32))
    {
        hist = atoi(value);
    }
    new ids[12][16];
    GetItemIds("OPT_ITEM", ids);
    for (new i; i < 12; i++)
    {
        copy_string(g_szItemLabels[i], "", 64);
        g_bItemEnabled[i] = 0;
        if (OptBit(ids[i]) & flags)
        {
            g_bItemEnabled[i] = 1;
            last = i;
            if (!strcmp("MAILER_OPT_REWRITE", ids[i]))
            {
                get_tag_attr("TEXT", "msgmailer_list_opt_reedit", "value", g_szItemLabels[i], 16);
            }
            else if (!strcmp("MAILER_OPT_LOCK", ids[i]))
            {
                get_tag_attr("TEXT", "msgmailer_list_opt_lock", "value", g_szItemLabels[i], 16);
                g_bItemEnabled[i] = GetPubVar(g_manager, "g_OptLock");
            }
            else if (!strcmp("MAILER_OPT_REPLY", ids[i]))
            {
                get_tag_attr("TEXT", "msgmailer_list_opt_reply", "value", g_szItemLabels[i], 16);
            }
            else if (!strcmp("MAILER_OPT_FWD", ids[i]))
            {
                get_tag_attr("TEXT", "msgmailer_list_opt_fwd", "value", g_szItemLabels[i], 16);
            }
            else if (!strcmp("MAILER_OPT_SEND", ids[i]))
            {
                get_tag_attr("TEXT", "msgmailer_list_opt_send", "value", g_szItemLabels[i], 16);
            }
            else if (!strcmp("MAILER_OPT_INFOMATION", ids[i]))
            {
                get_tag_attr("TEXT", "msgmailer_list_opt_info", "value", g_szItemLabels[i], 16);
            }
            else if (!strcmp("MAILER_OPT_LOUPE", ids[i]))
            {
                get_tag_attr("TEXT", "msgmailer_list_opt_loupe", "value", g_szItemLabels[i], 16);
            }
            else if (!strcmp("MAILER_OPT_DELE", ids[i]))
            {
                get_tag_attr("TEXT", "msgmailer_list_opt_delete", "value", g_szItemLabels[i], 16);
            }
            else if (!strcmp("MAILER_OPT_ADDR_ADD", ids[i]))
            {
                get_tag_attr("TEXT", "msgmailer_list_opt_addradd", "value", g_szItemLabels[i], 16);
                g_bItemEnabled[i] = GetPubVar(g_manager, "g_OptAddr");
            }
            else if (!strcmp("MAILER_OPT_ADDR_REF", ids[i]))
            {
                get_tag_attr("TEXT", "msgmailer_write_opt_addrref", "value", g_szItemLabels[i], 16);
            }
            else if (!strcmp("MAILER_OPT_ADDR_DELE", ids[i]))
            {
                get_tag_attr("TEXT", "msgmailer_addredit_opt_delete", "value", g_szItemLabels[i], 16);
                g_bItemEnabled[i] = mx_CountAddr();
            }
            else if (!strcmp("MAILER_OPT_ADDR_NEW", ids[i]))
            {
                get_tag_attr("TEXT", "msgmailer_addredit_opt_new", "value", g_szItemLabels[i], 16);
            }
        }
    }
    new bg_tex;
    get_group_tag_attr2("OPT", "HEAD", 0, "bg_x", value, 32);
    pos[0] = atof(value);
    get_group_tag_attr2("OPT", "HEAD", 0, "bg_y", value, 32);
    pos[1] = atof(value);
    get_group_tag_attr2("OPT", "HEAD", 0, "bg_z", value, 32);
    pos[2] = atof(value);
    bg_tex = GetPubVar(g_manager, "g_txBgOpt");
    width = GetPubVar(g_manager, "g_BgOptWidth");
    height = GetPubVar(g_manager, "g_BgOptHeight");
    g_objBg = CreatePlane(Self());
    SetColor(g_objBg, 1.0, 1.0, 1.0, 0.0);
    SetPos(g_objBg, pos[0], pos[1], pos[2], 0);
    SetScale(g_objBg, width, height, 0.0);
    SetTexture(g_objBg, bg_tex, 0, -1);
    SetOptionI(g_objBg, TAG("ZSRT"), ZSORT_OVERLAY);
    new separator_tex = GetPubVar(g_manager, "g_txSeparate");
    get_group_tag_attr2("OPT", "HEAD", 0, "item_x", value, 32);
    pos[0] = atof(value);
    get_group_tag_attr2("OPT", "HEAD", 0, "item_y", value, 32);
    pos[1] = atof(value);
    get_group_tag_attr2("OPT", "HEAD", 0, "item_z", value, 32);
    pos[2] = atof(value);
    get_group_tag_attr2("OPT", "HEAD", 0, "line_height", value, 32);
    line_h = atof(value);
    width = GetPubVar(g_manager, "g_SeparateWidth");
    height = GetPubVar(g_manager, "g_SeparateHeight");
    g_objSeparator = CreatePlane(Self());
    SetColor(g_objSeparator, 1.0, 1.0, 1.0, 0.0);
    SetPos(g_objSeparator, pos[0], pos[1] + 2 * line_h, pos[2], 0);
    SetScale(g_objSeparator, width, height, 0.0);
    SetTexture(g_objSeparator, separator_tex, 0, -1);
    SetOptionI(g_objSeparator, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionI(g_objSeparator, TAG("ALPH"), BLEND_ADD);
    Hide(g_objSeparator);
    {
        new i;
        new row;
        for (; i < 12; i++)
        {
            if (strlen(g_szItemLabels[i]))
            {
                g_objItems[i] = CreateLabel(Self());
                SetColor(g_objItems[i], g_flItemRed, g_flItemGreen, g_flItemBlue, 0.0);
                SetOptionS(g_objItems[i], TAG("UTF8"), g_szItemLabels[i]);
                SetOptionI(g_objItems[i], TAG("HALN"), ALIGN_CENTER);
                SetOptionI(g_objItems[i], TAG("VALN"), ALIGN_CENTER);
                SetOptionI(g_objItems[i], TAG("FSIZ"), 15);
                SetOptionI(g_objItems[i], TAG("ZSRT"), ZSORT_OVERLAY);
                SetPos(g_objItems[i], pos[0], pos[1] + line_h * row, pos[2], 0);
                row++;
                if (hist == SCREEN_ADDR && !strcmp("MAILER_OPT_ADDR_NEW", ids[i]))
                {
                    SetPos(g_objItems[i], pos[0], pos[1] + 3 * line_h, pos[2], 0);
                    Show(g_objSeparator);
                }
                sleep;
            }
        }
    }
    SetOptionI(g_focus, TAG("ZSRT"), ZSORT_OVERLAY);
    get_group_tag_attr2("OPT", "HEAD", 0, "folder_x", value, 32);
    pos[0] = atof(value);
    get_group_tag_attr2("OPT", "HEAD", 0, "folder_y", value, 32);
    pos[1] = atof(value);
    get_group_tag_attr2("OPT", "HEAD", 0, "folder_z", value, 32);
    pos[2] = atof(value);
    get_tag_attr("TEXT", "msgmailer_opt_title", "value", value, 32);
    g_objTitle = CreateLabel(Self());
    SetPos(g_objTitle, pos[0], pos[1], pos[2], 0);
    SetColor(g_objTitle, g_flTitleRed, g_flTitleGreen, g_flTitleBlue, 0.0);
    SetOptionI(g_objTitle, TAG("HALN"), ALIGN_CENTER);
    SetOptionI(g_objTitle, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(g_objTitle, TAG("FSIZ"), 17);
    SetOptionI(g_objTitle, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionS(g_objTitle, TAG("UTF8"), value);
    g_iOptMenu = 0;
    while (!g_bItemEnabled[g_iOptMenu])
    {
        g_iOptMenu++;
    }
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
            new item = g_iOptMenu;
            while (item > 0)
            {
                if (g_bItemEnabled[--item])
                {
                    g_iOptMenu = item;
                    sndSePlay(SE_CURSOR);
                    refocus = 1;
                    break;
                }
            }
        }
        else if (pressed & PAD_DOWN || (repeated & PAD_DOWN && tickcount() - last_tick > 70))
        {
            last_tick = tickcount();
            new item = g_iOptMenu;
            while (item < last)
            {
                if (g_bItemEnabled[++item])
                {
                    g_iOptMenu = item;
                    sndSePlay(SE_CURSOR);
                    refocus = 1;
                    break;
                }
            }
        }
        if (g_active && refocus)
        {
            send_label_focus2(g_focus, g_objItems[g_iOptMenu], 0, 0.5, 60.0, 60.0, 70.0, 1, 1);
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
                    get_tag_attr("TEXT", "msgmailer_btn_enter", "value", value, 32);
                    SetSystemParamStr(SYS_STR_PADHELP_RR, value);
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
            }
        }
        sleep;
    }
    return;
}

/* Fades the menu to alpha, the disabled items to a part of it. */
FadeAll(Float:alpha, Float:fade_time, Float:delay)
{
    new Float:color[4];
    for (new i; i < 12; i++)
    {
        if (strlen(g_szItemLabels[i]))
        {
            if (!g_bItemEnabled[i])
            {
                GetColor(g_objItems[i], color[0], color[1], color[2], color[3]);
                AnimColor(g_objItems[i], color[0], color[1], color[2], 1.0 * alpha * g_flDisabledAlpha, fade_time,
                        delay, ANIM_FUNC_LINEAR);
            }
            else
            {
                GetColor(g_objItems[i], color[0], color[1], color[2], color[3]);
                AnimColor(g_objItems[i], color[0], color[1], color[2], 1.0 * alpha, fade_time, delay, ANIM_FUNC_LINEAR);
            }
        }
    }
    GetColor(g_objTitle, color[0], color[1], color[2], color[3]);
    AnimColor(g_objTitle, color[0], color[1], color[2], alpha, fade_time, delay, ANIM_FUNC_LINEAR);
    GetColor(g_objBg, color[0], color[1], color[2], color[3]);
    AnimColor(g_objBg, color[0], color[1], color[2], alpha, fade_time, delay, ANIM_FUNC_LINEAR);
    GetColor(g_objSeparator, color[0], color[1], color[2], color[3]);
    AnimColor(g_objSeparator, color[0], color[1], color[2], 0.3 * alpha, fade_time, delay, ANIM_FUNC_LINEAR);
}

FadeIn()
{
    FadeAll(1.0, float(400), 0.0);
}

FadeOut()
{
    FadeAll(0.0, float(400), 0.0);
}

#include "mailsync.inc"
