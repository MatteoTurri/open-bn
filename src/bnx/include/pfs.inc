/*
 * The work directories of the photos on the hard disk, the save of the pad help, dialogs, text labels and the
 * password check.
 */

/* Creates a label at (x, y) with the text, font size, alignments and blending, fading in to color[3]. */
stock create_text_label(parent, Float:x, Float:y, Float:color[], font_size, text[], text_w, v_align, h_align, blend)
{
    new label;
    label = CreateLabel(parent);
    SetPos(label, x, y, 0.0, 0);
    SetColor(label, color[0], color[1], color[2], 0.0);
    SetOptionS(label, TAG("UTF8"), text);
    SetOptionI(label, TAG("ALPH"), blend);
    SetOptionI(label, TAG("HALN"), h_align);
    SetOptionI(label, TAG("VALN"), v_align);
    SetOptionI(label, TAG("FSIZ"), font_size);
    if (text_w > 0)
    {
        SetOptionI(label, TAG("W64 "), text_w * 64);
    }
    AnimColor(label, color[0], color[1], color[2], color[3], 200.0, 0.0, ANIM_FUNC_LINEAR);
    return label;
}

/* Like create_button, white. */
stock create_button_white(parent, Float:x, Float:y)
{
    new button = CreatePlane3x3(parent);
    SetPos(button, x, y, 0.0, 0);
    SetScale(button, 80.0, 32.0, 1.0);
    SetColor(button, 1.0, 1.0, 1.0, 0.0);
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

#if !defined g_pad_help_on
new g_pad_help_on;
#endif
#if !defined g_pad_help_cross
new g_pad_help_cross[32];
#endif
#if !defined g_pad_help_triangle
new g_pad_help_triangle[32];
#endif
#if !defined g_pad_help_square
new g_pad_help_square[32];
#endif
#if !defined g_pad_help_circle
new g_pad_help_circle[32];
#endif
/* Saves the pad help (its on/off state and the labels of the four buttons) for restore_pad_help. */
stock save_pad_help()
{
    g_pad_help_on = GetSystemParam(SYS_PADHELP);
    GetSystemParamStr(SYS_STR_PADHELP_RD, g_pad_help_cross, 32);
    GetSystemParamStr(SYS_STR_PADHELP_RU, g_pad_help_triangle, 32);
    GetSystemParamStr(SYS_STR_PADHELP_RL, g_pad_help_square, 32);
    GetSystemParamStr(SYS_STR_PADHELP_RR, g_pad_help_circle, 32);
}

/* Asks the background scene to follow the camera of the page. */
stock adjust_bg_camera()
{
    new scene = wndGetScene(WND_BG);
    if (scene)
    {
        SendMessage(scene, TAG("AJST"), Camera(0), 0, 0, 0);
    }
}

/* Same as open_dialog. */
stock open_dialog2(args[])
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

/* Cancels a dialog, waits for its script to end and destroys it. */
stock close_dialog(dialog)
{
    SetPubVar(dialog, "canceled", 1);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    Destroy(dialog);
}

/* Reads the TEXT element whose id is the value attribute of node; returns 0 if there is none. */
stock get_linked_text(node, text[], maxlen)
{
    new text_id[16];
    if (domGetData(node, "value", text_id, 16))
    {
        if (0 != (node = domFindTag(domRoot(), "TEXT", text_id, 0)))
        {
            return domGetData(node, "value", text, maxlen);
        }
    }
    return 0;
}

new g_pfs_contents[] = "pfs:/__contents/";
/* Returns 0 if pfs:/__contents/ has more than need_kb kilobytes free, -4 if not, -2 on error. */
stock check_pfs_free(need_kb)
{
    new ret;
    new free_kb;
    ret = fileGetFreeSize(g_pfs_contents, free_kb);
    printf("check pfs free %d Mb, req %d Kbyte\n", free_kb / 1024, need_kb);
    if (ret < 0)
    {
        return -2;
    }
    if (free_kb <= need_kb)
    {
        return -4;
    }
    return 0;
}

#if !defined g_contents_dir
new g_contents_dir[] = "pfs:/__contents/contents/";
#endif
#if !defined g_not_found
new g_not_found = -2; /* what fileLstat returns for a missing file */
#endif
new g_photo_dir[] = "pfs:/__contents/contents/photo/";
/* Creates the contents and photo directories where missing; returns 0, or -2 on error. */
stock make_photo_dirs()
{
    new st_mode;
    new st_size;
    new ret = fileLstat(g_contents_dir, st_mode, st_size);
    if (ret == g_not_found)
    {
        ret = fileMkdir(g_contents_dir);
        if (ret < 0)
        {
            return -2;
        }
    }
    else if (ret < 0)
    {
        return -2;
    }
    ret = fileLstat(g_photo_dir, st_mode, st_size);
    if (ret == g_not_found)
    {
        ret = fileMkdir(g_photo_dir);
        if (ret < 0)
        {
            return -2;
        }
    }
    else if (ret < 0)
    {
        return -2;
    }
    return 0;
}

#if !defined g_photo_tmp_dir
new g_photo_tmp_dir[] = "pfs:/__contents/contents/photo/tmp/";
#endif
/*
 * Makes an empty photo tmp directory, deleting the old one with progress from from to to percent on progress_win;
 * returns 0, or -2 on error.
 */
stock prepare_photo_tmp(progress_win, from, to)
{
    printf("check filetmp\n");
    new st_mode;
    new st_size;
    new ret = make_photo_dirs();
    if (ret < 0)
    {
        return -2;
    }
    ret = fileLstat(g_photo_tmp_dir, st_mode, st_size);
    if (ret == g_not_found)
    {
        ret = fileMkdir(g_photo_tmp_dir);
        if (ret < 0)
        {
            return -2;
        }
    }
    else if (ret >= 0)
    {
        ret = fileDelete(g_photo_tmp_dir);
        if (ret < 0)
        {
            return -2;
        }
        new error;
        if (wait_file_op(ret, error, progress_win, from, to) < 0)
        {
            if (ret < 0)
            {
                return -2;
            }
        }
        ret = fileMkdir(g_photo_tmp_dir);
        if (ret < 0)
        {
            return -2;
        }
    }
    else if (ret < 0)
    {
        return -2;
    }
    return 0;
}

/* Deletes the photo tmp directory if it exists; returns 0, or -2 on error. */
stock clear_photo_tmp(progress_win, from, to)
{
    printf("clear filetmp\n");
    new st_mode;
    new st_size;
    new ret = fileLstat(g_photo_tmp_dir, st_mode, st_size);
    if (ret >= 0)
    {
        ret = fileDelete(g_photo_tmp_dir);
        if (ret < 0)
        {
            return -2;
        }
        new error;
        if (wait_file_op(ret, error, progress_win, from, to) < 0)
        {
            if (ret < 0)
            {
                return -2;
            }
        }
    }
    else
    {
        if (ret == g_not_found)
        {
            return 0;
        }
        if (ret < 0)
        {
            return -2;
        }
    }
    return 0;
}

/* Returns 1 for 6 to 31 allowed characters with at least one digit and one letter. */
stock is_valid_password(password[])
{
    if (!(strlen(password) >= 6 && strlen(password) <= 31))
    {
        return;
    }
    new i;
    new c;
    new has_digit;
    new has_letter;
    i = 0;
    for (; password{i}; i++)
    {
        c = password{i};
        if (c >= '0' && c <= '9')
        {
            has_digit = 1;
            continue;
        }
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
        {
            has_letter = 1;
            continue;
        }
        if (c == '.' || c == '_' || c == '-' || c == '@' || c == '#' || c == '+' || c == '!' || c == '$' || c == ':'
                || c == '%' || c == '*')
        {
            continue;
        }
        return;
    }
    return has_digit == 1 && has_letter == 1 ? 1 : 0;
}

