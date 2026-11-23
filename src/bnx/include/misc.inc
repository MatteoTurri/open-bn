/*
 * Assorted helpers: curve positions, the screen size, focus and caption helpers, memory card information, dialog
 * parts, ROT13 and Base64, color packing.
 */

/* Gets the point at t (0.0 to 1.0) of a Hermite curve of node_count nodes. */
stock get_curve_pos(curve, Float:t, node_count, &Float:x, &Float:y, &Float:z)
{
    GetPosHermite(curve, x, y, z, t, node_count);
}

new Float:g_screen_width2;
new Float:g_screen_height2;
new Float:g_screen_depth2;
/* Same as get_screen_size, with globals of its own. */
stock get_screen_size2(&Float:screen_w, &Float:screen_h)
{
    GetScreen(Camera(0), g_screen_width2, g_screen_height2, g_screen_depth2);
    screen_w = g_screen_width2;
    screen_h = g_screen_height2;
}

/* Sets the focus parameters of a label as public variables of a focus object. */
stock set_label_focus(obj, target, z_offset, Float:sharp, Float:extra_w, Float:extra_h, Float:move_time, smoothing,
        blinking)
{
    if (!obj)
    {
        return;
    }
    SetPubVar(obj, "focused", target);
    SetPubVar(obj, "class", TAG("LABL"));
    SetPubVar(obj, "zoffs", z_offset);
    SetPubVar(obj, "sharpness", sharp);
    SetPubVar(obj, "offs_width", extra_w);
    SetPubVar(obj, "offs_height", extra_h);
    SetPubVar(obj, "move_time", move_time);
    SetPubVar(obj, "smooth", smoothing);
    SetPubVar(obj, "blink", blinking);
}

#if !defined g_default_interval
new Float:g_default_interval = 70.0;
#endif
/* Returns the script argument interval<index> as a float, or g_default_interval. */
stock Float:get_interval(index)
{
    new key[64];
    new value[64];
    sprintf(key, "interval%d", index);
    if (GetArg(key, value, 64))
    {
        return atof(value);
    }
    return g_default_interval;
}

/* Refreshes the information of one memory card (port, slot) or of both ports; returns 0 or a negative error. */
stock update_card_info(&unused, one_card, port, slot)
{
    new ret;
    new job;
    if (one_card)
    {
        ret = svdatMCInfo(port, slot, job);
        if (ret < 0)
        {
            return ret;
        }
        for (; ; )
        {
            new percent;
            new status;
            ret = svdatSync(percent, status, job);
            if (ret == 1)
            {
                break;
            }
            if (ret < 0)
            {
                break;
            }
            sleep;
        }
        if (ret < 0)
        {
            return ret;
        }
    }
    else
    {
        ret = svdatMCInfo(0, 0, job);
        if (ret < 0)
        {
            return ret;
        }
        for (; ; )
        {
            new percent2;
            new status2;
            ret = svdatSync(percent2, status2, job);
            if (ret == 1)
            {
                break;
            }
            if (ret < 0)
            {
                break;
            }
            sleep;
        }
        if (ret < 0)
        {
            return ret;
        }
        ret = svdatMCInfo(1, 0, job);
        if (ret < 0)
        {
            return ret;
        }
        for (; ; )
        {
            new percent3;
            new status3;
            ret = svdatSync(percent3, status3, job);
            if (ret == 1)
            {
                break;
            }
            if (ret < 0)
            {
                break;
            }
            sleep;
        }
        if (ret < 0)
        {
            return ret;
        }
    }
    return 0;
}

/* Waits for the pending album database request. */
stock wait_database()
{
    new status;
    for (; ; )
    {
        status = datSync();
        if (0 != status)
        {
            return status;
        }
        sleep;
    }
    return 0;
}

stock apply_rot13(text[])
{
    for (new i; text{i}; i++)
    {
        if ('a' <= text{i} <= 'z')
        {
            text{i} = (text{i} - 'a' + 13) % 26 + 'a';
        }
        else if ('A' <= text{i} <= 'Z')
        {
            text{i} = (text{i} - 'A' + 13) % 26 + 'A';
        }
    }
}

/* Like create_button, white and with resource texture 28. */
stock create_button_textured(parent, Float:x, Float:y)
{
    new button = CreatePlane3x3(parent);
    SetPos(button, x, y, 0.0, 0);
    SetScale(button, 80.0, 32.0, 1.0);
    SetColor(button, 1.0, 1.0, 1.0, 0.0);
    SetTexture(button, imgGetResource(RES_BUTTON0), 0, -1);
    SetOptionI(button, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(button, TAG("ZOFS"), 1.0);
    SetOptionI(button, TAG("ALPH"), BLEND_ADD);
    new Float:tex_margins[4] = {0.5, 0.5, 0.5, 0.5};
    SetOptionP(button, TAG("MGNT"), tex_margins, 4);
    new Float:scale[3];
    GetScale(button, scale[0], scale[1], scale[2]);
    new Float:margins[4];
    if (scale[0] > scale[1])
    {
        margins[0] = margins[1] = 0.5 * scale[1] / scale[0];
        margins[2] = margins[3] = 0.5;
    }
    else
    {
        margins[2] = margins[3] = 0.5 * scale[0] / scale[1];
        margins[0] = margins[1] = 0.5;
    }
    SetOptionP(button, TAG("MGN"), margins, 4);
    return button;
}

/* Creates the textbox of a dialog message, text_w by text_h pixels. */
stock create_message_text(parent, text_w, text_h)
{
    new textbox = CreateTextbox(parent);
    SetOptionI(textbox, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionI(textbox, TAG("HALN"), ALIGN_CENTER);
    SetOptionI(textbox, TAG("FSIZ"), 15);
    SetOptionI(textbox, TAG("TW64"), text_w << 6);
    SetOptionI(textbox, TAG("TH64"), text_h << 6);
    SetOptionI(textbox, TAG("LS64"), 4 * 64);
    return textbox;
}

#if !defined g_caption_x
new Float:g_caption_x;
#endif
#if !defined g_caption_y
new Float:g_caption_y;
#endif
/* Sets where show_caption puts the caption. */
stock set_caption_pos(Float:x, Float:y)
{
    g_caption_x = x;
    g_caption_y = y;
}

/* Same as send_label_focus. */
stock send_label_focus2(obj, target, z_offset, Float:sharp, Float:extra_w, Float:extra_h, Float:move_time, smoothing,
        blinking)
{
    if (!obj)
    {
        return;
    }
    SendMessage(obj, TAG("FOC0"), target, TAG("LABL"), z_offset, sharp);
    SendMessage(obj, TAG("FOC1"), extra_w, extra_h, move_time, smoothing);
    SendMessage(obj, TAG("FOC2"), blinking, 0, 0, 0);
}

/* Encodes len bytes of data in Base64 into out (out_max cells); returns the number of characters. */
stock encode_base64(data[], len, out[], out_max)
{
    out_max = out_max * 4;
    new out_len;
    for (new i; i < len; i = i + 3)
    {
        new pad;
        new chunk[1];
        chunk{0} = data{i};
        if (i + 1 < len)
        {
            chunk{1} = data{i + 1};
        }
        else
        {
            pad++;
        }
        if (i + 2 < len)
        {
            chunk{2} = data{i + 2};
        }
        else
        {
            pad++;
        }
        for (new j; j < 4 - pad; j++)
        {
            out{out_len} = get_base64_char(chunk[0] >> 8 >> (3 - j) * 6 & 0x3F);
            out_len++;
        }
        for (new k; k < pad; k++)
        {
            out{out_len} = '=';
            out_len++;
        }
    }
    return out_len;
}

/* Packs four bytes into a cell, r in the highest byte. */
stock pack_color(r, g, b, a)
{
    return (r & 0xFF) << 24 | (g & 0xFF) << 16 | (b & 0xFF) << 8 | a & 0xFF;
}

/* Splits a packed color into components from 0.0 to 1.0. */
stock unpack_color(color, &Float:r, &Float:g, &Float:b, &Float:a)
{
    r = float(color >> 24 & 0xFF) / 255.0;
    g = float(color >> 16 & 0xFF) / 255.0;
    b = float(color >> 8 & 0xFF) / 255.0;
    a = float(color & 0xFF) / 255.0;
}

