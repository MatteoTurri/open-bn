/*
 * The deletion of a photo from a USB storage device and the copy of a photo to one.
 */

#if !defined g_not_found
new g_not_found = -2;
#endif
/*
 * Asks, deletes photo item of the USB album and shows the result; returns 0 or a negative error (-5: the device was
 * removed).
 */
stock delete_usb_photo(focus_obj, item)
{
    new album_mode;
    {
        new mode_text[32];
        avalGetValue("ALBUM_MODE", 0, mode_text, 32);
        if (!strcmp("USB", mode_text))
        {
            album_mode = ALBUM_USB;
        }
        else
        {
            return -1;
        }
    }
    new ret;
    new progress_win;
    focus = focus_obj;
    {
        new answer = ask_yes_no("msgphoto_pict_del_ask", _, _, 0);
        if (0 != answer)
        {
            return -1;
        }
    }
    new invalid;
    if (get_wnd_var_int("usb_invalid", invalid))
    {
        if (0 != invalid)
        {
            ret = -5;
            goto done;
        }
    }
    new path[128];
    photoListGet(item, path, 128);
    if (strlen(path) <= 0)
    {
        ret = -1;
        goto done;
    }
    {
        new st_mode;
        new st_size;
        ret = fileLstat(path, st_mode, st_size);
        if (!(st_mode & 0x80)) /* not writable by its owner */
        {
            new answer2 = ask_yes_no("msgphoto_pict_del_ask_protect", "msgphoto_pict_del_ask_entity2", _, 0);
            if (0 != answer2)
            {
                return -1;
            }
        }
    }
    progress_win = open_progress_window("msgphoto_pict_del");
    SetPubVar(progress_win, "progress", 15);
    if (get_wnd_var_int("usb_invalid", invalid))
    {
        if (0 != invalid)
        {
            ret = -5;
            goto done;
        }
    }
    SetPubVar(progress_win, "progress", 50);
    printf("selected item %d ------------------\n", item);
    if (get_wnd_var_int("usb_invalid", invalid))
    {
        if (0 != invalid)
        {
            ret = -5;
            goto done;
        }
    }
    if (album_mode == ALBUM_USB)
    {
        new error;
        ret = fileDeleteUSB(path);
        if (ret == 1)
        {
            ret = -3;
            goto done;
        }
        if (ret < 0)
        {
            ret = -2;
            goto done;
        }
        if (wait_file_op(ret, error, progress_win, 33, 99) < 0)
        {
            if (ret < 0)
            {
                ret = -2;
                goto done;
            }
            if (error == 2)
            {
                ret = -4;
                goto done;
            }
        }
        if (get_wnd_var_int("usb_invalid", invalid))
        {
            if (0 != invalid)
            {
                ret = -5;
                goto done;
            }
        }
    }
    sleep;
    {
        new st_mode2;
        new st_size2;
        ret = fileLstat(path, st_mode2, st_size2);
        if (ret != g_not_found)
        {
            ret = -2;
            goto done;
        }
        if (get_wnd_var_int("usb_invalid", invalid))
        {
            if (0 != invalid)
            {
                ret = -5;
                goto done;
            }
        }
        ret = 0;
    }
    done:
    if (progress_win)
    {
        SetPubVar(progress_win, "progress", 100);
        while (SCRIPT_FINISHED != ScriptState(progress_win))
        {
            sleep;
        }
        Destroy(progress_win);
    }
    printf("delete ret %d\n", ret);
    show_result_message(ret, "msgphoto_pict_del_complete", _, "msgphoto_pict_del_fail", _, "msgphoto_pict_del_fail", _,
            "msgphoto_pict_del_fail", "msgphoto_pict_checkout_mswrite_disable", "", _, "msgphoto_pict_del_fail",
            "msgphoto_warn_usb_connect");
    return ret;
}

/*
 * Asks, copies photo item into the photo folder of the USB storage (made where missing), named after its title, and
 * shows the result; returns 0 or a negative error.
 */
stock export_photo_to_usb(focus_obj, item)
{
    focus = focus_obj;
    new ret;
    new error;
    new progress_win;
    new invalid;
    ret = ask_yes_no("msgphoto_pict_checkout_ask", _, _, 0);
    if (0 != ret)
    {
        return -1;
    }
    if (get_wnd_var_int("usb_invalid", invalid))
    {
        if (invalid > 0)
        {
            ret = -5;
            goto done;
        }
        if (invalid <= 0)
        {
            if (get_wnd_var_int("usb_found", invalid))
            {
                if (invalid)
                {
                    set_wnd_var_int("usb_invalid", 0);
                }
                else
                {
                    ret = -5;
                    goto done;
                }
            }
        }
    }
    progress_win = open_progress_window("msgphoto_pict_checkin", "msgphoto_warn_external_device_eject",
            "msgphoto_warn_console_power");
    new db = open_album_db(4, 0);
    printf("new db opened %d\n", db);
    if (db < 0)
    {
        ret = -1;
        goto done;
    }
    if (get_wnd_var_int("usb_invalid", invalid))
    {
        if (0 != invalid)
        {
            ret = -5;
            goto done;
        }
    }
    {
        new buf[128];
        new dir[128];
        new st_mode;
        new st_size;
        get_tag_attr("TEXT", "text_USBMOUNT", "value", dir, 128);
        append_string(dir, "/", 128);
        sleep;
        ret = fileLstat(dir, st_mode, st_size);
        if (get_wnd_var_int("usb_invalid", invalid))
        {
            if (0 != invalid)
            {
                ret = -5;
                goto done;
            }
        }
        if (ret == g_not_found)
        {
            ret = fileMkdir(dir);
            if (ret < 0)
            {
                ret = -2;
                goto done;
            }
        }
        else if (ret < 0)
        {
            ret = -2;
            goto done;
        }
        get_tag_attr("TEXT", "text_USBMSSONY", "value", buf, 128);
        append_string(dir, buf, 128);
        append_string(dir, "/", 128);
        sleep;
        ret = fileLstat(dir, st_mode, st_size);
        if (get_wnd_var_int("usb_invalid", invalid))
        {
            if (0 != invalid)
            {
                ret = -5;
                goto done;
            }
        }
        if (ret == g_not_found)
        {
            ret = fileMkdir(dir);
            if (ret < 0)
            {
                ret = -2;
                goto done;
            }
        }
        else if (ret < 0)
        {
            ret = -2;
            goto done;
        }
        get_tag_attr("TEXT", "text_USBPSBB", "value", buf, 128);
        append_string(dir, buf, 128);
        append_string(dir, "/", 128);
        sleep;
        ret = fileLstat(dir, st_mode, st_size);
        if (get_wnd_var_int("usb_invalid", invalid))
        {
            if (0 != invalid)
            {
                ret = -5;
                goto done;
            }
        }
        if (ret == g_not_found)
        {
            ret = fileMkdir(dir);
            if (ret < 0)
            {
                ret = -2;
                goto done;
            }
        }
        else if (ret < 0)
        {
            ret = -2;
            goto done;
        }
        get_tag_attr("TEXT", "text_USBPHOTO", "value", buf, 128);
        append_string(dir, buf, 128);
        append_string(dir, "/", 128);
        sleep;
        ret = fileLstat(dir, st_mode, st_size);
        if (get_wnd_var_int("usb_invalid", invalid))
        {
            if (0 != invalid)
            {
                ret = -5;
                goto done;
            }
        }
        if (ret == g_not_found)
        {
            ret = fileMkdir(dir);
            if (ret < 0)
            {
                ret = -2;
                goto done;
            }
        }
        else if (ret < 0)
        {
            ret = -2;
            goto done;
        }
        SetPubVar(progress_win, "progress", 33);
        new ext[32];
        new src[128];
        photoListGet(item, src, 128);
        get_extension(ext, src, 32);
        printf("usb checkout dir %s\n", dir);
        {
            new file_name[128];
            get_photo_field(item, FIELD_TITLE, buf, PHOTOS_LIBRARY, 0, 128);
            photoUTF8toSJIS(buf, buf);
            ret = make_usb_file_name(file_name, buf, 128);
            if (ret == 1)
            {
                ret = -255;
                goto done;
            }
            if (!ends_with_nocase(file_name, ext))
            {
                append_string(file_name, ext, 128);
            }
            sprintf(buf, "%s%s", dir, file_name);
            sleep;
            ret = fileLstat(buf, st_mode, st_size);
            if (get_wnd_var_int("usb_invalid", invalid))
            {
                if (0 != invalid)
                {
                    ret = -5;
                    goto done;
                }
            }
            printf("dst_url check %d , %s\n", ret, buf);
            if (ret == g_not_found)
            {
                copy_string(dir, buf, 128);
            }
            else
            {
                if (ret < 0)
                {
                    ret = -2;
                    goto done;
                }
                if (ret >= 0)
                {
                    ret = -3;
                    goto done;
                }
            }
        }
        printf("src %s\n", src);
        printf("dst %s\n", dir);
        SetPubVar(progress_win, "progress", 50);
        sleep;
        if (get_wnd_var_int("usb_invalid", invalid))
        {
            if (0 != invalid)
            {
                ret = -5;
                goto done;
            }
        }
        ret = fileCopy(src, dir);
        if (ret < 0)
        {
            ret = -2;
            goto done;
        }
        if (wait_file_op(ret, error, progress_win, 50, 99) < 0)
        {
            if (ret < 0)
            {
                ret = -2;
                goto done;
            }
            if (error == 2)
            {
                ret = -4;
                goto done;
            }
        }
        if (get_wnd_var_int("usb_invalid", invalid))
        {
            if (0 != invalid)
            {
                ret = -5;
                goto done;
            }
        }
        sleep;
        ret = fileLstat(dir, st_mode, st_size);
        if (ret == g_not_found)
        {
            ret = -2;
            goto done;
        }
        if (get_wnd_var_int("usb_invalid", invalid))
        {
            if (0 != invalid)
            {
                ret = -5;
                goto done;
            }
        }
    }
    done:
    if (progress_win)
    {
        SetPubVar(progress_win, "progress", 100);
        while (SCRIPT_FINISHED != ScriptState(progress_win))
        {
            sleep;
        }
        Destroy(progress_win);
    }
    if (ret == -255)
    {
        new msg[256];
        new line_text[128];
        get_tag_attr("TEXT", "msgphoto_pict_checkin_fail", "value", msg, 256);
        get_tag_attr("TEXT", "msgphoto_pict_checkout_warn_string", "value", line_text, 128);
        append_string(msg, "\n", 256);
        append_string(msg, line_text, 256);
        append_string(msg, "\n", 256);
        sprintf(line_text, "                     \\ / : * ? \" < > | ");
        append_string(msg, "\n", 256);
        append_string(msg, line_text, 256);
        show_message(msg, "");
    }
    else
    {
        show_result_message(ret, "msgphoto_pict_checkin_complete", _, "msgphoto_pict_checkin_fail",
                "msgphoto_pict_checkin_fail_possible", "msgphoto_pict_checkin_fail",
                "msgphoto_pict_checkin_fail_possible", "msgphoto_pict_checkin_fail", "msgphoto_usb_samedata",
                "msgphoto_pict_checkin_fail", "msgphoto_pict_external_device_nospace", "msgphoto_pict_checkin_fail",
                "msgphoto_warn_usb_connect");
    }
    return ret;
}

