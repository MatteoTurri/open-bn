/*
 * /opt0/bn/script/mailer/mailseditcreate.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The mail creation settings of the mailer (signature, quoting the message in a reply) as a list under a
 * title (argument "top"); the settings page chooses a new value with a list of choices and reattaches
 * the script with the arguments "edit" (the item) and "parts" (the choice), which are stored here.
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
new g_objLabels[2];
new g_objValues[2];
new g_szItemIds[2][16]; /* the ids of the items, from SEDIT_CREATE_ITEM */
new g_iChoices[2]; /* the choice of each item: the signature kind, quoting on (0) or off (1) */
new g_szSignature[128]; /* the text of the signature */

main()
{
    new Float:label_pos[3];
    new Float:value_pos[3];
    new Float:line_h;
    new text[64];
    g_chkskb = 0;
    copy_string(g_szSignature, "", 512);
    get_group_tag_attr2("SEDIT_CREATE", "HEAD", 0, "folder_x", text, 64);
    label_pos[0] = atof(text);
    get_group_tag_attr2("SEDIT_CREATE", "HEAD", 0, "folder_y", text, 64);
    label_pos[1] = atof(text);
    get_group_tag_attr2("SEDIT_CREATE", "HEAD", 0, "folder_z", text, 64);
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
    GetItemIds("SEDIT_CREATE_ITEM", g_szItemIds);
    get_group_tag_attr2("SEDIT_CREATE", "HEAD", 0, "item_x", text, 64);
    label_pos[0] = atof(text);
    get_group_tag_attr2("SEDIT_CREATE", "HEAD", 0, "item_y", text, 64);
    label_pos[1] = atof(text);
    get_group_tag_attr2("SEDIT_CREATE", "HEAD", 0, "item_z", text, 64);
    label_pos[2] = atof(text);
    get_group_tag_attr2("SEDIT_CREATE", "HEAD", 0, "info_x", text, 64);
    value_pos[0] = atof(text);
    get_group_tag_attr2("SEDIT_CREATE", "HEAD", 0, "info_y", text, 64);
    value_pos[1] = atof(text);
    get_group_tag_attr2("SEDIT_CREATE", "HEAD", 0, "info_z", text, 64);
    value_pos[2] = atof(text);
    get_group_tag_attr2("SEDIT_CREATE", "HEAD", 0, "line_height", text, 64);
    line_h = atof(text);
    for (new i; i < 2; i++)
    {
        if (!strcmp(g_szItemIds[i], "MAILER_SEDIT_CREATE_SIGN"))
        {
            if (GetArg("edit", text, 64) && i == atoi(text))
            {
                GetArg("parts", text, 64);
                g_iChoices[i] = atoi(text);
                mx_SetPref("SIG_TYPE", g_iChoices[i]);
                mx_Restart();
            }
            else
            {
                mx_GetPref("SIG_TYPE", g_iChoices[i]);
            }
            mx_GetPrefStr("SIG_TEXT", g_szSignature, 90);
        }
        if (!strcmp(g_szItemIds[i], "MAILER_SEDIT_CREATE_MARK"))
        {
            if (GetArg("edit", text, 64) && i == atoi(text))
            {
                GetArg("parts", text, 64);
                g_iChoices[i] = atoi(text);
                mx_SetPref("WITH_QUOTE", g_iChoices[i] ^ 1);
                mx_Restart();
            }
            else
            {
                mx_GetPref("WITH_QUOTE", g_iChoices[i]);
                g_iChoices[i] ^= 1;
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
            if (g_iSetMenu + 1 < 2)
            {
                g_iSetMenu++;
                sndSePlay(SE_CURSOR);
                refocus = 1;
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
                    if (!strcmp(g_szItemIds[g_iSetMenu], "MAILER_SEDIT_CREATE_SIGN"))
                    {
                        GetSignatureArgs(text);
                    }
                    else if (!strcmp(g_szItemIds[g_iSetMenu], "MAILER_SEDIT_CREATE_MARK"))
                    {
                        GetQuoteArgs(text);
                    }
                    SendMessageStr(g_manager, TAG("ACWT"), text);
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
    for (new i; i < 2; i++)
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

/* Gets the caption of item index and the text of its current choice. */
GetItemTexts(index, label[], value[])
{
    new label_text[64];
    new value_text[64];
    new item;
    new yes_no_ids[2][16];
    GetItemIds("SEDIT_COMMON_ITEM", yes_no_ids);
    if (!strcmp(g_szItemIds[index], "MAILER_SEDIT_CREATE_SIGN"))
    {
        get_tag_attr("TEXT", "msgmailer_set_item_write_signature", "value", label_text, 64);
        new sign_ids[4][16];
        GetItemIds("SEDIT_CREATE_SIGN_ITEM", sign_ids);
        item = GetItemIndex("SEDIT_CREATE_ITEM", "MAILER_SEDIT_CREATE_SIGN");
        if (!strcmp(sign_ids[g_iChoices[item]], "MAILER_SEDIT_CREATE_NONE"))
        {
            get_tag_attr("TEXT", "msgmailer_set_item_write_signature_none", "value", value_text, 64);
        }
        else if (!strcmp(sign_ids[g_iChoices[item]], "MAILER_SEDIT_CREATE_ADDR"))
        {
            get_tag_attr("TEXT", "msgmailer_set_item_write_signature_addr", "value", value_text, 64);
        }
        else if (!strcmp(sign_ids[g_iChoices[item]], "MAILER_SEDIT_CREATE_NAME_ADDR"))
        {
            get_tag_attr("TEXT", "msgmailer_set_item_write_signature_nameaddr", "value", value_text, 64);
        }
        else if (!strcmp(sign_ids[g_iChoices[item]], "MAILER_SEDIT_CREATE_TEXT"))
        {
            if (strlen(g_szSignature))
            {
                new text[128];
                copy_string(text, g_szSignature, 512);
                mx_TrimString(text, 10);
                sprintf(value_text, "\"%s\"", text);
            }
            else
            {
                get_tag_attr("TEXT", "msgmailer_set_item_write_signature_text", "value", value_text, 64);
            }
        }
    }
    else if (!strcmp(g_szItemIds[index], "MAILER_SEDIT_CREATE_MARK"))
    {
        get_tag_attr("TEXT", "msgmailer_set_item_reply_with_body", "value", label_text, 64);
        item = GetItemIndex("SEDIT_CREATE_ITEM", "MAILER_SEDIT_CREATE_MARK");
        if (!strcmp(yes_no_ids[g_iChoices[item]], "MAILER_SEDIT_YES"))
        {
            get_tag_attr("TEXT", "msgmailer_set_item_reply_with_body_yes", "value", value_text, 64);
        }
        else if (!strcmp(yes_no_ids[g_iChoices[item]], "MAILER_SEDIT_NO"))
        {
            get_tag_attr("TEXT", "msgmailer_set_item_reply_with_body_no", "value", value_text, 64);
        }
    }
    copy_string(label, label_text, 256);
    copy_string(value, value_text, 256);
}

/* Writes in args the arguments of the list of signature choices. */
GetSignatureArgs(args[])
{
    new label[64];
    new choices[4][64];
    new item = GetItemIndex("SEDIT_CREATE_ITEM", "MAILER_SEDIT_CREATE_SIGN");
    new text_index = GetItemIndex("SEDIT_CREATE_SIGN_ITEM", "MAILER_SEDIT_CREATE_TEXT");
    {
        new saved = g_iChoices[item];
        g_iChoices[item] = 0;
        for (new i; i < 4; i++)
        {
            GetItemTexts(item, label, choices[i]);
            g_iChoices[item]++;
        }
        g_iChoices[item] = saved;
    }
    sprintf(args, "btn0=%s;btn1=%s;btn2=%s;btn3=%s;label=%s;sel=%d;next=%d;flg=%d;", choices[0], choices[1], choices[2],
            choices[3], label, g_iChoices[item], text_index, 1);
}

/* Writes in args the arguments of the yes/no choice on quoting. */
GetQuoteArgs(args[])
{
    new label[32];
    new choices[10][32];
    new item = GetItemIndex("SEDIT_CREATE_ITEM", "MAILER_SEDIT_CREATE_MARK");
    {
        new saved = g_iChoices[item];
        g_iChoices[item] = 0;
        for (new i; i < 2; i++)
        {
            GetItemTexts(item, label, choices[i]);
            g_iChoices[item]++;
        }
        g_iChoices[item] = saved;
    }
    sprintf(args, "btn0=%s;btn1=%s;label=%s;sel=%d;", choices[0], choices[1], label, g_iChoices[item]);
}

#include "mailsync.inc"
