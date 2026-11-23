/*
 * Helpers of strings (search from the end, file name extensions, quotes, sizes), of the focus, a caption, the dialog
 * script, the links of the guide, the disc type and memory card information. get_screen_size3, 4 and 5 are
 * get_screen_size again, each with globals of its own: the scripts that include this file compile them where it puts
 * them.
 */

new Float:g_screen_width3;
new Float:g_screen_height3;
new Float:g_screen_depth3;
/* Same as get_screen_size, with globals of its own. */
stock get_screen_size3(&Float:screen_w, &Float:screen_h)
{
    GetScreen(Camera(0), g_screen_width3, g_screen_height3, g_screen_depth3);
    screen_w = g_screen_width3;
    screen_h = g_screen_height3;
}

/* Returns the index of the last occurrence of sub in str, ignoring case, or -1. */
stock find_last_nocase(str[], sub[])
{
    new len = strlen(str);
    new sub_len = strlen(sub);
    new skip;
    new j;
    new matched;
    while (skip < len)
    {
        while (skip < len)
        {
            if (toupper(str{len - 1 - skip}) == toupper(sub{sub_len - 1}))
            {
                break;
            }
            skip++;
        }
        if (skip >= len)
        {
            return -1;
        }
        matched = 1;
        j = 0;
        while (skip + j < len && j < sub_len && matched)
        {
            if (toupper(str{len - 1 - skip - j}) != toupper(sub{sub_len - 1 - j}))
            {
                matched = 0;
                skip++;
                break;
            }
            j++;
        }
        if (matched)
        {
            return len - skip - j;
        }
    }
    return -1;
}

new Float:g_screen_width4;
new Float:g_screen_height4;
new Float:g_screen_depth4;
/* Same as get_screen_size, with globals of its own. */
stock get_screen_size4(&Float:screen_w, &Float:screen_h)
{
    GetScreen(Camera(0), g_screen_width4, g_screen_height4, g_screen_depth4);
    screen_w = g_screen_width4;
    screen_h = g_screen_height4;
}

/* Copies src from start into dest (maxlen cells). */
stock copy_tail(dest[], src[], start, maxlen)
{
    new i;
    new max_chars;
    new count = strlen(src) - start;
    max_chars = maxlen * 4;
    i = 0;
    for (; i < count && i < max_chars; i++)
    {
        dest{i} = src{i + start};
    }
    return;
}

/* Copies the extension of path, from its last dot, into dest. */
stock get_extension(dest[], path[], maxlen)
{
    new count;
    new i;
    new max_chars;
    new dot = find_last_nocase(path, ".");
    count = strlen(path) - dot;
    max_chars = maxlen * 4;
    i = 0;
    for (; i < count && i < max_chars; i++)
    {
        dest{i} = path{i + dot};
    }
    return;
}

/* Copies src into dest doubling each single quote; returns -1 if dest is full. */
stock escape_quotes(dest[], src[], maxlen)
{
    new len = strlen(src);
    new max_chars = maxlen * 4 - 2;
    new out;
    for (new i; i < len && out < max_chars; i++)
    {
        if (src{i} == '\'')
        {
            dest{out++} = '\'';
            dest{out++} = '\'';
        }
        else
        {
            dest{out++} = src{i};
        }
    }
    dest{out++} = 0;
    if (out >= max_chars)
    {
        return -1;
    }
    return 0;
}

/* Replaces each single quote in text with a zero digit. */
stock replace_quotes(text[])
{
    new len = strlen(text);
    for (new i; i < len; i++)
    {
        if (text{i} == '\'')
        {
            text{i} = '0';
        }
    }
    return;
}

/* Returns whether str ends with suffix, ignoring case. */
stock ends_with_nocase(str[], suffix[])
{
    new len = strlen(str);
    new suffix_len = strlen(suffix);
    for (new i; i < suffix_len; i++)
    {
        if (toupper(str{len - 1 - i}) != toupper(suffix{suffix_len - 1 - i}))
        {
            return 0;
        }
    }
    return 1;
}

/* Sets the focus parameters as public variables of a focus object, with no class. */
stock set_focus(obj, target, z_offset, Float:sharp, Float:extra_w, Float:extra_h, Float:move_time, smoothing, blinking)
{
    if (!obj)
    {
        return;
    }
    SetPubVar(obj, "focused", target);
    SetPubVar(obj, "class", TAG(""));
    SetPubVar(obj, "zoffs", z_offset);
    SetPubVar(obj, "sharpness", sharp);
    SetPubVar(obj, "offs_width", extra_w);
    SetPubVar(obj, "offs_height", extra_h);
    SetPubVar(obj, "move_time", move_time);
    SetPubVar(obj, "smooth", smoothing);
    SetPubVar(obj, "blink", blinking);
}

/* Creates a centered button label at (x, y). */
stock create_button_label(parent, Float:x, Float:y)
{
    new label = CreateLabel(parent);
    SetPos(label, x, y, 0.0, 0);
    SetOptionI(label, TAG("HALN"), ALIGN_CENTER);
    SetOptionI(label, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(label, TAG("FSIZ"), 15);
    SetOptionI(label, TAG("AP64"), 1 * 64); /* 1 pixel */
    SetOptionI(label, TAG("KP64"), 1 * 64); /* 1 pixel */
    SetOptionI(label, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(label, TAG("ZOFS"), 2.0);
    return label;
}

new g_caption_index;
#if !defined g_caption_labels
new g_caption_labels[2][2]; /* two pairs of lines, shown in turn */
#endif
#if !defined g_caption_x
new Float:g_caption_x;
#endif
#if !defined g_caption_y
new Float:g_caption_y;
#endif
/* Shows a caption of one or two lines at (g_caption_x, g_caption_y), fading it in while the previous one fades out. */
stock show_caption(line1[] = "", line2[] = "", Float:target_a, Float:duration, Float:delay)
{
    new incoming = g_caption_index;
    g_caption_index = ++g_caption_index & 1;
    new outgoing = g_caption_index;
    SetOptionS(g_caption_labels[incoming][0], TAG("UTF8"), line1);
    SetOptionS(g_caption_labels[incoming][1], TAG("UTF8"), line2);
    if (strlen(line2) > 0)
    {
        SetPos(g_caption_labels[incoming][0], g_caption_x, g_caption_y - 12.0, 0.0, 0);
        SetPos(g_caption_labels[incoming][1], g_caption_x, 12.0 + g_caption_y, 0.0, 0);
    }
    else
    {
        SetPos(g_caption_labels[incoming][0], g_caption_x, g_caption_y, 0.0, 0);
        SetPos(g_caption_labels[incoming][1], g_caption_x, g_caption_y, 0.0, 0);
    }
    AnimColor(g_caption_labels[incoming][0], 0.7843, 0.7843, 0.7843, target_a, duration, delay, ANIM_FUNC_LINEAR);
    AnimColor(g_caption_labels[incoming][1], 0.7843, 0.7843, 0.7843, target_a, duration, delay, ANIM_FUNC_LINEAR);
    AnimColor(g_caption_labels[outgoing][0], 0.7843, 0.7843, 0.7843, 0.0, 0.5 * duration, 0.0, ANIM_FUNC_LINEAR);
    AnimColor(g_caption_labels[outgoing][1], 0.7843, 0.7843, 0.7843, 0.0, 0.5 * duration, 0.0, ANIM_FUNC_LINEAR);
}

/* Opens the dialog script with its arguments args at the center of the screen and returns its object. */
stock open_dialog(args[])
{
    new screen_w;
    new screen_h;
    new screen_d;
    wndGetScreen(WND_SELF, screen_w, screen_h, screen_d);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 0.5 * float(screen_w), 0.5 * float(screen_h), 0.0, 0);
    AttachScript(dialog, _, "dialog", args);
    SetPubVar(dialog, "manager", Self());
    return dialog;
}

/* The units of get_size_units, in the order of the TEXT formats msgutil_info_size_mbyte2, mbyte1, kbyte2, kbyte1. */
const SIZE_MB_THOUSANDS = 1; /* "%d,%03d MB": whole in thousands of MB, fraction the MB over them */
const SIZE_MB = 2;
const SIZE_KB_THOUSANDS = 3;
const SIZE_KB = 4;

/* Splits a size in bytes into a unit (SIZE_*), a whole part and a fraction; get_size_units never gives SIZE_MB. */
stock get_size_units(bytes, &unit, &whole, &fraction)
{
    new count;
    printf(".............size = %d\n", bytes);
    unit = 0;
    whole = 0;
    fraction = 0;
    count = bytes / 1048576000;
    if (count > 0)
    {
        unit = SIZE_MB_THOUSANDS;
        whole = count;
        fraction = (bytes - count * 1048576000) / 1048576;
        return;
    }
    count = bytes / 1024000;
    if (count > 0)
    {
        unit = SIZE_KB_THOUSANDS;
        whole = count;
        fraction = (bytes - count * 1024000) / 1024;
        return;
    }
    unit = SIZE_KB;
    whole = bytes / 1024;
    return;
}

new g_guide_links[54][64]; /* by link number, from 1; 0 is not used */
/* Fills g_guide_links with the names of the guide pages. */
stock init_guide_links()
{
    g_guide_links[1] = "ln_bnguide";
    g_guide_links[2] = "ln_back";
    g_guide_links[3] = "ln_whatsnew";
    g_guide_links[4] = "ln_guide";
    g_guide_links[5] = "ln_guide_beforeuse";
    g_guide_links[6] = "ln_guide_aboutfee";
    g_guide_links[7] = "ln_guide_aboutcertify";
    g_guide_links[8] = "ln_guide_aboutguide";
    g_guide_links[9] = "ln_guide_whatcan";
    g_guide_links[10] = "ln_guide_whatcan_guide";
    g_guide_links[11] = "ln_guide_whatcan_channel";
    g_guide_links[12] = "ln_guide_whatcan_isp";
    g_guide_links[13] = "ln_guide_whatcan_internet";
    g_guide_links[14] = "ln_guide_whatcan_setting";
    g_guide_links[15] = "ln_guide_operate";
    g_guide_links[19] = "ln_guide_navim";
    g_guide_links[20] = "ln_guide_operationm";
    g_guide_links[16] = "ln_guide_operationm_controler";
    g_guide_links[17] = "ln_guide_operationm_screenkey";
    g_guide_links[18] = "ln_guide_operationm_usbkey";
    g_guide_links[21] = "ln_guide_gamech";
    g_guide_links[22] = "ln_guide_gamech_play";
    g_guide_links[23] = "ln_guide_gamech_end";
    g_guide_links[24] = "ln_guide_gamech_delete";
    g_guide_links[25] = "ln_guide_musicch";
    g_guide_links[26] = "ln_guide_musicch_2";
    g_guide_links[27] = "ln_guide_musicch_record";
    g_guide_links[28] = "ln_guide_musicch_play";
    g_guide_links[29] = "ln_guide_musicch_info";
    g_guide_links[30] = "ln_guide_musicch_edit";
    g_guide_links[31] = "ln_guide_photoch";
    g_guide_links[32] = "ln_guide_photoch_save";
    g_guide_links[33] = "ln_guide_photoch_show";
    g_guide_links[34] = "ln_guide_photoch_info";
    g_guide_links[35] = "ln_guide_photoch_edit";
    g_guide_links[36] = "ln_guide_moviech";
    g_guide_links[37] = "ln_guide_moviech_play";
    g_guide_links[38] = "ln_guide_moviech_info";
    g_guide_links[39] = "ln_guide_setting";
    g_guide_links[40] = "ln_guide_setting_operateinfo";
    g_guide_links[41] = "ln_guide_setting_setting";
    g_guide_links[42] = "ln_guide_setting_netinfo";
    g_guide_links[43] = "ln_guide_setting_datainfo";
    g_guide_links[44] = "ln_support";
    g_guide_links[45] = "ln_support_faq";
    g_guide_links[46] = "ln_support_faq_a";
    g_guide_links[47] = "ln_support_map";
    g_guide_links[48] = "ln_support_map_a";
    g_guide_links[49] = "ln_support_term";
    g_guide_links[50] = "ln_support_term_a";
    g_guide_links[51] = "ln_support_question";
    g_guide_links[52] = "ln_support_question_a";
    g_guide_links[53] = "ln_readme";
}

/* Returns the disc type, CDVD_DETECTING for the types being detected and CDVD_ILLEGAL for unknown ones. */
stock get_cdvd_type()
{
    new type = GetSystemParam(SYS_CDVD_TYPE);
    switch (type)
    {
        case CDVD_NO_DISC:
        {
            return type;
        }
        case CDVD_DETECTING:
        {
            return type;
        }
        case CDVD_DETECTING_CD:
        {
            return CDVD_DETECTING;
        }
        case CDVD_DETECTING_DVD:
        {
            return CDVD_DETECTING;
        }
        case CDVD_DETECTING_DVD_DL:
        {
            return CDVD_DETECTING;
        }
        case CDVD_PS1_CD:
        {
            return type;
        }
        case CDVD_PS1_CDDA:
        {
            return type;
        }
        case CDVD_PS2_CD:
        {
            return type;
        }
        case CDVD_PS2_CDDA:
        {
            return type;
        }
        case CDVD_PS2_DVD:
        {
            return type;
        }
    }
    return CDVD_ILLEGAL;
}

new Float:g_screen_width5;
new Float:g_screen_height5;
new Float:g_screen_depth5;
/* Same as get_screen_size, with globals of its own. */
stock get_screen_size5(&Float:screen_w, &Float:screen_h)
{
    GetScreen(Camera(0), g_screen_width5, g_screen_height5, g_screen_depth5);
    screen_w = g_screen_width5;
    screen_h = g_screen_height5;
}

/* Returns whether name ends with a dot and ext, ignoring case. */
stock has_extension(name[], ext[])
{
    new len = strlen(name);
    new ext_len = strlen(ext);
    for (new i; i < ext_len; i++)
    {
        if (toupper(name{len - 1 - i}) != toupper(ext{ext_len - 1 - i}))
        {
            return 0;
        }
    }
    if ('.' != toupper(name{len - 1 - ext_len}))
    {
        return 0;
    }
    return 1;
}

/* The card types of get_card_info (0: no card), and its status besides 0 (the same card as at the last look). */
const MC_TYPE_PS1 = 1;
const MC_TYPE_PS2 = 2;
const MC_TYPE_PDA = 3;             /* a PocketStation */
const MC_CHANGED = -1;             /* another card since the last look, formatted */
const MC_CHANGED_UNFORMATTED = -2; /* another card since the last look, not formatted */

/* Gets the information of a memory card; returns 0 or a negative error. */
stock get_card_info(port, slot, &type, &free_blocks, &formatted, &status)
{
    new ret = svdatGetMCInfo(port, slot, type, free_blocks, formatted, status);
    if (ret < 0)
    {
        return ret;
    }
    return 0;
}

