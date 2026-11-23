/*
 * /opt0/bn/script/mailer/mailseditfound.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The basic settings of the mailer (name, address, SMTP and POP3 servers, user and password) as a list
 * under a title (argument "top"); the circle button edits the selected one with mailsetskb and saves it
 * in the mailer preferences.
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

#include "focus.inc"

public g_manager; /* the settings page: gets "SHWT" when the list is ready and "ACWT" after "ACT" */
public g_focus; /* the focus object, moved onto the selected item */
public g_active; /* 1 while the list has the focus and takes the pad */
public g_setchk; /* set by "ACT" "SCHK": 1 if a setting other than the name is empty */
public g_iSetMenu; /* the selected item */
public g_chkskb; /* 1: the circle button is taken here, where every item is edited with mailsetskb */
new g_objTitle;
new g_objLabels[6];
new g_objValues[6];
new g_objKeyboard; /* the mailsetskb object of the edit in progress */
new g_szItemIds[6][16]; /* the ids of the items, from SEDIT_FOUND_ITEM */
new g_szItemNames[6][32];
new g_szValues[6][32]; /* the values of the settings */

main()
{
    new Float:label_pos[3];
    new Float:value_pos[3];
    new Float:line_h;
    new text[32];
    g_chkskb = 1;
    get_group_tag_attr2("SEDIT_FOUND", "HEAD", 0, "folder_x", text, 32);
    label_pos[0] = atof(text);
    get_group_tag_attr2("SEDIT_FOUND", "HEAD", 0, "folder_y", text, 32);
    label_pos[1] = atof(text);
    get_group_tag_attr2("SEDIT_FOUND", "HEAD", 0, "folder_z", text, 32);
    label_pos[2] = atof(text);
    GetArg("top", text, 32);
    g_objTitle = CreateLabel(Self());
    SetColor(g_objTitle, g_flTitleRed, g_flTitleGreen, g_flTitleBlue, 0.0);
    SetPos(g_objTitle, label_pos[0], label_pos[1], label_pos[2], 0);
    SetOptionS(g_objTitle, TAG("UTF8"), text);
    SetOptionI(g_objTitle, TAG("HALN"), ALIGN_CENTER);
    SetOptionI(g_objTitle, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(g_objTitle, TAG("FSIZ"), 17);
    SetOptionI(g_objTitle, TAG("ZSRT"), ZSORT_OVERLAY);
    GetItemIds("SEDIT_FOUND_ITEM", g_szItemIds);
    get_group_tag_attr2("SEDIT_FOUND", "HEAD", 0, "item_x", text, 32);
    label_pos[0] = atof(text);
    get_group_tag_attr2("SEDIT_FOUND", "HEAD", 0, "item_y", text, 32);
    label_pos[1] = atof(text);
    get_group_tag_attr2("SEDIT_FOUND", "HEAD", 0, "item_z", text, 32);
    label_pos[2] = atof(text);
    get_group_tag_attr2("SEDIT_FOUND", "HEAD", 0, "info_x", text, 32);
    value_pos[0] = atof(text);
    get_group_tag_attr2("SEDIT_FOUND", "HEAD", 0, "info_y", text, 32);
    value_pos[1] = atof(text);
    get_group_tag_attr2("SEDIT_FOUND", "HEAD", 0, "info_z", text, 32);
    value_pos[2] = atof(text);
    get_group_tag_attr2("SEDIT_FOUND", "HEAD", 0, "line_height", text, 32);
    line_h = atof(text);
    for (new i; i < 6; i++)
    {
        if (!strcmp(g_szItemIds[i], "MAILER_SEDIT_FOUND_NAME"))
        {
            get_tag_attr("TEXT", "msgmailer_set_item_basic_name", "value", g_szItemNames[i], 32);
            mx_GetPrefStr("NAME", g_szValues[i], 128);
        }
        else if (!strcmp(g_szItemIds[i], "MAILER_SEDIT_FOUND_ADDR"))
        {
            get_tag_attr("TEXT", "msgmailer_set_item_basic_addr", "value", g_szItemNames[i], 32);
            mx_GetPrefStr("ADDR", g_szValues[i], 128);
        }
        else if (!strcmp(g_szItemIds[i], "MAILER_SEDIT_FOUND_SMTP"))
        {
            get_tag_attr("TEXT", "msgmailer_set_item_basic_smtp", "value", g_szItemNames[i], 32);
            mx_GetPrefStr("SMTP", g_szValues[i], 128);
        }
        else if (!strcmp(g_szItemIds[i], "MAILER_SEDIT_FOUND_POP3"))
        {
            get_tag_attr("TEXT", "msgmailer_set_item_basic_pop3", "value", g_szItemNames[i], 32);
            mx_GetPrefStr("POP3", g_szValues[i], 128);
        }
        else if (!strcmp(g_szItemIds[i], "MAILER_SEDIT_FOUND_USER"))
        {
            get_tag_attr("TEXT", "msgmailer_set_item_basic_user", "value", g_szItemNames[i], 32);
            mx_GetPrefStr("USERID", g_szValues[i], 128);
        }
        else if (!strcmp(g_szItemIds[i], "MAILER_SEDIT_FOUND_PASS"))
        {
            get_tag_attr("TEXT", "msgmailer_set_item_basic_pass", "value", g_szItemNames[i], 32);
            mx_GetPrefStr("PASSWD", g_szValues[i], 128);
        }
        g_objLabels[i] = CreateLabel(Self());
        SetPos(g_objLabels[i], label_pos[0], label_pos[1] + i * line_h, label_pos[2], 0);
        SetColor(g_objLabels[i], g_flLabelRed, g_flLabelGreen, g_flLabelBlue, 0.0);
        SetOptionS(g_objLabels[i], TAG("UTF8"), g_szItemNames[i]);
        SetOptionI(g_objLabels[i], TAG("HALN"), ALIGN_RIGHT);
        SetOptionI(g_objLabels[i], TAG("VALN"), ALIGN_CENTER);
        SetOptionI(g_objLabels[i], TAG("FSIZ"), 15);
        SetOptionI(g_objLabels[i], TAG("ZSRT"), ZSORT_OVERLAY);
        g_objValues[i] = CreateLabel(Self());
        SetPos(g_objValues[i], value_pos[0], value_pos[1] + i * line_h, value_pos[2], 0);
        SetColor(g_objValues[i], g_flTextRed, g_flTextGreen, g_flTextBlue, 0.0);
        SetOptionI(g_objValues[i], TAG("HALN"), ALIGN_LEFT);
        SetOptionI(g_objValues[i], TAG("VALN"), ALIGN_CENTER);
        SetOptionI(g_objValues[i], TAG("FSIZ"), 15);
        SetOptionI(g_objValues[i], TAG("W64 "), 200 * 64);
        SetOptionI(g_objValues[i], TAG("ZSRT"), ZSORT_OVERLAY);
        if (!strcmp(g_szItemIds[i], "MAILER_SEDIT_FOUND_PASS"))
        {
            mask_password(text, g_szValues[i]);
            SetOptionS(g_objValues[i], TAG("UTF8"), text);
        }
        else
        {
            SetOptionS(g_objValues[i], TAG("UTF8"), g_szValues[i]);
        }
    }
    g_iSetMenu = 0;
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
            if (g_iSetMenu - 1 >= 0)
            {
                g_iSetMenu--;
                sndSePlay(SE_CURSOR);
                refocus = 1;
            }
        }
        else if (pressed & PAD_DOWN || (repeated & PAD_DOWN && tickcount() - last_repeat > 70))
        {
            last_repeat = tickcount();
            if (g_iSetMenu + 1 < 6)
            {
                g_iSetMenu++;
                sndSePlay(SE_CURSOR);
                refocus = 1;
            }
        }
        else if (pressed & PAD_CIRCLE)
        {
            EditItem(g_iSetMenu);
        }
        if (g_active && refocus)
        {
            send_label_focus2(g_focus, g_objLabels[g_iSetMenu], 0, 0.5, 60.0, 60.0, 70.0, 1, 1);
            refocus = 0;
        }
        new msg;
        new what;
        new arg;
        /*
         * "FOCS" IN/OUT, "FADE" IN/OUT/DIM, "ACT" "SCHK" (check the settings); then "FNIS"/"FNIB" from mailsetskb with
         * the edited text.
         */
        while (RecvMessage(msg, what, arg))
        {
            if (msg == TAG("FOCS"))
            {
                if (what == TAG("IN"))
                {
                    SetActive(1);
                    refocus = 1;
                }
                else if (what == TAG("OUT"))
                {
                    SetActive(0);
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
            else if (msg == TAG("ACT"))
            {
                if (what == TAG("SCHK"))
                {
                    CheckSettings();
                    SendMessage(g_manager, TAG("ACWT"), 0, 0, 0, 0);
                }
            }
        }
        while (RecvMessageStr(msg, text, 32))
        {
            if (msg == TAG("FNIS"))
            {
                FinishEdit(g_iSetMenu, text, 1);
                refocus = 1;
            }
            if (msg == TAG("FNIB"))
            {
                FinishEdit(g_iSetMenu, text, 0);
                refocus = 1;
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
    for (new i; i < 6; i++)
    {
        GetColor(g_objLabels[i], color[0], color[1], color[2], color[3]);
        AnimColor(g_objLabels[i], color[0], color[1], color[2], alpha, duration, delay, ANIM_FUNC_LINEAR);
        GetColor(g_objValues[i], color[0], color[1], color[2], color[3]);
        AnimColor(g_objValues[i], color[0], color[1], color[2], alpha, duration, delay, ANIM_FUNC_LINEAR);
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

/* Gives the list the pad and the pad help, or takes them away with the focus. */
SetActive(active)
{
    if (active)
    {
        SetSystemParam(SYS_PADHELP, PADHELP_CIRCLE | PADHELP_CROSS);
        g_active = 1;
    }
    else
    {
        send_focus(g_focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        g_active = 0;
    }
}

/* Opens mailsetskb on the setting index, with the keyboard settings that suit it. */
EditItem(index)
{
    SetActive(0);
    new args[50];
    if (!strcmp(g_szItemIds[index], "MAILER_SEDIT_FOUND_NAME"))
    {
        sprintf(args, "mode=%d;input=%d;opt=%d;type=%d;limit=%d;title=%s;pass=%d;ucs4=%d;", SKB_KEYBOARD_HIRAGANA,
                SKB_INPUT_DEFAULT, 0, SKB_LENGTH_BYTES, 128, g_szItemNames[index], 0, 1);
    }
    else if (!strcmp(g_szItemIds[index], "MAILER_SEDIT_FOUND_ADDR"))
    {
        sprintf(args, "mode=%d;input=%d;opt=%d;type=%d;limit=%d;title=%s;pass=%d;ucs4=%d;", SKB_KEYBOARD_ALPHANUMERIC,
                SKB_INPUT_DIRECT, SKB_LOCK_INPUT_MODE | SKB_HALF_WIDTH, SKB_LENGTH_BYTES, 128, g_szItemNames[index], 0,
                1);
    }
    else if (!strcmp(g_szItemIds[index], "MAILER_SEDIT_FOUND_SMTP"))
    {
        sprintf(args, "mode=%d;input=%d;opt=%d;type=%d;limit=%d;title=%s;pass=%d;ucs4=%d;", SKB_KEYBOARD_ALPHANUMERIC,
                SKB_INPUT_DIRECT, SKB_LOCK_INPUT_MODE | SKB_HALF_WIDTH, SKB_LENGTH_BYTES, 128, g_szItemNames[index], 0,
                0);
    }
    else if (!strcmp(g_szItemIds[index], "MAILER_SEDIT_FOUND_POP3"))
    {
        sprintf(args, "mode=%d;input=%d;opt=%d;type=%d;limit=%d;title=%s;pass=%d;ucs4=%d;", SKB_KEYBOARD_ALPHANUMERIC,
                SKB_INPUT_DIRECT, SKB_LOCK_INPUT_MODE | SKB_HALF_WIDTH, SKB_LENGTH_BYTES, 128, g_szItemNames[index], 0,
                0);
    }
    else if (!strcmp(g_szItemIds[index], "MAILER_SEDIT_FOUND_USER"))
    {
        sprintf(args, "mode=%d;input=%d;opt=%d;type=%d;limit=%d;title=%s;pass=%d;ucs4=%d;", SKB_KEYBOARD_QWERTY,
                SKB_INPUT_DIRECT, SKB_LOCK_KEYBOARD | SKB_LOCK_INPUT_MODE | SKB_KANA_LOCK_FIXED | SKB_HALF_WIDTH,
                SKB_LENGTH_BYTES, 128, g_szItemNames[index], 0, 0);
    }
    else if (!strcmp(g_szItemIds[index], "MAILER_SEDIT_FOUND_PASS"))
    {
        sprintf(args, "mode=%d;input=%d;opt=%d;type=%d;limit=%d;title=%s;pass=%d;ucs4=%d;", SKB_KEYBOARD_QWERTY,
                SKB_INPUT_DIRECT, SKB_LOCK_KEYBOARD | SKB_LOCK_INPUT_MODE | SKB_KANA_LOCK_FIXED | SKB_HALF_WIDTH,
                SKB_LENGTH_BYTES, 120, g_szItemNames[index], 1, 0);
    }
    g_objKeyboard = CreateNull(Self());
    AttachScript(g_objKeyboard, _, "mailsetskb", args);
    SetPubVar(g_objKeyboard, "g_manager", Self());
    SetOptionI(g_objKeyboard, TAG("ZSRT"), ZSORT_OVERLAY);
    SendMessageStr(g_objKeyboard, TAG("INIT"), g_szValues[index]);
}

/* Ends the edit of setting index: on ok stores text in the preferences and restarts the mailer. */
FinishEdit(index, text[], ok)
{
    if (ok)
    {
        new keys[6][3] = {"NAME", "ADDR", "SMTP", "POP3", "USERID", "PASSWD"};
        copy_string(g_szValues[index], text, 32);
        mx_SetPrefStr(keys[index], text);
        mx_Restart();
        if (!strcmp(g_szItemIds[index], "MAILER_SEDIT_FOUND_PASS"))
        {
            new masked[32];
            mask_password(masked, text);
            SetOptionS(g_objValues[index], TAG("UTF8"), masked);
        }
        else
        {
            SetOptionS(g_objValues[index], TAG("UTF8"), text);
        }
    }
    SetSystemParam(SYS_PADHELP, PADHELP_CIRCLE | PADHELP_CROSS);
    Destroy(g_objKeyboard);
    SetActive(1);
}

/* Sets g_setchk if a setting other than the name is empty. */
CheckSettings()
{
    g_setchk = 0;
    for (new i; i < 6; i++)
    {
        if (!strcmp(g_szItemIds[i], "MAILER_SEDIT_FOUND_NAME"))
        {
            continue;
        }
        if (!strlen(g_szValues[i]))
        {
            g_setchk = 1;
            return;
        }
    }
}

#include "mailsync.inc"
