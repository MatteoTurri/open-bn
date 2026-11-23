/*
 * /opt0/bn/script/utility/browser_fileinfo.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The information on the selected save of the file browser (the source BRSRCURL): its titles, size, date, kind
 * and the operations it allows, drawn as text textures, in a short form beside the album or a full list.
 */

#include <core>
#include <bn>
#include <float>

#pragma dynamic 1280

#include "browser.inc"

#include "tag.inc"

#include "imgsync.inc"

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

#include "wndvar.inc"

public device_mode; /* window variable BRDEVMODE: DEVMODE_* */
public mc_port; /* the port of the memory card shown (window variable BRMCPORT) */
public mc_slot; /* its slot (window variable BRMCSLOT) */

#include "string.inc"

new g_labels[14]; /* planes with the text textures: 5 in the short form, 7 captions and 7 values in the list */
new g_loaded; /* the information of the save is read */
new g_file_name[512]; /* the name, and from cell 256 the directory */
new g_file_size;
new g_title1[256]; /* the title of the save, its first line */
new g_title2[256]; /* and its second line */
new g_save_kind; /* KIND_* */
new g_no_copy; /* 1: the save may not be copied or moved */
new g_damaged;
new g_year;
new g_month;
new g_day;
new g_hour;
new g_minute;
new g_second;
new g_mc_type; /* window variable BRMCTYPE: the type of the memory card */

public main()
{
    create_labels();
    new mode;
    new hide_countdown;
    for (; ; )
    {
        new msg;
        new Float:a;
        new Float:b;
        new c;
        /*
         * "FADE" (alpha, duration): fades the labels, the titles in yellow; "LOAD": reads the save again; "MODE" (mode)
         */
        while (RecvMessage(msg, a, b, c))
        {
            if (msg == TAG("FADE"))
            {
                if (!g_loaded)
                {
                    a = 0.0;
                }
                new count = 0 == mode ? 5 : 14;
                if (0 != _:a)
                {
                    ShowChildren(Self());
                    hide_countdown = 0;
                }
                else
                {
                    hide_countdown = 60;
                }
                new highlight_mask = 0 == mode ? 6 : 384;
                for (new i; i < count; i++)
                {
                    if (highlight_mask & 1 << i)
                    {
                        AnimColor(g_labels[i], 0.902, 0.902, 0.215, a, b, 0.0, ANIM_FUNC_LINEAR);
                    }
                    else
                    {
                        AnimColor(g_labels[i], 0.7843, 0.7843, 0.7843, a, b, 0.0, ANIM_FUNC_LINEAR);
                    }
                }
            }
            if (msg == TAG("LOAD"))
            {
                if (!load_info())
                {
                    continue;
                }
                if (0 == mode)
                {
                    show_album_info();
                }
                else
                {
                    show_details();
                }
            }
            if (msg == TAG("MODE"))
            {
                mode = a;
                if (0 == mode)
                {
                    show_album_info();
                }
                else
                {
                    show_details();
                }
            }
        }
        if (hide_countdown > 0)
        {
            hide_countdown--;
            if (0 == hide_countdown)
            {
                HideChildren(Self());
            }
        }
        sleep;
    }
}

/* Creates the planes of the labels, hidden. */
create_labels()
{
    HideChildren(Self());
    for (new i; i < 14; i++)
    {
        new label = CreatePlane(Self());
        g_labels[i] = label;
        SetColor(label, 1.0, 1.0, 1.0, 0.0);
        SetOptionI(label, TAG("NUM"), 2);
        SetItemOptionF(label, 0, TAG("DIFF"), 0.0, 0, 0, 1.0);
    }
}

/*
 * Reads the information of the save BRSRCURL; returns 1, or 0 on failure, which counts it as damaged. The address
 * 1248 given to svdatGetInfoData for the directory is g_file_name[256].
 */
load_info()
{
    new url[256];
    g_loaded = 0;
    avalGetValue("BRSRCURL", 0, url, 256);
    new job;
    new ret = svdatInformation(url, job);
    if (ret < 0)
    {
        return 0;
    }
    new percent;
    new status;
    for (; ; )
    {
        ret = svdatSync(percent, status, job);
        if (ret == 1 || ret < 0)
        {
            break;
        }
        sleep;
    }
    if (status < 0 || ret < 0)
    {
        if (status == SVDAT_EIO)
        {
            set_wnd_var_int("BRPFSERR", 1);
        }
        g_damaged = 1;
        return 0;
    }
    g_damaged = 0;
    ret = svdatGetInfoData(job, g_file_name, 1248, g_file_size, g_title1, g_title2, g_save_kind, g_no_copy, g_year,
            g_month, g_day, g_hour, g_minute, g_second, g_damaged, 256, 256, 256, 256);
    {
        new damaged;
        get_wnd_var_int("BRDAMAGED", damaged);
        if (damaged == STATE_DAMAGED)
        {
            g_damaged = 1;
        }
    }
    if (g_damaged)
    {
        get_tag_attr("TEXT", "msgutil_info_datatype_broken", "value", g_title1, 256);
        copy_string(g_title2, "", 256);
    }
    get_wnd_var_int("BRDEVMODE", device_mode);
    get_wnd_var_int("BRMCPORT", mc_port);
    get_wnd_var_int("BRMCSLOT", mc_slot);
    get_wnd_var_int("BRMCTYPE", g_mc_type);
    g_loaded = 1;
    return 1;
}

/* Places and font sizes of the short form: album name, the two titles, size, date. */
new Float:g_album_label_x[5] = {-260.0, -260.0, -260.0, -260.0, -260.0};
new Float:g_album_label_y[5] = {150.0, 125.0, 100.0, -143.0, -160.0};
new g_album_font_sizes[5] = {14, 18, 18, 14, 14};

/* Shows the short form: the album name, the two titles, the size and the date. */
show_album_info()
{
    new label;
    for (new i; i < 14; i++)
    {
        Hide(g_labels[i]);
    }
    for (new i; i < 5; i++)
    {
        label = g_labels[i];
        SetPos(label, g_album_label_x[i], g_album_label_y[i], 0.0, 0);
    }
    new text[64];
    avalGetValue("BrowserAlbumName", 0, text, 64);
    sleep;
    draw_text(g_labels[0], text, g_album_font_sizes[0], 6, 304);
    sleep;
    draw_text(g_labels[1], g_title1, g_album_font_sizes[1], 6, 512);
    sleep;
    draw_text(g_labels[2], g_title2, g_album_font_sizes[2], 6, 512);
    sleep;
    format_size(text, 64);
    sleep;
    draw_text(g_labels[3], text, g_album_font_sizes[3], 6, 512);
    sleep;
    format_date(text, 64);
    sleep;
    draw_text(g_labels[4], text, g_album_font_sizes[4], 6, 512);
    sleep;
    for (new i; i < 5; i++)
    {
        Show(g_labels[i]);
    }
}

/* The TEXT ids of the captions of the list. */
new g_caption_ids[7][] = {
    "msgutil_devinfo_title", "", "msgutil_devinfo_location", "msgutil_info_size",
    "msgutil_info_date", "msgutil_info_datatype", "msgutil_info_ope"
};

/* Shows the list: title, location (album), size, date, kind and operations, each with its caption. */
show_details()
{
    new label;
    new Float:y = 35.0;
    for (new i; i < 7; i++)
    {
        label = g_labels[i];
        Hide(label);
        SetPos(label, -70.0, y, 0.0, 0);
        label = g_labels[i + 7];
        Hide(label);
        SetPos(label, -50.0, y, 0.0, 0);
        y = y - 28.8;
    }
    SetPos(g_labels[0], -70.0, 20.5, 0.0, 0);
    if (0 == strlen(g_title2))
    {
        SetPos(g_labels[7], -50.0, 20.5, 0.0, 0);
    }
    else
    {
        SetPos(g_labels[7], -50.0, 35.0 - 4.0, 0.0, 0);
        SetPos(g_labels[8], -50.0, 6.2 + 4.0, 0.0, 0);
    }
    new text[64];
    for (new i; i < 7; i++)
    {
        copy_string(text, "", 64);
        get_tag_attr("TEXT", g_caption_ids[i], "value", text, 64);
        sleep;
        draw_text(g_labels[i], text, 15, 4, 512);
        sleep;
    }
    draw_text(g_labels[7], g_title1, 15, 6, 512);
    sleep;
    draw_text(g_labels[8], g_title2, 15, 6, 512);
    sleep;
    avalGetValue("BrowserAlbumName", 0, text, 64);
    sleep;
    draw_text(g_labels[9], text, 15, 6, 336);
    sleep;
    format_size(text, 64);
    sleep;
    draw_text(g_labels[10], text, 15, 6, 512);
    sleep;
    format_date(text, 64);
    sleep;
    draw_text(g_labels[11], text, 15, 6, 512);
    sleep;
    get_kind_text(text, 64);
    sleep;
    draw_text(g_labels[12], text, 15, 6, 512);
    sleep;
    get_operation_text(text, 64);
    sleep;
    draw_text(g_labels[13], text, 15, 6, 512);
    sleep;
    for (new i; i < 14; i++)
    {
        Show(g_labels[i]);
    }
}

/* Offsets of a text texture from its position, in its own size, by the alignment bits of draw_text. */
new Float:g_align_offsets[3] = {-0.5, 0.0, 0.5};

/*
 * Renders text into the texture of label at font_size, at most width_limit pixels wide; align has the horizontal
 * offset in bits 0-1 and the vertical one in bits 2-3 (an index of g_align_offsets).
 */
draw_text(label, text[], font_size, align, width_limit)
{
    imgDecStringStart(text, font_size, 64, 64, width_limit);
    wait_image_decode();
    SetTexture(label, imgTexID(), 0, -1);
    new Float:tex_w = float(imgTexWidth());
    new Float:tex_h = float(imgTexHeight());
    SetLocalScale(label, tex_w, tex_h, 1.0);
    new Float:offset_x = g_align_offsets[align & 3];
    new Float:offset_y = g_align_offsets[align >> 2 & 3];
    SetOptionF(label, TAG("CLIP"), offset_x, offset_y, 1.0, 1.0);
    SetItemOptionF(label, 0, TAG("CROP"), -2.0 / tex_w, -2.0 / tex_h, 1.0, 1.0);
}

/* Writes the size of the save into text, with the formats msgutil_info_size_*. */
format_size(text[], maxlen)
{
    new format[64];
    new unit;
    new whole;
    new fraction;
    get_size_units(g_file_size, unit, whole, fraction);
    switch (unit)
    {
        case SIZE_MB_THOUSANDS:
        {
            get_tag_attr("TEXT", "msgutil_info_size_mbyte2", "value", format, 64);
            sprintf(text, format, whole, fraction);
        }
        case SIZE_MB:
        {
            get_tag_attr("TEXT", "msgutil_info_size_mbyte1", "value", format, 64);
            sprintf(text, format, whole);
        }
        case SIZE_KB_THOUSANDS:
        {
            get_tag_attr("TEXT", "msgutil_info_size_kbyte2", "value", format, 64);
            sprintf(text, format, whole, fraction);
        }
        case SIZE_KB:
        {
            get_tag_attr("TEXT", "msgutil_info_size_kbyte1", "value", format, 64);
            sprintf(text, format, whole);
        }
    }
}

/*
 * Writes the date of the save into text with the time and date notation of the system; without a date (a virtual
 * memory card, a PlayStation or PocketStation card, an unknown field) the msgutil_info_date_bar* form of it.
 */
format_date(text[], maxlen)
{
    new time_notation = GetSystemParam(SYS_TIME_NOTATION);
    new date_notation = GetSystemParam(SYS_DATE_NOTATION);
    if (time_notation < 0 || time_notation > 1)
    {
        time_notation = 0;
    }
    if (date_notation < 0 || date_notation > 2)
    {
        date_notation = 0;
    }
    if (device_mode == DEVMODE_VMC
            || (device_mode == DEVMODE_MC && (g_mc_type == MC_TYPE_PDA || g_mc_type == MC_TYPE_PS1)) || g_year == -1
            || g_month == -1 || g_day == -1 || g_hour == -1 || g_minute == -1 || g_second == -1)
    {
        switch (time_notation)
        {
            case 0:
            {
                switch (date_notation)
                {
                    case DATE_YYYYMMDD:
                    {
                        get_tag_attr("TEXT", "msgutil_info_date_bar1", "value", text, maxlen);
                    }
                    case DATE_MMDDYYYY:
                    {
                        get_tag_attr("TEXT", "msgutil_info_date_bar2", "value", text, maxlen);
                    }
                    case DATE_DDMMYYYY:
                    {
                        get_tag_attr("TEXT", "msgutil_info_date_bar3", "value", text, maxlen);
                    }
                }
            }
            case 1:
            {
                switch (date_notation)
                {
                    case DATE_YYYYMMDD:
                    {
                        get_tag_attr("TEXT", "msgutil_info_date_bar11", "value", text, maxlen);
                    }
                    case DATE_MMDDYYYY:
                    {
                        get_tag_attr("TEXT", "msgutil_info_date_bar12", "value", text, maxlen);
                    }
                    case DATE_DDMMYYYY:
                    {
                        get_tag_attr("TEXT", "msgutil_info_date_bar13", "value", text, maxlen);
                    }
                }
            }
        }
    }
    else
    {
        new format[64];
        switch (time_notation)
        {
            case 0:
            {
                switch (date_notation)
                {
                    case DATE_YYYYMMDD:
                    {
                        get_tag_attr("TEXT", "msgutil_info_date_num1", "value", format, 64);
                        sprintf(text, format, g_year, g_month, g_day, g_hour, g_minute, g_second);
                    }
                    case DATE_MMDDYYYY:
                    {
                        get_tag_attr("TEXT", "msgutil_info_date_num2", "value", format, 64);
                        sprintf(text, format, g_month, g_day, g_year, g_hour, g_minute, g_second);
                    }
                    case DATE_DDMMYYYY:
                    {
                        get_tag_attr("TEXT", "msgutil_info_date_num3", "value", format, 64);
                        sprintf(text, format, g_day, g_month, g_year, g_hour, g_minute, g_second);
                    }
                }
            }
            case 1:
            {
                new ampm[1]; /* "AM" or "PM", packed */
                new hour;
                if (g_hour >= 12)
                {
                    hour = g_hour - 12;
                    sprintf(ampm, "PM");
                }
                else
                {
                    hour = g_hour;
                    sprintf(ampm, "AM");
                }
                if (0 == hour)
                {
                    hour = 12;
                }
                switch (date_notation)
                {
                    case DATE_YYYYMMDD:
                    {
                        get_tag_attr("TEXT", "msgutil_info_date_num11", "value", format, 64);
                        sprintf(text, format, g_year, g_month, g_day, hour, g_minute, g_second, ampm);
                    }
                    case DATE_MMDDYYYY:
                    {
                        get_tag_attr("TEXT", "msgutil_info_date_num12", "value", format, 64);
                        sprintf(text, format, g_month, g_day, g_year, hour, g_minute, g_second, ampm);
                    }
                    case DATE_DDMMYYYY:
                    {
                        get_tag_attr("TEXT", "msgutil_info_date_num13", "value", format, 64);
                        sprintf(text, format, g_day, g_month, g_year, hour, g_minute, g_second, ampm);
                    }
                }
            }
        }
    }
}

/* Writes the kind of the save (PlayStation 2 or PlayStation save, PocketStation data, ...) into text. */
get_kind_text(text[], maxlen)
{
    if (0 == g_damaged)
    {
        switch (g_save_kind)
        {
            case KIND_PS1_SAVE:
            {
                get_tag_attr("TEXT", "msgutil_info_datatype_saveps1", "value", text, maxlen);
            }
            case KIND_PS2_SAVE:
            {
                get_tag_attr("TEXT", "msgutil_info_datatype_saveps2", "value", text, maxlen);
            }
            case KIND_PDA_SAVE:
            {
                get_tag_attr("TEXT", "msgutil_info_datatype_pocket", "value", text, maxlen);
            }
            case KIND_PS2_SOFTWARE:
            {
                get_tag_attr("TEXT", "msgutil_info_datatype_ps2software", "value", text, maxlen);
            }
            case KIND_UNKNOWN:
            {
                get_tag_attr("TEXT", "msgutil_info_datatype_unknown", "value", text, maxlen);
            }
            case KIND_PDA:
            {
                get_tag_attr("TEXT", "msgutil_info_datatype_pocket", "value", text, maxlen);
            }
            case KIND_SETTING:
            {
                get_tag_attr("TEXT", "msgutil_info_datatype_setting", "value", text, maxlen);
            }
            case KIND_SYSTEM_DRIVER:
            {
                get_tag_attr("TEXT", "msgutil_info_datatype_systemdrv", "value", text, maxlen);
            }
            default:
            {
                get_tag_attr("TEXT", "msgutil_info_datatype_unknown", "value", text, maxlen);
            }
        }
    }
    else
    {
        get_tag_attr("TEXT", "msgutil_info_datatype_broken", "value", text, maxlen);
    }
}

/* Writes the operations the save does not allow into text: copy and move, or none. */
get_operation_text(text[], maxlen)
{
    new copy_text[32];
    new move_text[32];
    if (0 == g_damaged)
    {
        if (g_no_copy == 1)
        {
            get_tag_attr("TEXT", "msgutil_info_ope_copy_prohibit", "value", copy_text, 32);
            get_tag_attr("TEXT", "msgutil_info_ope_move_prohibit", "value", move_text, 32);
            sprintf(text, "%s %s", copy_text, move_text);
        }
        else
        {
            get_tag_attr("TEXT", "msgutil_info_ope_none", "value", text, maxlen);
        }
    }
    else
    {
        get_tag_attr("TEXT", "msgutil_info_ope_copy_prohibit", "value", copy_text, 32);
        get_tag_attr("TEXT", "msgutil_info_ope_move_prohibit", "value", move_text, 32);
        sprintf(text, "%s %s", copy_text, move_text);
    }
}
