/*
 * /opt0/bn/script/mailer/mailseditrecept.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The receiving settings of the mailer (when to check for new mail, the sound on new mail, the largest
 * message received) as a list under a title (argument "top"); the settings page chooses a new value
 * and reattaches the script with the arguments "edit" (the item) and "parts" (the choice).
 */

#include <core>
#include <bn>

#pragma dynamic 3000

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

#include "misc.inc"

new Float:g_flTextRed = 0.7843;
new Float:g_flTextGreen = 0.7843;
new Float:g_flTextBlue = 0.7843;
new Float:g_flTitleRed = 0.902;
new Float:g_flTitleGreen = 0.902;
new Float:g_flTitleBlue = 0.215;
new Float:g_flLabelRed = 0.215;
new Float:g_flLabelGreen = 0.708;
new Float:g_flLabelBlue = 0.999;
new Float:g_flDimAlpha = 0.2;
new Float:g_flDisabledAlpha = 0.35;
new g_iSizeSteps[7] = {8, 16, 32, 64, 128, 256, 512}; /* the choices of the largest message, in KB */
new g_iCheckOrder[3] = {2, 1}; /* the choice of the check list for each CHECK value, and back */
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

public g_manager; /* the settings page: gets "SHWT" when the list is ready and "ACWT" after "ACT" */
public g_focus; /* the focus object, moved onto the selected item */
public g_active; /* 1 while the list has the focus and takes the pad */
public g_iSetMenu; /* the selected item */
public g_chkskb; /* 0: the circle button is left to the settings page */
new g_objTitle;
new g_objLabels[3];
new g_objValues[3];
new g_szItemIds[3][16]; /* the ids of the items, from SEDIT_RECEPT_ITEM */
new g_iCheck; /* the CHECK preference */
new g_iInterval[1]; /* the INTERVAL preference, the minutes between checks (3 to 60) */
new g_iSound; /* the choice of the sound: SOUND inverted, 0 on */
new g_iSizeLimit; /* the SIZE preference, in KB */
new g_bEnabled[3]; /* the items that can be selected */

main()
{
    new Float:label_pos[3];
    new Float:value_pos[3];
    new Float:line_h;
    new last_item;
    new text[64];
    g_chkskb = 0;
    get_group_tag_attr3("SEDIT_RECEPT", "HEAD", 0, "folder_x", text, 64);
    label_pos[0] = atof(text);
    get_group_tag_attr3("SEDIT_RECEPT", "HEAD", 0, "folder_y", text, 64);
    label_pos[1] = atof(text);
    get_group_tag_attr3("SEDIT_RECEPT", "HEAD", 0, "folder_z", text, 64);
    label_pos[2] = atof(text);
    GetArg("top", text, 64);
    g_objTitle = CreateLabel(Self());
    SetColor(g_objTitle, g_flTitleRed, g_flTitleGreen, g_flTitleBlue, 0.0);
    SetPos(g_objTitle, label_pos[0], label_pos[1], label_pos[2], 0);
    SetOptionS(g_objTitle, TAG("UTF8"), text);
    SetOptionI(g_objTitle, TAG("HALN"), ALIGN_CENTER);
    SetOptionI(g_objTitle, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(g_objTitle, TAG("FSIZ"), 17);
    SetOptionI(g_objTitle, TAG("ZSRT"), ZSORT_OVERLAY);
    GetItemIds("SEDIT_RECEPT_ITEM", g_szItemIds);
    get_group_tag_attr3("SEDIT_RECEPT", "HEAD", 0, "item_x", text, 64);
    label_pos[0] = atof(text);
    get_group_tag_attr3("SEDIT_RECEPT", "HEAD", 0, "item_y", text, 64);
    label_pos[1] = atof(text);
    get_group_tag_attr3("SEDIT_RECEPT", "HEAD", 0, "item_z", text, 64);
    label_pos[2] = atof(text);
    get_group_tag_attr3("SEDIT_RECEPT", "HEAD", 0, "info_x", text, 64);
    value_pos[0] = atof(text);
    get_group_tag_attr3("SEDIT_RECEPT", "HEAD", 0, "info_y", text, 64);
    value_pos[1] = atof(text);
    get_group_tag_attr3("SEDIT_RECEPT", "HEAD", 0, "info_z", text, 64);
    value_pos[2] = atof(text);
    get_group_tag_attr3("SEDIT_RECEPT", "HEAD", 0, "line_height", text, 64);
    line_h = atof(text);
    for (new i; i < 3; i++)
    {
        g_bEnabled[i] = 1;
        last_item = i;
        if (!strcmp(g_szItemIds[i], "MAILER_SEDIT_RECEPT_CHECK"))
        {
            if (GetArg("edit", text, 64) && i == atoi(text))
            {
                GetArg("parts", text, 64);
                g_iCheck = g_iCheckOrder[atoi(text)];
                mx_SetPref("CHECK", g_iCheck);
                mx_Restart();
            }
            else
            {
                mx_GetPref("CHECK", g_iCheck);
            }
            mx_GetPref("INTERVAL", g_iInterval);
        }
        else if (!strcmp(g_szItemIds[i], "MAILER_SEDIT_RECEPT_SOUND"))
        {
            if (GetArg("edit", text, 64) && i == atoi(text))
            {
                GetArg("parts", text, 64);
                g_iSound = atoi(text);
                mx_SetPref("SOUND", g_iSound ^ 1);
                mx_Restart();
            }
            else
            {
                mx_GetPref("SOUND", g_iSound);
                g_iSound = g_iSound ^ 1;
            }
        }
        else if (!strcmp(g_szItemIds[i], "MAILER_SEDIT_RECEPT_SIZE"))
        {
            if (GetArg("edit", text, 64) && i == atoi(text))
            {
                GetArg("parts", text, 64);
                g_iSizeLimit = atoi(text);
                mx_SetPref("SIZE", g_iSizeLimit);
                mx_Restart();
            }
            else
            {
                mx_GetPref("SIZE", g_iSizeLimit);
            }
        }
        new label[64];
        new value[64];
        GetItemTexts(i, label, value);
        g_objLabels[i] = CreateLabel(Self());
        SetPos(g_objLabels[i], label_pos[0], label_pos[1] + i * line_h, label_pos[2], 0);
        SetColor(g_objLabels[i], g_flLabelRed, g_flLabelGreen, g_flLabelBlue, 0.0);
        SetOptionS(g_objLabels[i], TAG("UTF8"), label);
        SetOptionI(g_objLabels[i], TAG("HALN"), ALIGN_RIGHT);
        SetOptionI(g_objLabels[i], TAG("VALN"), ALIGN_CENTER);
        SetOptionI(g_objLabels[i], TAG("FSIZ"), 15);
        SetOptionI(g_objLabels[i], TAG("ZSRT"), ZSORT_OVERLAY);
        g_objValues[i] = CreateLabel(Self());
        SetPos(g_objValues[i], value_pos[0], value_pos[1] + i * line_h, value_pos[2], 0);
        SetColor(g_objValues[i], g_flTextRed, g_flTextGreen, g_flTextBlue, 0.0);
        SetOptionS(g_objValues[i], TAG("UTF8"), value);
        SetOptionI(g_objValues[i], TAG("HALN"), ALIGN_LEFT);
        SetOptionI(g_objValues[i], TAG("VALN"), ALIGN_CENTER);
        SetOptionI(g_objValues[i], TAG("FSIZ"), 15);
        SetOptionI(g_objValues[i], TAG("ZSRT"), ZSORT_OVERLAY);
    }
    UpdateSoundItem();
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
            new item = g_iSetMenu;
            while (item > 0)
            {
                if (g_bEnabled[--item])
                {
                    g_iSetMenu = item;
                    sndSePlay(SE_CURSOR);
                    refocus = 1;
                    break;
                }
            }
        }
        else if (pressed & PAD_DOWN || (repeated & PAD_DOWN && tickcount() - last_repeat > 70))
        {
            last_repeat = tickcount();
            new item = g_iSetMenu;
            while (item < last_item)
            {
                if (g_bEnabled[++item])
                {
                    g_iSetMenu = item;
                    sndSePlay(SE_CURSOR);
                    refocus = 1;
                    break;
                }
            }
        }
        if (g_active && refocus)
        {
            send_label_focus2(g_focus, g_objLabels[g_iSetMenu], 0, 0.5, 60.0, 60.0, 70.0, 1, 1);
            refocus = 0;
        }
        new msg;
        new what;
        new arg;
        /* "FOCS" IN/OUT, "FADE" IN/OUT/DIM; "ACT" "PART" gets "ACWT" with the arguments of the choices of the item. */
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
                if (what == TAG("IN"))
                {
                }
                else if (what == TAG("OUT"))
                {
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
                if (what == TAG("PART"))
                {
                    if (!strcmp(g_szItemIds[g_iSetMenu], "MAILER_SEDIT_RECEPT_CHECK"))
                    {
                        GetCheckArgs(text);
                    }
                    else if (!strcmp(g_szItemIds[g_iSetMenu], "MAILER_SEDIT_RECEPT_SOUND"))
                    {
                        GetSoundArgs(text);
                    }
                    else if (!strcmp(g_szItemIds[g_iSetMenu], "MAILER_SEDIT_RECEPT_SIZE"))
                    {
                        new unit[16];
                        get_tag_attr("TEXT", "msgmailer_set_item_recv_size_kb", "value", text, 64);
                        replace_all(unit, text, "%MAILSIZE", "", 16);
                        GetSizeArgs(text, 4, unit);
                    }
                    SendMessageStr(g_manager, TAG("ACWT"), text);
                }
            }
        }
        sleep;
    }
    return;
}

/* Fades every label of the list to alpha, the disabled items dimmer. */
FadeItems(Float:alpha, Float:duration, Float:delay)
{
    new Float:color[4];
    for (new i; i < 3; i++)
    {
        new Float:item_alpha = alpha;
        if (!g_bEnabled[i])
        {
            item_alpha = item_alpha * g_flDisabledAlpha;
        }
        GetColor(g_objLabels[i], color[0], color[1], color[2], color[3]);
        AnimColor(g_objLabels[i], color[0], color[1], color[2], item_alpha, duration, delay, ANIM_FUNC_LINEAR);
        GetColor(g_objValues[i], color[0], color[1], color[2], color[3]);
        AnimColor(g_objValues[i], color[0], color[1], color[2], item_alpha, duration, delay, ANIM_FUNC_LINEAR);
    }
    GetColor(g_objTitle, color[0], color[1], color[2], color[3]);
    AnimColor(g_objTitle, color[0], color[1], color[2], alpha, duration, delay, ANIM_FUNC_LINEAR);
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

/* Disables the sound item when mail is never checked. */
UpdateSoundItem()
{
    new check_ids[3][16];
    GetItemIds("SEDIT_RECEPT_CHECK_ITEM", check_ids);
    new sound_item = GetItemIndex("SEDIT_RECEPT_ITEM", "MAILER_SEDIT_RECEPT_SOUND");
    if (!strcmp(check_ids[g_iCheckOrder[g_iCheck]], "MAILER_SEDIT_RECEPT_NONE"))
    {
        g_bEnabled[sound_item] = 0;
    }
    else
    {
        g_bEnabled[sound_item] = 1;
    }
}

/* Gets the caption of item index and the text of its current value. */
GetItemTexts(index, label[], value[])
{
    new label_text[64];
    new value_text[64];
    new yes_no_ids[2][16];
    GetItemIds("SEDIT_COMMON_ITEM", yes_no_ids);
    if (!strcmp(g_szItemIds[index], "MAILER_SEDIT_RECEPT_CHECK"))
    {
        get_tag_attr("TEXT", "msgmailer_set_item_recv_check", "value", label_text, 64);
        new check_ids[3][16];
        GetItemIds("SEDIT_RECEPT_CHECK_ITEM", check_ids);
        if (!strcmp(check_ids[g_iCheckOrder[g_iCheck]], "MAILER_SEDIT_RECEPT_INTERVAL"))
        {
            new interval_label[8];
            new interval_format[8];
            new format[8];
            new interval_text[8];
            get_tag_attr("TEXT", "msgmailer_set_item_recv_check_interval", "value", interval_label, 8);
            get_tag_attr("TEXT", "msgmailer_set_item_recv_check_interval_2", "value", interval_format, 8);
            replace_all(format, interval_format, "%CHKINTV", "%d", 8);
            sprintf(interval_text, format, g_iInterval);
            sprintf(value_text, "%s %s", interval_label, interval_text);
        }
        else if (!strcmp(check_ids[g_iCheckOrder[g_iCheck]], "MAILER_SEDIT_RECEPT_START"))
        {
            get_tag_attr("TEXT", "msgmailer_set_item_recv_check_start", "value", value_text, 64);
        }
        else if (!strcmp(check_ids[g_iCheckOrder[g_iCheck]], "MAILER_SEDIT_RECEPT_NONE"))
        {
            get_tag_attr("TEXT", "msgmailer_set_item_recv_check_none", "value", value_text, 64);
        }
    }
    else if (!strcmp(g_szItemIds[index], "MAILER_SEDIT_RECEPT_SOUND"))
    {
        get_tag_attr("TEXT", "msgmailer_set_item_recv_check_sound", "value", label_text, 64);
        if (!strcmp(yes_no_ids[g_iSound], "MAILER_SEDIT_YES"))
        {
            get_tag_attr("TEXT", "msgmailer_set_item_recv_check_sound_yes", "value", value_text, 64);
        }
        else if (!strcmp(yes_no_ids[g_iSound], "MAILER_SEDIT_NO"))
        {
            get_tag_attr("TEXT", "msgmailer_set_item_recv_check_sound_no", "value", value_text, 64);
        }
    }
    else if (!strcmp(g_szItemIds[index], "MAILER_SEDIT_RECEPT_SIZE"))
    {
        get_tag_attr("TEXT", "msgmailer_set_item_recv_size", "value", label_text, 64);
        new size_format[16];
        new format[16];
        get_tag_attr("TEXT", "msgmailer_set_item_recv_size_kb", "value", size_format, 16);
        replace_all(format, size_format, "%MAILSIZE", "%d", 16);
        sprintf(value_text, format, g_iSizeLimit);
    }
    copy_string(label, label_text, 256);
    copy_string(value, value_text, 256);
}

/* Writes in args the arguments of the list of check choices. */
GetCheckArgs(args[])
{
    new label[32];
    new choices[3][32];
    new item = GetItemIndex("SEDIT_RECEPT_ITEM", "MAILER_SEDIT_RECEPT_CHECK");
    new interval_index = GetItemIndex("SEDIT_RECEPT_CHECK_ITEM", "MAILER_SEDIT_RECEPT_INTERVAL");
    {
        new saved = g_iCheck;
        g_iCheck = 0;
        for (new i; i < 3; i++)
        {
            GetItemTexts(item, label, choices[g_iCheckOrder[i]]);
            g_iCheck++;
        }
        g_iCheck = saved;
    }
    sprintf(args, "btn0=%s;btn1=%s;btn2=%s;label=%s;sel=%d;next=%d", choices[0], choices[1], choices[2], label,
            g_iCheckOrder[g_iCheck], interval_index);
}

/* Writes in args the arguments of the yes/no choice on the sound. */
GetSoundArgs(args[])
{
    new label[32];
    new choices[10][32];
    new item = GetItemIndex("SEDIT_RECEPT_ITEM", "MAILER_SEDIT_RECEPT_SOUND");
    {
        new saved = g_iSound;
        g_iSound = 0;
        for (new i; i < 2; i++)
        {
            GetItemTexts(item, label, choices[i]);
            g_iSound++;
        }
        g_iSound = saved;
    }
    sprintf(args, "btn0=%s;btn1=%s;label=%s;sel=%d;", choices[0], choices[1], label, g_iSound);
}

/* Writes in args the arguments of the spin of the largest message size, suffix after the number. */
GetSizeArgs(args[], speed, suffix[])
{
    new label[64];
    new value[64];
    new sel;
    new item = GetItemIndex("SEDIT_RECEPT_ITEM", "MAILER_SEDIT_RECEPT_SIZE");
    GetItemTexts(item, label, value);
    for (new i; i < 7; i++)
    {
        if (g_iSizeSteps[i] == g_iSizeLimit)
        {
            sel = i;
            break;
        }
    }
    sprintf(args, "max=%d;min=%d;spd=%d;label=%s;end=%s;sel=%d;flg=%d;", 6, 0, speed, label, suffix, sel, 1);
}

#include "mailsync.inc"
#include "mailline.inc"
