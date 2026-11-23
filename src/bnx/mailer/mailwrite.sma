/*
 * /opt0/bn/script/mailer/mailwrite.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The write page of the mailer, opened by main: the recipients, the subject and the body, each edited with the
 * keyboard mailwriteskb, and the send button; it saves the mail to the queue or to the drafts for main.
 */

#include <core>
#include <bn>

#pragma dynamic 150000

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

/* Appends src to dest (maxlen cells), packed or unpacked like dest. */
append_string(dest[], src[], maxlen)
{
    new dest_len;
    new src_len;
    new i;
    new room;
    dest_len = strlen(dest);
    src_len = strlen(src);
    if (is_packed(dest))
    {
        room = maxlen * 4 - dest_len - 1;
        if (is_packed(src))
        {
            i = 0;
            for (; i < src_len && i < room; i++)
            {
                dest{dest_len + i} = src{i};
            }
        }
        else
        {
            i = 0;
            for (; i < src_len && i < room; i++)
            {
                dest{dest_len + i} = src[i];
            }
        }
        new pad = 4 - (dest_len + i) % 4;
        for (new j; j < pad; j++)
        {
            dest{dest_len + i + j} = 0;
        }
    }
    else
    {
        room = maxlen - dest_len - 1;
        if (is_packed(src))
        {
            i = 0;
            for (; i < src_len && i < room; i++)
            {
                dest[dest_len + i] = src{i};
            }
        }
        else
        {
            i = 0;
            for (; i < src_len && i < room; i++)
            {
                dest[dest_len + i] = src[i];
            }
        }
        dest[dest_len + i] = 0;
    }
    return;
}

#include "text.inc"

/* Copies src into dest (maxlen cells) with every occurrence of from replaced by to. */
replace_all(dest[], src[], from[], to[], maxlen)
{
    new count;
    new from_len = strlen(from);
    new to_len = strlen(to);
    new src_pos;
    new dest_pos;
    for (; ; )
    {
        new found = find_substring(src, from, src_pos);
        if (found < 0)
        {
            copy_substring(dest, dest_pos, src, src_pos, -1, maxlen);
            return count;
        }
        copy_substring(dest, dest_pos, src, src_pos, found - src_pos, maxlen);
        dest_pos = dest_pos + (found - src_pos);
        append_string(dest, to, maxlen);
        dest_pos = dest_pos + to_len;
        src_pos = found + from_len;
    }
    return 0;
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

/* Same as get_group_tag_attr of text.inc. */
get_group_tag_attr3(tag[], group[], index, attr[], value[], maxlen)
{
    new count;
    new dom_root = domRoot();
    for (new node = domFirstChild(dom_root, tag); node; node = domNext(node, tag))
    {
        new name[64];
        domGetData(node, "group", name, 64);
        if (0 == strcmp(name, group))
        {
            if (count == index)
            {
                new len = domGetData(node, attr, value, maxlen);
                if (0 == strcmp(attr, "src") || 0 == strcmp(attr, "href"))
                {
                    xmlMakeFullPath(value, maxlen);
                }
                return len;
            }
            count++;
        }
    }
    return 0;
}

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

new Float:g_flTextRed = 0.7843;
new Float:g_flTextGreen = 0.7843;
new Float:g_flTextBlue = 0.7843;
new Float:g_flTitleRed = 0.902;
new Float:g_flTitleGreen = 0.902;
new Float:g_flTitleBlue = 0.215;
new Float:g_flLabelRed = 0.215;
new Float:g_flDimAlpha = 0.2;
new Float:g_flBlinkOutTime = 1200.0;
new Float:g_flBlinkInTime = 600.0;
new g_iItemCount;
new g_szItemId[64];

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
new g_szAnyId[1]; /* an empty id: the first element with the tag */
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

#include "mailsync.inc"

public g_manager; /* the mailer page: holds the textures, gets "SHWT" when ready and "ACWT" after "ACT" */
public g_focus; /* the focus object */
public g_active; /* the page reads the pad */
public g_blink; /* the cursor blinks */
public g_iIndex; /* the selected item: recipients, subject, body or send button */
public g_revert; /* the mail was changed */
public g_space; /* the result of removing the draft being saved again: 0, ERR_NOSPACE or 1 */
public g_iSend; /* the item of the send button */
new g_objSkb;
new g_objBodyEdit;
new g_objTitle;
new g_objFieldBgs[5]; /* the backgrounds of the fields (WRITE_PLANE) */
new g_objItemLabels[2]; /* the names of the recipients and subject fields */
new g_objFieldTexts[2]; /* the recipients and the subject */
new g_objCursor;
new g_button_label;
new g_objButton;
new g_objButtonBase;
new g_objButtonWindow;
new g_objButtonGlow;
new g_objFadeTimer;
new g_bBlinkOn;

#include "pfs.inc"

#include "focus.inc"

main()
{
    new to[246];
    new subject[246];
    new body[32768];
    new text[32768];
    new mode;
    new old_mode;
    new line;
    new folder;
    g_iSend = GetItemIndex("WRITE_ITEM", "MAILER_WRITE_SEND");
    g_revert = 0;
    copy_string(to, "", 984);
    copy_string(subject, "", 984);
    copy_string(body, "", 131072);
    if (GetArg("send", text, 32768))
    {
        mode = atoi(text);
    }
    if (GetArg("old", text, 32768))
    {
        old_mode = atoi(text);
    }
    if (GetArg("folder", text, 32768))
    {
        folder = atoi(text);
    }
    if (GetArg("line", text, 32768))
    {
        line = atoi(text);
    }
    for (new i; i < 5; i++)
    {
        new x[10];
        new y[10];
        new h[10];
        new w[10];
        new red[10];
        new green[10];
        new blue[10];
        get_group_tag_attr3("WRITE", "WRITE_PLANE", i, "x", x, 10);
        get_group_tag_attr3("WRITE", "WRITE_PLANE", i, "y", y, 10);
        get_group_tag_attr3("WRITE", "WRITE_PLANE", i, "w", w, 10);
        get_group_tag_attr3("WRITE", "WRITE_PLANE", i, "h", h, 10);
        get_group_tag_attr3("WRITE", "WRITE_PLANE", i, "r", red, 10);
        get_group_tag_attr3("WRITE", "WRITE_PLANE", i, "g", green, 10);
        get_group_tag_attr3("WRITE", "WRITE_PLANE", i, "b", blue, 10);
        g_objFieldBgs[i] = CreatePlane(Self());
        SetPos(g_objFieldBgs[i], atof(x) + atof(w) / 2, atof(y) + atof(h) / 2, 0.0, 0);
        SetColor(g_objFieldBgs[i], atof(red), atof(green), atof(blue), 0.0);
        SetScale(g_objFieldBgs[i], atof(w), atof(h), 0.0);
        SetOptionI(g_objFieldBgs[i], TAG("ALPH"), BLEND_ADD);
    }
    {
        new tex = GetPubVar(g_manager, "g_txAddrEdit");
        g_objCursor = CreatePlane3x3(Self());
        SetColor(g_objCursor, 1.0, 1.0, 1.0, 0.0);
        SetTexture(g_objCursor, tex, 0, -1);
        SetOptionI(g_objCursor, TAG("ZSRT"), ZSORT_OVERLAY);
        SetOptionF(g_objCursor, TAG("ZOFS"), -1.0);
        SetOptionI(g_objCursor, TAG("ALPH"), BLEND_ADD);
    }
    {
        new title_mode = mode;
        if (mode == WRITE_ADD_ADDR || mode == WRITE_RETURN)
        {
            title_mode = old_mode;
        }
        if (WRITE_NEW == title_mode)
        {
            get_tag_attr("TEXT", "msgmailer_write_title", "value", text, 32768);
        }
        else if (title_mode == WRITE_REPLY)
        {
            get_tag_attr("TEXT", "msgmailer_write_title_reply", "value", text, 32768);
        }
        else if (title_mode == WRITE_FORWARD)
        {
            get_tag_attr("TEXT", "msgmailer_write_title_fwd", "value", text, 32768);
        }
        else if (title_mode == WRITE_REWRITE)
        {
            get_tag_attr("TEXT", "msgmailer_write_title_reedit", "value", text, 32768);
        }
        else if (title_mode == WRITE_FROM_ARGS)
        {
            get_tag_attr("TEXT", "msgmailer_write_title_reedit", "value", text, 32768);
        }
    }
    {
        new x[10];
        new y[10];
        new z[10];
        get_group_tag_attr3("WRITE", "HEAD", 0, "folder_x", x, 10);
        get_group_tag_attr3("WRITE", "HEAD", 0, "folder_y", y, 10);
        get_group_tag_attr3("WRITE", "HEAD", 0, "folder_z", z, 10);
        g_objTitle = CreateLabel(Self());
        SetPos(g_objTitle, atof(x), atof(y), atof(z), 0);
        SetColor(g_objTitle, g_flTitleRed, g_flTitleGreen, g_flTitleBlue, 0.0);
        SetOptionS(g_objTitle, TAG("UTF8"), text);
        SetOptionI(g_objTitle, TAG("HALN"), ALIGN_CENTER);
        SetOptionI(g_objTitle, TAG("VALN"), ALIGN_CENTER);
        SetOptionI(g_objTitle, TAG("FSIZ"), 17);
        SetOptionI(g_objTitle, TAG("ZSRT"), ZSORT_OVERLAY);
    }
    {
        new x[10];
        new y[10];
        new z[10];
        get_group_tag_attr3("WRITE", "HEAD", 0, "btn_x", x, 10);
        get_group_tag_attr3("WRITE", "HEAD", 0, "btn_y", y, 10);
        get_group_tag_attr3("WRITE", "HEAD", 0, "btn_z", z, 10);
        get_tag_attr("TEXT", "msgmailer_write_item_send", "value", text, 32768);
        g_button_label = CreateLabel(Self());
        SetPos(g_button_label, atof(x), atof(y), atof(z), 0);
        SetColor(g_button_label, g_flLabelRed, 0.708, 0.999, 0.0);
        SetOptionI(g_button_label, TAG("HALN"), ALIGN_CENTER);
        SetOptionI(g_button_label, TAG("VALN"), ALIGN_CENTER);
        SetOptionI(g_button_label, TAG("FSIZ"), 15);
        SetOptionI(g_button_label, TAG("ZSRT"), ZSORT_OVERLAY);
        SetOptionI(g_button_label, TAG("AP64"), 1 * 64);
        SetOptionI(g_button_label, TAG("KP64"), 1 * 64);
        SetOptionF(g_button_label, TAG("ZOFS"), 2.0);
        SetOptionS(g_button_label, TAG("UTF8"), text);
        g_objButton = create_button_white(Self(), 0.0, 78.0);
        SetTexture(g_objButton, imgGetResource(RES_BUTTON0), 0, -1);
        SetPos(g_objButton, atof(x), atof(y), 0.0, 0);
        g_objButtonBase = CreatePlane(Self());
        SetPos(g_objButtonBase, atof(x), atof(y), 0.0, 0);
        SetColor(g_objButtonBase, 1.0, 1.0, 1.0, 0.0);
        SetOptionI(g_objButtonBase, TAG("ZSRT"), ZSORT_OVERLAY);
        SetOptionF(g_objButtonBase, TAG("ZOFS"), -2.0);
        SetOptionI(g_objButtonBase, TAG("ALPH"), BLEND_KEEP);
        g_objButtonWindow = CreatePlane3x3(g_objButtonBase);
        SetColor(g_objButtonWindow, 0.0, 0.2353, 0.3922, 0.0);
        SetTexture(g_objButtonWindow, imgGetResource(RES_WINDOW1), 0, -1);
        SetOptionI(g_objButtonWindow, TAG("ZSRT"), ZSORT_OVERLAY);
        SetOptionF(g_objButtonWindow, TAG("ZOFS"), -1.0);
        SetOptionI(g_objButtonWindow, TAG("ALPH"), BLEND_ADD);
        new Float:tex_margins[4] = {0.5, 0.5, 0.5, 0.5};
        SetOptionP(g_objButtonWindow, TAG("MGNT"), tex_margins, 4);
        new Float:margins[4];
        margins[0] = margins[1] = 0.5 * 128.0 / 400.0;
        margins[2] = margins[3] = 0.5 * 128.0 / 260.0;
        SetOptionP(g_objButtonWindow, TAG("MGN"), margins, 4);
        g_objButtonGlow = CreatePlane(g_objButtonWindow);
        SetPos(g_objButtonGlow, 0.3, -0.2, 0.0, 0);
        SetScale(g_objButtonGlow, 2.0, 2.0, 1.0);
        SetColor(g_objButtonGlow, 0.0, 0.0, 0.0, 0.0);
        SetTexture(g_objButtonGlow, imgGetResource(RES_DOT), 0, -1);
        SetOptionI(g_objButtonGlow, TAG("ZSRT"), ZSORT_OVERLAY);
        SetOptionI(g_objButtonGlow, TAG("ALPH"), BLEND_ADD | BLEND_DEST_ALPHA);
    }
    {
        new Float:button_w;
        new Float:button_h;
        fit_button_to_label(button_w, button_h);
        SetScale(g_objButton, button_w, button_h, 1.0);
        SetLocalScale(g_objButtonBase, 2.0 * button_w, 2.0 * button_h, 1.0);
        SetScale(g_objButtonWindow, button_w - 5.0, button_h - 5.0, 1.0);
    }
    for (new i; i < 2; i++)
    {
        new x[10];
        new y[10];
        new h[10];
        new w[10];
        get_group_tag_attr3("WRITE", "WRITE_PLANE", i, "x", x, 10);
        get_group_tag_attr3("WRITE", "WRITE_PLANE", i, "y", y, 10);
        get_group_tag_attr3("WRITE", "WRITE_PLANE", i, "w", w, 10);
        get_group_tag_attr3("WRITE", "WRITE_PLANE", i, "h", h, 10);
        g_objItemLabels[i] = CreateLabel(Self());
        SetColor(g_objItemLabels[i], g_flLabelRed, 0.708, 0.999, 0.0);
        SetOptionI(g_objItemLabels[i], TAG("HALN"), ALIGN_CENTER);
        SetOptionI(g_objItemLabels[i], TAG("VALN"), ALIGN_CENTER);
        SetOptionI(g_objItemLabels[i], TAG("FSIZ"), 15);
        SetOptionI(g_objItemLabels[i], TAG("ZSRT"), ZSORT_OVERLAY);
        SetOptionI(g_objItemLabels[i], TAG("AP64"), 1 * 64);
        SetOptionI(g_objItemLabels[i], TAG("KP64"), 1 * 64);
        SetOptionF(g_objItemLabels[i], TAG("ZOFS"), 2.0);
        SetPos(g_objItemLabels[i], atof(x) + atof(w) / 2, 1.0 + (atof(y) + atof(h) / 2), 0.0, 0);
        if (i == GetItemIndex("WRITE_ITEM", "MAILER_WRITE_TO"))
        {
            get_tag_attr("TEXT", "msgmailer_write_item_to", "value", text, 32768);
        }
        else if (i == GetItemIndex("WRITE_ITEM", "MAILER_WRITE_SUBJECT"))
        {
            get_tag_attr("TEXT", "msgmailer_write_item_subject", "value", text, 32768);
        }
        SetOptionS(g_objItemLabels[i], TAG("UTF8"), text);
    }
    for (new i; i < 2; i++)
    {
        new x[10];
        new y[10];
        new h[10];
        new w[10];
        get_group_tag_attr3("WRITE", "WRITE_PLANE", i + 2, "x", x, 10);
        get_group_tag_attr3("WRITE", "WRITE_PLANE", i + 2, "y", y, 10);
        get_group_tag_attr3("WRITE", "WRITE_PLANE", i + 2, "w", w, 10);
        get_group_tag_attr3("WRITE", "WRITE_PLANE", i + 2, "h", h, 10);
        g_objFieldTexts[i] = CreateLabel(Self());
        SetPos(g_objFieldTexts[i], 12.0 + atof(x), 1.0 + (atof(y) + atof(h) / 2), 0.0, 0);
        SetColor(g_objFieldTexts[i], g_flTextRed, g_flTextGreen, g_flTextBlue, 0.0);
        SetOptionI(g_objFieldTexts[i], TAG("HALN"), ALIGN_LEFT);
        SetOptionI(g_objFieldTexts[i], TAG("VALN"), ALIGN_CENTER);
        SetOptionI(g_objFieldTexts[i], TAG("FSIZ"), 15);
        SetOptionI(g_objFieldTexts[i], TAG("ZSRT"), ZSORT_OVERLAY);
        SetOptionI(g_objFieldTexts[i], TAG("AP64"), 1 * 64);
        SetOptionI(g_objFieldTexts[i], TAG("KP64"), 1 * 64);
        SetOptionF(g_objFieldTexts[i], TAG("ZOFS"), 2.0);
        SetOptionI(g_objFieldTexts[i], TAG("W64 "), (atoi(w) - 16) * 64);
    }
    {
        new x[10];
        new y[10];
        get_group_tag_attr3("WRITE", "WRITE_PLANE", 4, "x", x, 10);
        get_group_tag_attr3("WRITE", "WRITE_PLANE", 4, "y", y, 10);
        g_objBodyEdit = CreateEditML_P(Self());
        SetPos(g_objBodyEdit, 8.0 + atof(x), 6.0 + atof(y), 0.0, 0);
        SetColor(g_objBodyEdit, g_flTextRed, g_flTextGreen, g_flTextBlue, 0.0);
        SetOptionF(g_objBodyEdit, TAG("ZOFS"), 2.0);
        SetOptionI(g_objBodyEdit, TAG("FSIZ"), 15);
        SetOptionI(g_objBodyEdit, TAG("SDWE"), 0);
        SetOptionI(g_objBodyEdit, TAG("CRSW"), 0);
        SetOptionI(g_objBodyEdit, TAG("EDTE"), 0);
        SetOptionI(g_objBodyEdit, TAG("LSPC"), 3);
        SetOptionI(g_objBodyEdit, TAG("W64 "), 548 * 64);
        SetOptionI(g_objBodyEdit, TAG("H64 "), 189 * 64);
        {
            new color[4];
            static bg_color[16];
            pack_color_bytes(color, bg_color);
            SetOptionP(g_objBodyEdit, TAG("BGC4"), color, 4);
            static text_color[16] = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};
            pack_color_bytes(color, text_color);
            SetOptionP(g_objBodyEdit, TAG("COLR"), color, 4);
        }
    }
    g_objFadeTimer = CreateNull(Self());
    FillMail(folder, line, mode);
    MoveCursor(g_iIndex);
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
            if (g_iIndex > 0)
            {
                sndSePlay(SE_CURSOR);
                g_iIndex = g_iIndex > 0 ? --g_iIndex : g_iIndex; /* redundant as written */
                MoveCursor(g_iIndex);
                SetBlink(g_iIndex, 1);
                SetActive(g_iIndex, 1);
                refocus = 1;
            }
        }
        else if (pressed & PAD_DOWN || (repeated & PAD_DOWN && tickcount() - last_tick > 70))
        {
            last_tick = tickcount();
            if (g_iIndex < 3)
            {
                sndSePlay(SE_CURSOR);
                g_iIndex = g_iIndex < 3 ? ++g_iIndex : g_iIndex; /* redundant as written */
                MoveCursor(g_iIndex);
                SetBlink(g_iIndex, 1);
                SetActive(g_iIndex, 1);
                refocus = 1;
            }
        }
        else if (pressed & PAD_CIRCLE)
        {
            if (g_iIndex == GetItemIndex("WRITE_ITEM", "MAILER_WRITE_TO"))
            {
                OpenKeyboard(g_iIndex, to);
            }
            else if (g_iIndex == GetItemIndex("WRITE_ITEM", "MAILER_WRITE_SUBJECT"))
            {
                OpenKeyboard(g_iIndex, subject);
            }
            else if (g_iIndex == GetItemIndex("WRITE_ITEM", "MAILER_WRITE_BODY"))
            {
                OpenKeyboard(g_iIndex, body);
            }
        }
        if (g_active && refocus)
        {
            if (g_iIndex == g_iSend)
            {
                send_focus(g_focus, g_objButton, 0, 0.5, 60.0, 60.0, 70.0, 1, 1);
            }
            else
            {
                send_focus(g_focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            }
            refocus = 0;
        }
        new msg;
        new what;
        new arg;
        new extra;
        /*
         * The actions of main ("ACT"), each answered with "ACWT": "BSET" sends the mail to main ("MTO", "MSUB",
         * "MBDY"), "SDCK" answers whether it has a recipient, "SEND" saves it to the queue and "DRFT" to the drafts
         * (in place of the draft it was opened from), answering its line and the error.
         */
        while (RecvMessage(msg, what, arg, extra))
        {
            if (msg == TAG("FOCS"))
            {
                if (what == TAG("IN"))
                {
                    SetActive(g_iIndex, 1);
                    refocus = 1;
                }
                else if (what == TAG("OUT"))
                {
                    SetActive(g_iIndex, 0);
                }
            }
            else if (msg == TAG("BLNK"))
            {
                if (what == TAG("IN"))
                {
                    SetBlink(g_iIndex, 1);
                }
                else if (what == TAG("OUT"))
                {
                    SetBlink(g_iIndex, 0);
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
                else if (what == TAG("DIM"))
                {
                    FadeDim();
                }
            }
            else if (msg == TAG("ACT"))
            {
                if (what == TAG("BSET"))
                {
                    SendMessageStr(g_manager, TAG("MTO"), to);
                    SendMessageStr(g_manager, TAG("MSUB"), subject);
                    SendMessageStr(g_manager, TAG("MBDY"), body);
                    SendMessage(g_manager, TAG("ACWT"), 0, 0, 0, 0);
                }
                else if (what == TAG("SDCK"))
                {
                    new has_to;
                    if (strlen(to))
                    {
                        has_to = 1;
                    }
                    SendMessage(g_manager, TAG("ACWT"), has_to, 0, 0, 0);
                }
                else if (what == TAG("SEND"))
                {
                    if (folder == FOLDER_DRAFT && old_mode == WRITE_REWRITE)
                    {
                        g_space = remove_message(folder, line);
                    }
                    new error;
                    new index = save_message(FOLDER_QUEUE, -1, to, "", "", subject, body, old_mode, folder, line,
                            error);
                    SendMessage(g_manager, TAG("ACWT"), index, error, 0, 0);
                }
                else if (what == TAG("DRFT"))
                {
                    if (folder == FOLDER_DRAFT && old_mode == WRITE_REWRITE)
                    {
                        g_space = remove_message(folder, line);
                    }
                    new error;
                    new index = save_message(FOLDER_DRAFT, -1, to, "", "", subject, body, old_mode, folder, line,
                            error);
                    SendMessage(g_manager, TAG("ACWT"), index, error, 0, 0);
                }
            }
        }
        /*
         * The text of the keyboard ("FNIS" accepted, "FNIB" dropped), and the fields: "FROM", "SUBJ", "BODY", and
         * "BYCR", a body from main that gets the signature.
         */
        while (RecvMessageStr(msg, text, 32768))
        {
            if (msg == TAG("FNIS"))
            {
                CloseKeyboard(g_iIndex, text, 1);
                refocus = 1;
            }
            else if (msg == TAG("FNIB"))
            {
                CloseKeyboard(g_iIndex, text, 0);
                refocus = 1;
            }
            else if (msg == TAG("FROM"))
            {
                copy_string(to, text, 984);
                new item = GetItemIndex("WRITE_ITEM", "MAILER_WRITE_TO");
                SetOptionS(g_objFieldTexts[item], TAG("UTF8"), text);
            }
            else if (msg == TAG("SUBJ"))
            {
                copy_string(subject, text, 984);
                new item = GetItemIndex("WRITE_ITEM", "MAILER_WRITE_SUBJECT");
                SetOptionS(g_objFieldTexts[item], TAG("UTF8"), text);
            }
            else if (msg == TAG("BODY"))
            {
                new shown[2048];
                copy_string(body, text, 131072);
                copy_string(shown, text, 2048);
                shown{8191} = 0;
                SetOptionS(g_objBodyEdit, TAG("UTF8"), shown);
            }
            else if (msg == TAG("BYCR"))
            {
                {
                    new signature[128];
                    GetSignature(signature);
                    mx_SetMsgBody(text);
                    read_quoted_body(-1, -1, text, 131072, "", "", signature);
                }
                new shown[2048];
                copy_string(body, text, 131072);
                copy_string(shown, text, 2048);
                shown{8191} = 0;
                SetOptionS(g_objBodyEdit, TAG("UTF8"), shown);
            }
        }
        if (g_blink)
        {
            if (ANIM_DONE == AnimGetState(g_objCursor, ANIM_COLOR))
            {
                new Float:color[4];
                new Float:alpha = 1.0;
                if (!g_bBlinkOn)
                {
                    alpha = 0.5;
                }
                GetColor(g_objCursor, color[0], color[1], color[2], color[3]);
                AnimColor(g_objCursor, color[0], color[1], color[2], alpha, g_flBlinkOutTime, 0.0, ANIM_FUNC_LINEAR);
                g_bBlinkOn = g_bBlinkOn ^ 1;
            }
        }
        sleep;
    }
    return;
}

/* Fades the page to alpha (the background of each field to alpha times its own "a"). */
FadeAll(Float:alpha, Float:fade_time, Float:delay)
{
    new Float:color[4];
    GetColor(g_objTitle, color[0], color[1], color[2], color[3]);
    AnimColor(g_objTitle, color[0], color[1], color[2], alpha, fade_time, delay, ANIM_FUNC_LINEAR);
    for (new i; i < 2; i++)
    {
        GetColor(g_objItemLabels[i], color[0], color[1], color[2], color[3]);
        AnimColor(g_objItemLabels[i], color[0], color[1], color[2], alpha, fade_time, delay, ANIM_FUNC_LINEAR);
        GetColor(g_objFieldTexts[i], color[0], color[1], color[2], color[3]);
        AnimColor(g_objFieldTexts[i], color[0], color[1], color[2], alpha, fade_time, delay, ANIM_FUNC_LINEAR);
    }
    for (new i; i < 5; i++)
    {
        new plane_alpha[10];
        get_group_tag_attr3("WRITE", "WRITE_PLANE", i, "a", plane_alpha, 10);
        GetColor(g_objFieldBgs[i], color[0], color[1], color[2], color[3]);
        AnimColor(g_objFieldBgs[i], color[0], color[1], color[2], alpha * atof(plane_alpha), fade_time, delay,
                ANIM_FUNC_LINEAR);
    }
    GetColor(g_button_label, color[0], color[1], color[2], color[3]);
    AnimColor(g_button_label, color[0], color[1], color[2], alpha, fade_time, delay, ANIM_FUNC_LINEAR);
    GetColor(g_objButton, color[0], color[1], color[2], color[3]);
    AnimColor(g_objButton, color[0], color[1], color[2], alpha, fade_time, delay, ANIM_FUNC_LINEAR);
    GetColor(g_objButtonBase, color[0], color[1], color[2], color[3]);
    AnimColor(g_objButtonBase, color[0], color[1], color[2], alpha, fade_time, delay, ANIM_FUNC_LINEAR);
    GetColor(g_objButtonWindow, color[0], color[1], color[2], color[3]);
    AnimColor(g_objButtonWindow, color[0], color[1], color[2], alpha, fade_time, delay, ANIM_FUNC_LINEAR);
    GetColor(g_objButtonGlow, color[0], color[1], color[2], color[3]);
    AnimColor(g_objButtonGlow, color[0], color[1], color[2], alpha, fade_time, delay, ANIM_FUNC_LINEAR);
    GetColor(g_objBodyEdit, color[0], color[1], color[2], color[3]);
    AnimColor(g_objBodyEdit, color[0], color[1], color[2], alpha, fade_time, delay, ANIM_FUNC_LINEAR);
    AnimTimerManager(g_objFadeTimer, fade_time, 0.0, ANIM_FUNC_LINEAR);
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

/* Lights the label of the send button when item is the button, otherwise moves the cursor to field item. */
MoveCursor(item)
{
    new Float:color[4];
    if (item == g_iSend)
    {
        if (ANIM_DONE != AnimGetState(g_objFadeTimer, ANIM_TIMER))
        {
            new Float:fade_time = float(400) * (1.0 - AnimDoTimer(g_objFadeTimer, ANIM_TIMER, 1.0));
            AnimColor(g_button_label, g_flTextRed, g_flTextGreen, g_flTextBlue, 1.0, fade_time, 0.0, ANIM_FUNC_LINEAR);
        }
        else
        {
            GetColor(g_button_label, color[0], color[1], color[2], color[3]);
            SetColor(g_button_label, g_flTextRed, g_flTextGreen, g_flTextBlue, color[3]);
        }
    }
    else
    {
        if (ANIM_DONE != AnimGetState(g_objFadeTimer, ANIM_TIMER))
        {
            new Float:fade_time = float(400) * (1.0 - AnimDoTimer(g_objFadeTimer, ANIM_TIMER, 1.0));
            AnimColor(g_button_label, g_flLabelRed, 0.708, 0.999, 1.0, fade_time, 0.0, ANIM_FUNC_LINEAR);
        }
        else
        {
            GetColor(g_button_label, color[0], color[1], color[2], color[3]);
            SetColor(g_button_label, g_flLabelRed, 0.708, 0.999, color[3]);
        }
        new x[10];
        new y[10];
        new w[10];
        new h[10];
        new Float:tex_w = GetPubVar(g_manager, "g_AddrEditWidth");
        new Float:tex_h = GetPubVar(g_manager, "g_AddrEditHeight");
        get_group_tag_attr3("WRITE", "WRITE_PLANE", item + 2, "x", x, 10);
        get_group_tag_attr3("WRITE", "WRITE_PLANE", item + 2, "y", y, 10);
        get_group_tag_attr3("WRITE", "WRITE_PLANE", item + 2, "w", w, 10);
        get_group_tag_attr3("WRITE", "WRITE_PLANE", item + 2, "h", h, 10);
        SetPos(g_objCursor, atof(x) + atof(w) / 2, atof(y) + atof(h) / 2, 0.0, 0);
        SetScale(g_objCursor, atof(w), atof(h), 0.0);
        new Float:margins[4];
        margins[0] = 5.0 / tex_w;
        margins[1] = 5.0 / tex_w;
        margins[2] = 5.0 / tex_h;
        margins[3] = 5.0 / tex_h;
        SetOptionP(g_objCursor, TAG("MGNT"), margins, 4);
        margins[0] = 5.0 / atof(w);
        margins[1] = 5.0 / atof(w);
        margins[2] = 5.0 / atof(h);
        margins[3] = 5.0 / atof(h);
        SetOptionP(g_objCursor, TAG("MGN"), margins, 4);
    }
}

/* Shows the cursor blinking on field item, or hides it. */
SetBlink(item, on)
{
    Hide(g_objCursor);
    if (on)
    {
        if (item != g_iSend)
        {
            Show(g_objCursor);
            g_blink = 1;
            g_bBlinkOn = 0;
            new Float:color[4];
            GetColor(g_objCursor, color[0], color[1], color[2], color[3]);
            if (color[3])
            {
                SetColor(g_objCursor, color[0], color[1], color[2], 0.5);
            }
            AnimColor(g_objCursor, color[0], color[1], color[2], 1.0, g_flBlinkInTime, 0.0, ANIM_FUNC_SLOWER);
        }
    }
    else
    {
        g_blink = 0;
    }
}

/* Takes the pad, with the pad help (the options only on the recipients), or leaves it and the focus. */
SetActive(item, active)
{
    new label[16];
    if (active)
    {
        get_tag_attr("TEXT", "msgmailer_btn_enter", "value", label, 16);
        SetSystemParamStr(SYS_STR_PADHELP_RR, label);
        get_tag_attr("TEXT", "msgmailer_btn_option", "value", label, 16);
        SetSystemParamStr(SYS_STR_PADHELP_RU, label);
        if (item == GetItemIndex("WRITE_ITEM", "MAILER_WRITE_TO"))
        {
            SetSystemParam(SYS_PADHELP, PADHELP_CIRCLE | PADHELP_CROSS | PADHELP_TRIANGLE);
        }
        else
        {
            SetSystemParam(SYS_PADHELP, PADHELP_CIRCLE | PADHELP_CROSS);
        }
        g_active = 1;
    }
    else
    {
        g_active = 0;
        send_focus(g_focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    }
}

/* Builds the signature of the preferences (SIG_TYPE) under its delimiter. */
GetSignature(signature[])
{
    new sig_type;
    new delim_text[3];
    new delim[3];
    get_tag_attr("TEXT", "msgmailer_write_item_signature_delimiter", "value", delim_text, 3);
    replace_all(delim, delim_text, "\\n", "\n", 3);
    mx_GetPref("SIG_TYPE", sig_type);
    if (SIGNATURE_NONE == sig_type)
    {
        copy_string(signature, "", 512);
    }
    else if (sig_type == SIGNATURE_ADDR)
    {
        new addr[32];
        mx_GetPrefStr("ADDR", addr, 128);
        sprintf(signature, "%s\n%s\n", delim, addr);
    }
    else if (sig_type == SIGNATURE_NAME_ADDR)
    {
        new name[32];
        new addr[32];
        mx_GetPrefStr("NAME", name, 128);
        mx_GetPrefStr("ADDR", addr, 128);
        sprintf(signature, "%s\n%s <%s>\n", delim, name, addr);
    }
    else if (sig_type == SIGNATURE_TEXT)
    {
        new sig_text[23];
        mx_GetPrefStr("SIG_TEXT", sig_text, 90);
        sprintf(signature, "%s\n%s\n", delim, sig_text);
    }
}

/*
 * Fills the fields for mode from mail line of folder: the subject and the body of a reply or of a forward, or
 * the mail edited again; a mail with an address from the address book counts as changed.
 */
FillMail(folder, line, mode)
{
    new text[32768];
    new reply_mark[3];
    new prefix[3];
    new signature[128];
    GetSignature(signature);
    get_tag_attr("TEXT", "msgmailer_write_item_reply_mark", "value", reply_mark, 3);
    if (WRITE_NEW == mode)
    {
        SendMessageStr(Self(), TAG("BODY"), signature);
    }
    else if (mode == WRITE_REPLY)
    {
        {
            new subject[246];
            get_tag_attr("TEXT", "msgmailer_write_item_reply_subject", "value", prefix, 3);
            mx_GetMsgSubject(folder, line, subject, 984 - strlen(prefix), 1);
            sprintf(text, "%s%s", prefix, subject);
        }
        SendMessageStr(Self(), TAG("SUBJ"), text);
        mx_GetMsgReplyTo(folder, line, text, 984);
        SendMessageStr(Self(), TAG("FROM"), text);
        copy_string(text, "", 131072);
        {
            new with_quote;
            mx_GetPref("WITH_QUOTE", with_quote);
            if (with_quote)
            {
                read_quoted_body(folder, line, text, 131072, reply_mark, "", signature);
            }
            else
            {
                sprintf(text, "%s", signature);
            }
        }
        SendMessageStr(Self(), TAG("BODY"), text);
    }
    else if (mode == WRITE_FORWARD)
    {
        {
            new subject[246];
            get_tag_attr("TEXT", "msgmailer_write_item_forward_subject", "value", prefix, 3);
            mx_GetMsgSubject(folder, line, subject, 984 - strlen(prefix));
            sprintf(text, "%s%s", prefix, subject);
        }
        SendMessageStr(Self(), TAG("SUBJ"), text);
        {
            new head[32];
            new tail[160];
            copy_string(text, "", 131072);
            get_tag_attr("TEXT", "msgmailer_write_item_forward_tag_start", "value", head, 32);
            sprintf(head, "%s\n", head);
            get_tag_attr("TEXT", "msgmailer_write_item_forward_tag_end", "value", tail, 160);
            sprintf(tail, "%s\n%s", tail, signature);
            read_quoted_body(folder, line, text, 131072, "", head, tail);
        }
        SendMessageStr(Self(), TAG("BODY"), text);
    }
    else if (mode == WRITE_REWRITE)
    {
        mx_GetMsgSubject(folder, line, text, 984);
        SendMessageStr(Self(), TAG("SUBJ"), text);
        if (FOLDER_INBOX == folder)
        {
            mx_GetMsgFrom(folder, line, text, 984);
        }
        else
        {
            mx_GetMsgTo(folder, line, text, 984);
        }
        SendMessageStr(Self(), TAG("FROM"), text);
        read_message_body(folder, line, text, 131072);
        SendMessageStr(Self(), TAG("BODY"), text);
    }
    else if (mode == WRITE_ADD_ADDR)
    {
        g_revert = 1;
    }
}

/* Opens the keyboard (mailwriteskb) on the text of item. */
OpenKeyboard(item, text[])
{
    SetActive(item, 0);
    SetBlink(item, 0);
    new args[16];
    g_objSkb = CreateNull(Self());
    SetPos(g_objSkb, 0.0, 0.0, 0.0, 0);
    SetOptionI(g_objSkb, TAG("ZSRT"), ZSORT_OVERLAY);
    if (item == GetItemIndex("WRITE_ITEM", "MAILER_WRITE_TO"))
    {
        sprintf(args, "mode=%d;input=%d;opt=%d;type=%d;limit=%d;line=%d;", SKB_KEYBOARD_QWERTY, SKB_INPUT_DIRECT, 0,
                SKB_LENGTH_BYTES, 984, item);
        AttachScript(g_objSkb, _, "mailwriteskb", args);
        SetPubVar(g_objSkb, "g_manager", Self());
        SetPubVar(g_objSkb, "g_manager2", g_manager);
        SendMessageStr(g_objSkb, TAG("SKB"), text);
    }
    else if (item == GetItemIndex("WRITE_ITEM", "MAILER_WRITE_SUBJECT"))
    {
        sprintf(args, "mode=%d;input=%d;opt=%d;type=%d;limit=%d;line=%d;", SKB_KEYBOARD_HIRAGANA, SKB_INPUT_DEFAULT, 0,
                SKB_LENGTH_BYTES, 984, item);
        AttachScript(g_objSkb, _, "mailwriteskb", args);
        SetPubVar(g_objSkb, "g_manager", Self());
        SetPubVar(g_objSkb, "g_manager2", g_manager);
        SendMessageStr(g_objSkb, TAG("SKB"), text);
    }
    else if (item == GetItemIndex("WRITE_ITEM", "MAILER_WRITE_BODY"))
    {
        sprintf(args, "mode=%d;input=%d;opt=%d;type=%d;limit=%d;line=%d;", SKB_KEYBOARD_HIRAGANA, SKB_INPUT_DEFAULT, 0,
                SKB_LENGTH_BYTES, 131072, item);
        AttachScript(g_objSkb, _, "mailwriteskb", args);
        SetPubVar(g_objSkb, "g_manager", Self());
        SetPubVar(g_objSkb, "g_manager2", g_manager);
        SendMessageStr(g_objSkb, TAG("INIT"), text);
    }
}

/* Closes the keyboard; the text, when it was accepted, goes to item and the mail counts as changed. */
CloseKeyboard(item, text[], accepted)
{
    if (accepted)
    {
        if (item == GetItemIndex("WRITE_ITEM", "MAILER_WRITE_TO"))
        {
            SendMessageStr(Self(), TAG("FROM"), text);
        }
        else if (item == GetItemIndex("WRITE_ITEM", "MAILER_WRITE_SUBJECT"))
        {
            SendMessageStr(Self(), TAG("SUBJ"), text);
        }
        else if (item == GetItemIndex("WRITE_ITEM", "MAILER_WRITE_BODY"))
        {
            SendMessageStr(Self(), TAG("BODY"), text);
        }
        g_revert = 1;
    }
    Destroy(g_objSkb);
    SetActive(item, 1);
    SetBlink(item, 1);
}
