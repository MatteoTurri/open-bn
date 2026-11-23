/*
 * /opt0/bn/script/mailer/mailinfo.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The information on the selected mail, opened by main from the option menu: its headers, its size and its lock,
 * as a text scrolled with up, down, L1 and R1.
 */

#include <core>
#include <bn>

#pragma dynamic 16000

#include "mailer.inc"

#include "tag.inc"

#include <float>

const PAGE_LINES = 14; /* the lines of the text shown at once */

/* Returns whether str is a packed string. */
is_packed(str[])
{
    return str[0] <= 0 || str[0] > 255;
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

new Float:g_flTextRed = 0.7843;
new Float:g_flTextGreen = 0.7843;
new Float:g_flTextBlue = 0.7843;
new Float:g_flTitleRed = 0.902;
new Float:g_flTitleGreen = 0.902;
new Float:g_flTitleBlue = 0.215;
new Float:g_flDimAlpha = 0.2;
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

public g_manager; /* the mailer page: gets "SHWT" when the page is ready */
public g_focus; /* unused, but part of the layout */
public g_active; /* the page reads the pad */
public g_blink; /* unused, but part of the layout */
public g_line; /* the mail is g_line of g_folder */
public g_top; /* unused, but part of the layout */
public g_folder; /* FOLDER_* */
new g_objText;
new g_objTextBg;
new g_objScrollBar;
new g_objTitle;
new g_iLineCount; /* the lines of the text, with the room past the last one */
new g_iScrollLine; /* the first line of the text shown */

main()
{
    new value[8];
    new Float:pos[3];
    new Float:color[3];
    new Float:width;
    new Float:height;
    if (GetArg("folder", value, 8))
    {
        g_folder = atoi(value);
    }
    if (GetArg("line", value, 8))
    {
        g_line = atoi(value);
    }
    get_group_tag_attr3("INFO", "HEAD", 0, "folder_x", value, 8);
    pos[0] = atof(value);
    get_group_tag_attr3("INFO", "HEAD", 0, "folder_y", value, 8);
    pos[1] = atof(value);
    get_group_tag_attr3("INFO", "HEAD", 0, "folder_z", value, 8);
    pos[2] = atof(value);
    get_tag_attr("TEXT", "msgmailer_info_title", "value", value, 8);
    g_objTitle = CreateLabel(Self());
    SetPos(g_objTitle, pos[0], pos[1], pos[2], 0);
    SetColor(g_objTitle, g_flTitleRed, g_flTitleGreen, g_flTitleBlue, 0.0);
    SetOptionS(g_objTitle, TAG("UTF8"), value);
    SetOptionI(g_objTitle, TAG("HALN"), ALIGN_CENTER);
    SetOptionI(g_objTitle, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(g_objTitle, TAG("FSIZ"), 17);
    SetOptionI(g_objTitle, TAG("ZSRT"), ZSORT_OVERLAY);
    get_group_tag_attr3("INFO", "HEAD", 0, "body_bg_x", value, 8);
    pos[0] = atof(value);
    get_group_tag_attr3("INFO", "HEAD", 0, "body_bg_y", value, 8);
    pos[1] = atof(value);
    get_group_tag_attr3("INFO", "HEAD", 0, "body_bg_z", value, 8);
    pos[2] = atof(value);
    get_group_tag_attr3("INFO", "HEAD", 0, "body_bg_r", value, 8);
    color[0] = atof(value);
    get_group_tag_attr3("INFO", "HEAD", 0, "body_bg_g", value, 8);
    color[1] = atof(value);
    get_group_tag_attr3("INFO", "HEAD", 0, "body_bg_b", value, 8);
    color[2] = atof(value);
    get_group_tag_attr3("INFO", "HEAD", 0, "body_bg_w", value, 8);
    width = atof(value);
    get_group_tag_attr3("INFO", "HEAD", 0, "body_bg_h", value, 8);
    height = atof(value);
    g_objTextBg = CreatePlane(Self());
    SetPos(g_objTextBg, pos[0], pos[1], pos[2], 0);
    SetScale(g_objTextBg, width, height, 0.0);
    SetColor(g_objTextBg, color[0], color[1], color[2], 0.0);
    SetOptionI(g_objTextBg, TAG("ALPH"), BLEND_ADD);
    get_group_tag_attr3("INFO", "HEAD", 0, "body_text_x", value, 8);
    pos[0] = atof(value);
    get_group_tag_attr3("INFO", "HEAD", 0, "body_text_y", value, 8);
    pos[1] = atof(value);
    get_group_tag_attr3("INFO", "HEAD", 0, "body_text_z", value, 8);
    pos[2] = atof(value);
    g_objText = CreateEditML_P(Self());
    SetPos(g_objText, pos[0], pos[1], pos[2], 0);
    SetColor(g_objText, g_flTextRed, g_flTextGreen, g_flTextBlue, 0.0);
    SetOptionF(g_objText, TAG("ZOFS"), 2.0);
    SetOptionI(g_objText, TAG("FSIZ"), 15);
    SetOptionI(g_objText, TAG("SDWE"), 0);
    SetOptionI(g_objText, TAG("CRSW"), 0);
    SetOptionI(g_objText, TAG("EDTE"), 0);
    SetOptionI(g_objText, TAG("LSPC"), 3);
    SetOptionI(g_objText, TAG("W64 "), 548 * 64);
    SetOptionI(g_objText, TAG("H64 "), 275 * 64);
    {
        new packed[4];
        static bg_color[16];
        pack_color_bytes(packed, bg_color);
        SetOptionP(g_objText, TAG("BGC4"), packed, 4);
        static text_color[16] = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};
        pack_color_bytes(packed, text_color);
        SetOptionP(g_objText, TAG("COLR"), packed, 4);
    }
    get_group_tag_attr3("INFO", "HEAD", 0, "bar_x", value, 8);
    pos[0] = atof(value);
    get_group_tag_attr3("INFO", "HEAD", 0, "bar_y", value, 8);
    pos[1] = atof(value);
    get_group_tag_attr3("INFO", "HEAD", 0, "bar_z", value, 8);
    pos[2] = atof(value);
    get_group_tag_attr3("INFO", "HEAD", 0, "bar_h", value, 8);
    height = atof(value);
    g_objScrollBar = CreateNull(Self());
    SetPos(g_objScrollBar, pos[0], pos[1], pos[2], 0);
    SetOptionI(g_objScrollBar, TAG("ZSRT"), ZSORT_OVERLAY);
    AttachScript(g_objScrollBar, _, "scroll_bar", "");
    SetPubVar(g_objScrollBar, "manager", Self());
    SetPubVar(g_objScrollBar, "focus", Self());
    SetPubVar(g_objScrollBar, "height", height);
    SendMessage(g_objScrollBar, TAG("FADE"), 0, 0, 0, 0);
    ShowInfo(g_folder, g_line);
    sleep;
    SendMessage(Self(), TAG("FADE"), TAG("IN"), 0, 0, 0);
    SendMessage(Self(), TAG("FOCS"), TAG("IN"), 0, 0, 0);
    SendMessage(g_manager, TAG("SHWT"), 0, 0, 0, 0);
    for (; ; )
    {
        if (g_iLineCount > PAGE_LINES + 1)
        {
            new held;
            new pressed;
            new released;
            new repeated;
            if (g_active)
            {
                PadState(held, pressed, released, repeated, -1);
            }
            static last_tick;
            if (pressed & PAD_L1 || (repeated & PAD_L1 && tickcount() - last_tick > 70))
            {
                last_tick = tickcount();
                SetOptionI(g_objText, TAG("SCLY"), -PAGE_LINES);
                if (g_iScrollLine > 0)
                {
                    sndSePlay(SE_CURSOR);
                    g_iScrollLine = g_iScrollLine - PAGE_LINES;
                    if (g_iScrollLine < 0)
                    {
                        g_iScrollLine = 0;
                    }
                    UpdateScrollBar();
                }
            }
            else if (pressed & PAD_R1 || (repeated & PAD_R1 && tickcount() - last_tick > 70))
            {
                last_tick = tickcount();
                SetOptionI(g_objText, TAG("SCLY"), PAGE_LINES);
                if (g_iScrollLine <= g_iLineCount - PAGE_LINES - 1)
                {
                    sndSePlay(SE_CURSOR);
                    g_iScrollLine = g_iScrollLine + PAGE_LINES;
                    if (g_iScrollLine > g_iLineCount - PAGE_LINES - 1)
                    {
                        g_iScrollLine = g_iLineCount - PAGE_LINES;
                    }
                    UpdateScrollBar();
                }
            }
            else if (pressed & PAD_UP || (repeated & PAD_UP && tickcount() - last_tick > 70))
            {
                last_tick = tickcount();
                SetOptionI(g_objText, TAG("SCLY"), -1);
                if (g_iScrollLine > 0)
                {
                    sndSePlay(SE_CURSOR);
                    g_iScrollLine--;
                    UpdateScrollBar();
                }
            }
            else if (pressed & PAD_DOWN || (repeated & PAD_DOWN && tickcount() - last_tick > 70))
            {
                last_tick = tickcount();
                SetOptionI(g_objText, TAG("SCLY"), 1);
                if (g_iScrollLine <= g_iLineCount - PAGE_LINES - 1)
                {
                    sndSePlay(SE_CURSOR);
                    g_iScrollLine++;
                    UpdateScrollBar();
                }
            }
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
                    SetSystemParam(SYS_PADHELP, PADHELP_CROSS);
                    g_active = 1;
                }
                else if (what == TAG("OUT"))
                {
                    g_active = 0;
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

/* Fades the page to alpha (the background of the text to alpha times its "body_bg_a"). */
FadeAll(Float:alpha, Float:fade_time, Float:delay)
{
    new Float:color[4];
    GetColor(g_objTitle, color[0], color[1], color[2], color[3]);
    AnimColor(g_objTitle, color[0], color[1], color[2], alpha, fade_time, delay, ANIM_FUNC_LINEAR);
    GetColor(g_objText, color[0], color[1], color[2], color[3]);
    AnimColor(g_objText, color[0], color[1], color[2], alpha, fade_time, delay, ANIM_FUNC_LINEAR);
    if (g_iLineCount > PAGE_LINES + 1)
    {
        SendMessage(g_objScrollBar, TAG("FADE"), alpha, fade_time, delay, 0);
    }
    new bg_alpha[10];
    get_group_tag_attr3("INFO", "HEAD", 0, "body_bg_a", bg_alpha, 10);
    GetColor(g_objTextBg, color[0], color[1], color[2], color[3]);
    AnimColor(g_objTextBg, color[0], color[1], color[2], alpha * atof(bg_alpha), fade_time, delay, ANIM_FUNC_LINEAR);
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

/* Sets the scroll bar to the part of the text shown. */
UpdateScrollBar()
{
    if (g_iLineCount > PAGE_LINES + 1)
    {
        new Float:length = float(PAGE_LINES) / float(g_iLineCount);
        length = length > 1.0 ? 1.0 : length;
        length = length < 0.0 ? 0.0 : length;
        length = length < 0.1 ? 0.1 : length;
        new Float:top = float(g_iScrollLine) / float(g_iLineCount - PAGE_LINES) * (1.0 - length);
        SetPubVar(g_objScrollBar, "top", top);
        SetPubVar(g_objScrollBar, "length", length);
    }
    else
    {
        SetPubVar(g_objScrollBar, "top", 0);
        SetPubVar(g_objScrollBar, "length", 0);
    }
}

/*
 * Shows the information on mail index of folder: recipients, copies, sender, subject, reply address, mailer,
 * date, size and lock; then counts the lines of the text, with a page of room past the last one.
 */
ShowInfo(folder, index)
{
    new info[2460];
    new field[246];
    new label[16];
    mx_GetMsgTo(folder, index, field, 984, 3);
    get_tag_attr("TEXT", "msgmailer_info_to", "value", label, 16);
    if (strlen(field))
    {
        sprintf(info, "%s\n%s\n\n", label, field);
    }
    else
    {
        sprintf(info, "%s\n\n", label);
    }
    mx_GetMsgCc(folder, index, field, 984, 3);
    get_tag_attr("TEXT", "msgmailer_info_cc", "value", label, 16);
    if (strlen(field))
    {
        sprintf(info, "%s%s\n%s\n\n", info, label, field);
    }
    else
    {
        sprintf(info, "%s%s\n\n", info, label);
    }
    mx_GetMsgFrom(folder, index, field, 984, 3);
    get_tag_attr("TEXT", "msgmailer_info_from", "value", label, 16);
    if (strlen(field))
    {
        sprintf(info, "%s%s\n%s\n\n", info, label, field);
    }
    else
    {
        sprintf(info, "%s%s\n\n", info, label);
    }
    mx_GetMsgSubject(folder, index, field, 984);
    get_tag_attr("TEXT", "msgmailer_info_subject", "value", label, 16);
    if (strlen(field))
    {
        sprintf(info, "%s%s\n%s\n\n", info, label, field);
    }
    else
    {
        sprintf(info, "%s%s\n\n", info, label);
    }
    mx_GetMsgReplyTo(folder, index, field, 984, 1, 3);
    get_tag_attr("TEXT", "msgmailer_info_replyto", "value", label, 16);
    if (strlen(field))
    {
        sprintf(info, "%s%s\n%s\n\n", info, label, field);
    }
    else
    {
        sprintf(info, "%s%s\n\n", info, label);
    }
    mx_GetMsgXMailer(folder, index, field, 984);
    get_tag_attr("TEXT", "msgmailer_info_xmailer", "value", label, 16);
    if (strlen(field))
    {
        sprintf(info, "%s%s\n%s\n\n", info, label, field);
    }
    else
    {
        sprintf(info, "%s%s\n\n", info, label);
    }
    mx_GetMsgDate(folder, index, field, 100);
    if (strlen(field))
    {
        if (FOLDER_INBOX == folder)
        {
            get_tag_attr("TEXT", "msgmailer_info_date_inbox", "value", label, 16);
        }
        else if (folder == FOLDER_OUTBOX)
        {
            get_tag_attr("TEXT", "msgmailer_info_date_outbox", "value", label, 16);
        }
        else if (folder == FOLDER_DRAFT)
        {
            get_tag_attr("TEXT", "msgmailer_info_date_draft", "value", label, 16);
        }
        else if (folder == FOLDER_QUEUE)
        {
            get_tag_attr("TEXT", "msgmailer_info_date_queue", "value", label, 16);
        }
        sprintf(info, "%s%s\n %s\n\n", info, label, field);
    }
    get_tag_attr("TEXT", "msgmailer_info_size", "value", field, 246);
    sprintf(info, "%s%s\n", info, field);
    new size_text[8];
    new size_fmt[8];
    new size[8];
    get_tag_attr("TEXT", "msgmailer_info_size_byte", "value", size_text, 8);
    replace_all(size_fmt, size_text, "%MAILSIZE", "%d", 8);
    sprintf(size, size_fmt, mx_GetMsgSize(folder, index));
    sprintf(info, "%s%s\n\n", info, size);
    get_tag_attr("TEXT", "msgmailer_info_lock", "value", field, 246);
    sprintf(info, "%s%s\n", info, field);
    if (IsLocked(folder, index))
    {
        get_tag_attr("TEXT", "msgmailer_info_lock_on", "value", field, 246);
    }
    else
    {
        get_tag_attr("TEXT", "msgmailer_info_lock_off", "value", field, 246);
    }
    sprintf(info, "%s%s\n\n", info, field);
    SetOptionS(g_objText, TAG("UTF8"), info);
    g_iScrollLine = 0;
    SetOptionI(g_objText, TAG("CPDY"), 0);
    GetOptionI(g_objText, TAG("CPDM"), g_iLineCount);
    if (g_iLineCount > PAGE_LINES + 1)
    {
        g_iLineCount = g_iLineCount + PAGE_LINES - 1;
    }
    UpdateScrollBar();
}

IsLocked(folder, index)
{
    new lock_text[3];
    mx_GetMsgLock(folder, index, lock_text, 10);
    return atoi(lock_text);
}

#include "mailsync.inc"
