/*
 * /opt0/bn/script/system/download.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The download page: downloads the content of the argument cref to the hard disk, into the album album,
 * showing its thumbnail, title, size, progress and time left; then offers to play it, or goes to nextref.
 */

#include <core>
#include <bn>

#pragma dynamic 2048

/* What downloadGetData and downloadSetData read or write. */
const DL_FILE = 0;     /* the path of the downloaded file */
const DL_TITLE = 1;    /* the title to save it with */
const DL_ALBUM = 2;    /* the name of the album it went into */
const DL_TYPE = 4;     /* the kind of content (CONTENT_*) */
const DL_TMP_FILE = 5; /* the file being written, while the download runs */

/* The phases of a download (the phase of downloadSync). */
const PHASE_PREPARE = 0;
const PHASE_RECEIVE = 1;    /* the file is being received */
const PHASE_WAIT_TITLE = 2; /* waiting for downloadSetData(DL_TITLE, ...) */
const PHASE_INSTALL = 3;    /* the content goes into its album and the database */
const PHASE_DONE = 4;
const PHASE_FAILED = 5;

/* The results of downloadSync. */
const SYNC_RUNNING = 0;
const SYNC_FINISHED = 1;
const SYNC_ERROR = -1; /* the code is in downloadGetError */

/* The errors of downloadGetError: of the HTTP library, of the HTTP file, and of the download library. */
const DL_ERR_HTTP_TIMEOUT = -1005;
const DL_ERR_HTTP_ABORTED = -1011;
const DL_ERR_HTTP_LENGTH = -1015; /* the body is not as long as its Content-Length */
const DL_ERR_LENGTH = -10004;     /* the same */
const DL_ERR_TIMEOUT = -10006;
const DL_ERR_TOO_MANY_ALBUMS = -51008;
const DL_ERR_ALBUM_FULL = -51009; /* the album has as many contents as it can */

/*
 * Kinds of content: 0 until the download knows it. The download library itself reports only movies (.pss, .psm)
 * and photos.
 */
const CONTENT_MOVIE = 1;
const CONTENT_PHOTO = 2;
const CONTENT_MUSIC = 3;
const CONTENT_OTHER = -1;

/* The units of format_size. */
const UNIT_KB = 0;
const UNIT_MB = 1;

#include "tag.inc"

#include <float>

/* The natives of the download library (download.so): the download of a content to the hard disk. */
#pragma library "download"
native downloadStart(const url[], const title[], const album[]);
/*
 * Returns SYNC_RUNNING while the download runs, SYNC_FINISHED once it has finished and SYNC_ERROR on an error
 * (downloadGetError); space_ok is 0 when the hard disk has no room for the file and 256 MB more.
 */
native downloadSync(&phase, &received, &total, &space_ok);
native downloadGetError();
/* Suspends the download, or resumes it with suspend 0. */
native downloadSuspend(suspend);
native downloadAbort();
native downloadGetData(what, const buf[], maxlength);
/* Sets the title (what 1) of a download that waits for it (phase 2). */
native downloadSetData(what, const value[]);
#pragma library

/* The natives of the moviedata library (moviedata.so): the title and the thumbnail of a movie file. */
#pragma library "moviedata"
/*
 * Starts reading block entry of the user data of a movie file: kind 0 reads it as text (entry 0: the title), kind 1
 * as a TIM2 texture (entry 3: the thumbnail).
 */
native moviedataReadStart(const path[], kind, entry);
/* Returns 0 while reading, -1 on an error, 1 with the text in buf or, for kind 1, the texture in buf[0]. */
native moviedataReadSync(const buf[], maxlength);

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

#include "xmlsync.inc"

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

new g_content_attr[1];

/* Reads the content of the tag element with that id. */
get_tag_content(tag[], id[], value[], maxlen)
{
    if (0 == tag[0] || 0 == id[0])
    {
        return 0;
    }
    new node = domFindTag(domRoot(), tag, id, 0);
    if (node)
    {
        return domGetData(node, g_content_attr, value, maxlen);
    }
    return 0;
}

/* Writes a size of bytes in KB (TEXT kb_id) or, from 10000 KB, in MB (TEXT mb_id). */
format_size(dest[], bytes, kb_id[], mb_id[])
{
    new kb = bytes / 1024;
    new mb = bytes / 1048576;
    new unit;
    new amount;
    if (0 == bytes)
    {
        unit = UNIT_KB;
        amount = 0;
    }
    else if (0 < bytes && kb < 1)
    {
        unit = UNIT_KB;
        amount = 1;
    }
    else if (kb < 10000)
    {
        unit = UNIT_KB;
        amount = kb;
    }
    else
    {
        unit = UNIT_MB;
        amount = mb;
    }
    new unit_text[32];
    switch (unit)
    {
        case UNIT_KB:
        {
            get_tag_attr("TEXT", kb_id, "value", unit_text, 32);
        }
        case UNIT_MB:
        {
            get_tag_attr("TEXT", mb_id, "value", unit_text, 32);
        }
    }
    sprintf(dest, "%d %s", amount, unit_text);
}

#include "focus.inc"

get_content_type()
{
    new data[1];
    downloadGetData(DL_TYPE, data, 1);
    return data[0];
}

/* Aborts the download and waits for it to stop; returns 1 if it had already finished, -1 otherwise. */
abort_download()
{
    downloadAbort();
    new last_phase;
    new last_received;
    for (; ; )
    {
        new dl_phase;
        new dl_received;
        new dl_total;
        new dl_space_ok;
        new status = downloadSync(dl_phase, dl_received, dl_total, dl_space_ok);
        if (dl_phase != last_phase || dl_received != last_received)
        {
            printf("status = %d, phase = %d, %d / %d (aborting...)\n", status, dl_phase, dl_received, dl_total);
        }
        last_received = dl_received;
        last_phase = dl_phase;
        switch (status)
        {
            case SYNC_ERROR:
            {
                new error = downloadGetError();
                printf("code = %d\n", error);
                switch (error)
                {
                    case DL_ERR_HTTP_ABORTED:
                    {
                        return -1;
                    }
                    case DL_ERR_HTTP_TIMEOUT:
                    {
                        printf("<<<<<<<<<<< timeout >>>>>>>>>>>\n");
                        return -1;
                    }
                    default:
                    {
                        new msg[512];
                        get_tag_attr("TEXT", "text_dlabort_error", "value", msg, 512);
                        show_error(msg, error);
                        go_to_next();
                    }
                }
            }
            case SYNC_FINISHED:
            {
                printf("download has already been finished.\n");
                return 1;
            }
            case SYNC_RUNNING:
            {
                if (dl_phase == PHASE_WAIT_TITLE)
                {
                    set_download_title();
                }
            }
        }
        sleep;
    }
    return -1;
}

new g_focus;
new g_saved_cross_label[64]; /* the pad help label of cross before the page set it to g_quit_label */
new g_quit_label[64];
/* The labels of the page, by index. */
enum
{
    LABEL_TITLE,        /* three lines, 0 to 2 */
    LABEL_SIZE = 3,     /* the size of the download */
    LABEL_ACTION,       /* "download" */
    LABEL_CAUTION0,
    LABEL_CAUTION1,
    LABEL_PERCENT,
    LABEL_TIME_LEFT,
    LABEL_COUNT
};

new Float:g_label_x[LABEL_COUNT] = {100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0};
new Float:g_label_y[LABEL_COUNT] = {-100.0, -80.0, -60.0, -40.0, -20.0, 20.0, 40.0, 80.0, 100.0};
new g_label_size[LABEL_COUNT] = {17, 17, 17, 15, 15, 15, 15, 17, 15};
new g_title[256];
new g_title_line[256]; /* the title on one line */
new g_album[256];
new g_cref[256];
new g_thumbnail[256];
new g_nextref[256];
new g_content_type;
new g_album_name[256];
new g_from_disc; /* cref is on the disc in the drive */
new g_percent_text[4];
new g_time_left_text[10];
new g_hour_text[4];
new g_minute_text[4];
new g_second_text[4];

main()
{
    hisClear();
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 0);
    SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
    g_content_type = 0;
    GetArg("title", g_title, 256);
    GetArg("album", g_album, 256);
    GetArg("cref", g_cref, 256);
    GetArg("thumbnail_image", g_thumbnail, 256);
    GetArg("nextref", g_nextref, 256);
    if (0 == g_title[0])
    {
        new last_slash = find_last_any_char(g_cref, "/", -1);
        printf("last_slash = %d\n", last_slash);
        if (last_slash >= 0)
        {
            copy_substring(g_title, 0, g_cref, last_slash + 1, -1, 256);
        }
        else
        {
            copy_string(g_title, g_cref, 256);
        }
    }
    else
    {
        new text[256];
        replace_all(text, g_title, "\\n", "\n", 256);
        copy_string(g_title, text, 256);
    }
    replace_all(g_title_line, g_title, "\n", " ", 256);
    if (0 == strncmp(g_cref, "file:/mnt/cdrom/", 16) || 0 == strncmp(g_cref, "/mnt/cdrom/", 11))
    {
        g_from_disc = 1;
    }
    GetSystemParamStr(SYS_STR_PADHELP_RD, g_saved_cross_label, 64);
    get_tag_attr("TEXT", "text_quit", "value", g_quit_label, 64);
    g_focus = create_focus_plane2("focus");
    new defocus;
    defocus = CreateDefocus(Disp2D(0));
    SetPos(defocus, 320.0, 240.0, 0.0, 0);
    SetColor(defocus, 1.0, 1.0, 1.0, 0.0);
    AnimColor(defocus, 0.5, 0.5, 0.5, 1.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
    SetOptionI(defocus, TAG("ZSRT"), ZSORT_BACKGROUND);
    SetOptionF(defocus, TAG("ZOFS"), -1.0);
    SetOptionI(defocus, TAG("LEVL"), 3);
    new window;
    window = CreateNull(Disp2D(0));
    SetPos(window, 320.0, 240.0, 0.0, 0);
    SetOptionI(window, TAG("ZSRT"), ZSORT_BACKGROUND);
    AttachScript(window, _, "window");
    SetPubVar(window, "manager", Self());
    SetPubVar(window, "focus", g_focus);
    SetPubVar(window, "width", 580.0);
    SetPubVar(window, "height", 320.0);
    new thumb;
    thumb = CreatePlane(window);
    SetLocalScale(thumb, 150.0, 150.0, 1.0);
    SetPos(thumb, -150.0, 0.0, 0.0, 0);
    SetColor(thumb, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(thumb, TAG("FDEG"), 1);
    AttachScript(thumb, "decode");
    if (g_thumbnail[0])
    {
        SendMessageStr(thumb, TAG("IMG"), g_thumbnail);
    }
    new labels[LABEL_COUNT];
    for (new i; i < LABEL_COUNT; i++)
    {
        labels[i] = create_string_label(window);
        SetPos(labels[i], g_label_x[i], g_label_y[i], 0.0, 0);
        SetOptionI(labels[i], TAG("ZSRT"), ZSORT_OVERLAY);
        send_value(labels[i], TAG("FSIZ"), g_label_size[i]);
    }
    SetSystemParamStr(SYS_STR_PADHELP_RD, g_quit_label);
    SetSystemParam(SYS_PADHELP, 0);
    new progress_bar;
    progress_bar = CreateNull(window);
    SetPos(progress_bar, 100.0, 60.0, 0.0, 0);
    SetOptionI(progress_bar, TAG("ZSRT"), ZSORT_OVERLAY);
    AttachScript(progress_bar, _, "progress_bar");
    SetPubVar(progress_bar, "manager", Self());
    SetPubVar(progress_bar, "focus", g_focus);
    {
        new title_len = strlen(g_title);
        new line_start;
        {
            new line;
            for (; line < 3; line++)
            {
                new text[64];
                {
                    new i;
                    for (; ; i++)
                    {
                        if (line_start + i >= title_len)
                        {
                            send_text(labels[line], TAG("UTF8"), text);
                            goto title_done;
                        }
                        if (g_title{line_start + i} == '\n')
                        {
                            line_start = line_start + i + 1;
                            break;
                        }
                        text{i} = g_title{line_start + i};
                    }
                }
                send_text(labels[line], TAG("UTF8"), text);
            }
        }
    }
    title_done:
    {
        new text[256];
        get_tag_attr("TEXT", "text_download", "value", text, 256);
        send_text(labels[LABEL_ACTION], TAG("UTF8"), text);
        if (g_from_disc)
        {
            get_tag_attr("TEXT", "text_download_cdvd_caution0", "value", text, 256);
            send_text(labels[LABEL_CAUTION0], TAG("UTF8"), text);
            get_tag_attr("TEXT", "text_download_cdvd_caution1", "value", text, 256);
            send_text(labels[LABEL_CAUTION1], TAG("UTF8"), text);
        }
        else
        {
            get_tag_attr("TEXT", "text_download_caution0", "value", text, 256);
            send_text(labels[LABEL_CAUTION0], TAG("UTF8"), text);
            get_tag_attr("TEXT", "text_download_caution1", "value", text, 256);
            send_text(labels[LABEL_CAUTION1], TAG("UTF8"), text);
        }
    }
    if (0 == g_album[0])
    {
        get_tag_attr("TEXT", "text_default_album_name", "value", g_album, 256);
    }
    get_tag_attr("TEXT", "text_percent", "value", g_percent_text, 4);
    get_tag_attr("TEXT", "text_lefttime", "value", g_time_left_text, 10);
    get_tag_attr("TEXT", "text_hour", "value", g_hour_text, 4);
    get_tag_attr("TEXT", "text_minute", "value", g_minute_text, 4);
    get_tag_attr("TEXT", "text_second", "value", g_second_text, 4);
    downloadStart(g_cref, g_title_line, g_album);
    {
        new last_total;
        new last_received;
        new last_phase;
        new status;
        new progress_received;
        new progress_tick = tickcount();
        for (; ; )
        {
            new dl_phase;
            new dl_received;
            new dl_total;
            new dl_space_ok;
            status = downloadSync(dl_phase, dl_received, dl_total, dl_space_ok);
            if (dl_phase != last_phase || dl_received != last_received)
            {
                printf("status = %d, phase = %d, %d / %d\n", status, dl_phase, dl_received, dl_total);
            }
            last_received = dl_received;
            last_phase = dl_phase;
            if (status == SYNC_ERROR)
            {
                new error = downloadGetError();
                printf("error = %d\n", error);
                switch (error)
                {
                    case DL_ERR_TIMEOUT:
                    {
                        new msg[256];
                        get_tag_attr("TEXT", "msgcom_error_network_timeout", "value", msg, 256);
                        show_message(msg);
                        go_to_next();
                    }
                    case DL_ERR_LENGTH, DL_ERR_HTTP_LENGTH:
                    {
                        new msg[256];
                        new text[256];
                        get_tag_attr("TEXT", "text_download_not_completed", "value", msg, 256);
                        replace_all(text, msg, "\\n", "\n", 256);
                        msg = text;
                        new code[8];
                        sprintf(code, "%d", error);
                        replace_all(text, msg, "%ERRORNO", code, 256);
                        msg = text;
                        show_message(msg);
                        go_to_next();
                    }
                    case DL_ERR_TOO_MANY_ALBUMS:
                    {
                        new msg[256];
                        switch (get_content_type())
                        {
                            case CONTENT_MOVIE:
                            {
                                get_tag_attr("TEXT", "msgisp_movie_warn_albumcnt_max", "value", msg, 256);
                            }
                            case CONTENT_PHOTO:
                            {
                                get_tag_attr("TEXT", "msgisp_photo_warn_albumcnt_max", "value", msg, 256);
                            }
                            case CONTENT_MUSIC:
                            {
                                get_tag_attr("TEXT", "msgisp_music_warn_albumcnt_max", "value", msg, 256);
                            }
                        }
                        new text[256];
                        replace_all(text, msg, "\\n", "\n", 256);
                        msg = text;
                        show_message(msg);
                        go_to_next();
                    }
                    case DL_ERR_ALBUM_FULL:
                    {
                        new msg[256];
                        switch (get_content_type())
                        {
                            case CONTENT_MOVIE:
                            {
                                get_tag_attr("TEXT", "msgisp_movie_warn_datacnt_max", "value", msg, 256);
                            }
                            case CONTENT_PHOTO:
                            {
                                get_tag_attr("TEXT", "msgisp_photo_warn_datacnt_max", "value", msg, 256);
                            }
                            case CONTENT_MUSIC:
                            {
                                get_tag_attr("TEXT", "msgisp_music_warn_datacnt_max", "value", msg, 256);
                            }
                        }
                        new text[256];
                        replace_all(text, msg, "\\n", "\n", 256);
                        msg = text;
                        replace_all(text, msg, "%ALBUM", g_album, 256);
                        msg = text;
                        show_message(msg);
                        go_to_next();
                    }
                    default:
                    {
                        new msg[512];
                        get_tag_attr("TEXT", "text_download_error", "value", msg, 512);
                        show_error(msg, downloadGetError());
                        go_to_next();
                    }
                }
            }
            if (!dl_space_ok)
            {
                printf("no hdd space\n");
                abort_download();
                new msg[512];
                get_tag_attr("TEXT", "text_download_no_space", "value", msg, 512);
                show_message(msg);
                go_to_next();
            }
            if (0 == g_content_type)
            {
                g_content_type = get_content_type();
                if (0 == g_thumbnail[0])
                {
                    switch (get_content_type())
                    {
                        case CONTENT_MOVIE:
                        {
                            get_tag_attr("IMG", "default_icon_movie", "src", g_thumbnail, 256);
                            SendMessageStr(thumb, TAG("IMG"), g_thumbnail);
                        }
                        case CONTENT_PHOTO:
                        {
                            get_tag_attr("IMG", "default_icon_photo", "src", g_thumbnail, 256);
                            SendMessageStr(thumb, TAG("IMG"), g_thumbnail);
                        }
                        case CONTENT_MUSIC:
                        {
                            get_tag_attr("IMG", "default_icon_music", "src", g_thumbnail, 256);
                            SendMessageStr(thumb, TAG("IMG"), g_thumbnail);
                        }
                        case CONTENT_OTHER:
                        {
                            get_tag_attr("IMG", "default_icon_other", "src", g_thumbnail, 256);
                            SendMessageStr(thumb, TAG("IMG"), g_thumbnail);
                        }
                    }
                }
            }
            if (dl_total > 0 && dl_total > dl_received)
            {
                SetPubVar(progress_bar, "progress", floatround(100.0 * float(dl_received) / float(dl_total), 1));
            }
            if (dl_total > 0 && dl_total != last_total)
            {
                new size[64];
                format_size(size, dl_total, "text_kilobyte", "text_megabyte");
                send_text(labels[LABEL_SIZE], TAG("UTF8"), size);
                last_total = dl_total;
            }
            new Float:percent;
            if (dl_total > 0)
            {
                switch (dl_phase)
                {
                    case PHASE_PREPARE, PHASE_RECEIVE:
                    {
                        percent = 100.0 * float(dl_received) / float(dl_total);
                    }
                    case PHASE_WAIT_TITLE .. PHASE_DONE:
                    {
                        percent = 100.0;
                    }
                    case PHASE_FAILED:
                    {
                        goto percent_done;
                    }
                }
                new text[64];
                sprintf(text, "%2.2f %s", percent, g_percent_text);
                send_text(labels[LABEL_PERCENT], TAG("UTF8"), text);
                percent_done:
            }
            if (dl_total > 0)
            {
                static frames;
                frames = frames + 1;
                new seconds = frames_to_seconds(frames);
                static last_seconds;
                if (seconds != last_seconds)
                {
                    if (dl_phase == PHASE_RECEIVE && seconds > 5 && percent > 1.0)
                    {
                        new left = floatround(seconds * (100.0 - percent) / percent, 0);
                        new left_s = left % 60;
                        new left_m = left / 60 % 60;
                        new left_h = left / 3600;
                        new text[64];
                        if (left_h > 0)
                        {
                            sprintf(text, "%s %d %s %d %s", g_time_left_text, left_h, g_hour_text, left_m,
                                    g_minute_text);
                        }
                        else if (left_m > 0)
                        {
                            if (left_m < 10)
                            {
                                sprintf(text, "%s %d %s %d %s", g_time_left_text, left_m, g_minute_text, left_s,
                                        g_second_text);
                            }
                            else
                            {
                                sprintf(text, "%s %d %s", g_time_left_text, left_m, g_minute_text);
                            }
                        }
                        else
                        {
                            sprintf(text, "%s %d %s", g_time_left_text, left_s, g_second_text);
                        }
                        send_text(labels[LABEL_TIME_LEFT], TAG("UTF8"), text);
                    }
                    else
                    {
                        send_text(labels[LABEL_TIME_LEFT], TAG("UTF8"), "");
                    }
                }
                last_seconds = seconds;
            }
            if (status == SYNC_FINISHED)
            {
                break;
            }
            if (dl_phase == PHASE_WAIT_TITLE)
            {
                set_download_title();
            }
            if (tickcount() - progress_tick > 30000)
            {
                if (abort_download() == -1)
                {
                    new msg[256];
                    get_tag_attr("TEXT", "msgcom_error_network_timeout", "value", msg, 256);
                    show_message(msg);
                    go_to_next();
                }
                else
                {
                }
            }
            if (dl_received != progress_received)
            {
                progress_tick = tickcount();
                progress_received = dl_received;
            }
            new pressed;
            PadState(_, pressed, _, _, -1);
            if (pressed & PAD_CROSS && dl_phase == PHASE_RECEIVE)
            {
                downloadSuspend(1);
                new question[256];
                new text[256];
                get_tag_content("TEXT", "text_confirm_quit", text, 256);
                copy_string(question, g_title, 256);
                append_string(question, "\n", 256);
                append_string(question, text, 256);
                new yes[32];
                get_tag_attr("TEXT", "text_yes", "value", yes, 32);
                new no[32];
                get_tag_attr("TEXT", "text_no", "value", no, 32);
                new answer = ask_yes_no(question, yes, no, 1);
                downloadSuspend(0);
                progress_tick = tickcount();
                if (0 == answer)
                {
                    if (abort_download() == -1)
                    {
                        go_to_next();
                    }
                    else
                    {
                        break;
                    }
                }
            }
            SetSystemParam(SYS_PADHELP, dl_phase == PHASE_RECEIVE ? PADHELP_CROSS : 0);
            sleep;
        }
        SetPubVar(progress_bar, "progress", 100);
        while (SCRIPT_FINISHED != ScriptState(progress_bar))
        {
            sleep;
        }
        Destroy(progress_bar);
        progress_bar = 0;
    }
    SetSystemParam(SYS_PADHELP, 0);
    for (new i; i < LABEL_COUNT; i++)
    {
        send_text(labels[i], TAG("UTF8"), "");
    }
    {
        new Float:color[4];
        GetColor(defocus, color[0], color[1], color[2], color[3]);
        AnimColor(defocus, color[0], color[1], color[2], 0.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
        GetColor(window, color[0], color[1], color[2], color[3]);
        AnimColor(window, color[0], color[1], color[2], 0.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
        GetColor(thumb, color[0], color[1], color[2], color[3]);
        AnimColor(thumb, color[0], color[1], color[2], 0.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
    }
    downloadGetData(DL_ALBUM, g_album_name, 256);
    switch (g_content_type)
    {
        case CONTENT_MOVIE:
        {
            new msg[256];
            get_tag_content("TEXT", "text_install_finished_movie_playnow", msg, 256);
            {
                new text[256];
                replace_all(text, msg, "%ALBUM", g_album_name, 256);
                msg = text;
            }
            new yes[32];
            get_tag_attr("TEXT", "text_yes", "value", yes, 32);
            new no[32];
            get_tag_attr("TEXT", "text_no", "value", no, 32);
            if (0 == ask_yes_no(msg, yes, no, 0))
            {
                hisClear();
                new file[256];
                new args[550];
                downloadGetData(DL_FILE, file, 256);
                sprintf(args, "file=%s;bref=%s", file, g_nextref);
                new url[256];
                get_tag_attr("LINK", "movie_player", "href", url, 256);
                xmlLoadStart(url, "", args, XMLLOAD_MAIN);
                hisClear();
                wait_xml_load();
                return;
            }
            go_to_next();
        }
        case CONTENT_PHOTO:
        {
            new msg[256];
            get_tag_content("TEXT", "text_install_finished_photo_playnow", msg, 256);
            {
                new text[256];
                replace_all(text, msg, "%ALBUM", g_album_name, 256);
                msg = text;
            }
            new yes[32];
            get_tag_attr("TEXT", "text_yes", "value", yes, 32);
            new no[32];
            get_tag_attr("TEXT", "text_no", "value", no, 32);
            if (0 == ask_yes_no(msg, yes, no, 0))
            {
                hisClear();
                new file[256];
                new args[550];
                downloadGetData(DL_FILE, file, 256);
                sprintf(args, "file=%s;bref=%s", file, g_nextref);
                new url[256];
                get_tag_attr("LINK", "photo_viewer", "href", url, 256);
                xmlLoadStart(url, "", args, XMLLOAD_MAIN);
                hisClear();
                wait_xml_load();
                return;
            }
            go_to_next();
        }
        case CONTENT_MUSIC:
        {
            new msg[256];
            get_tag_content("TEXT", "text_install_finished_music_playnow", msg, 256);
            {
                new text[256];
                replace_all(text, msg, "%ALBUM", g_album_name, 256);
                msg = text;
            }
            new yes[32];
            get_tag_attr("TEXT", "text_yes", "value", yes, 32);
            new no[32];
            get_tag_attr("TEXT", "text_no", "value", no, 32);
            if (0 == ask_yes_no(msg, yes, no, 0))
            {
                hisClear();
                new folder[256];
                new args[550];
                downloadGetData(DL_FILE, folder, 256);
                sprintf(args, "autostart=-1;folder=%s;bref=%s", folder, g_nextref);
                new url[256];
                get_tag_attr("LINK", "music_player", "href", url, 256);
                xmlLoadStart(url, "", args, XMLLOAD_MAIN);
                hisClear();
                wait_xml_load();
                return;
            }
            go_to_next();
        }
    }
}

new g_no_post[1];
new g_no_args[1];

/* Leaves the page for nextref. */
go_to_next()
{
    hisClear();
    xmlLoadStart(g_nextref, g_no_post, g_no_args, XMLLOAD_MAIN);
    wait_xml_load();
}

/* Asks msg with the buttons yes and no, select first; returns 0 for yes, 1 for no or cross. */
ask_yes_no(msg[], yes[], no[], select)
{
    new pad_help = GetSystemParam(SYS_PADHELP);
    SetSystemParamStr(SYS_STR_PADHELP_RD, g_saved_cross_label);
    new args[512];
    sprintf(args, "msg=%s;yes=%s;no=%s", msg, yes, no);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 240.0, 0.0, 0);
    AttachScript(dialog, _, "confirm_yesno", args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", g_focus);
    SetPubVar(dialog, "select", select);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    new answer = GetPubVar(dialog, "select");
    Destroy(dialog);
    if (answer == -1)
    {
        answer = 1;
    }
    SetSystemParam(SYS_PADHELP, pad_help);
    SetSystemParamStr(SYS_STR_PADHELP_RD, g_quit_label);
    return answer;
}

/* Shows msg in a dialog and waits until it is closed. */
show_message(msg[])
{
    new pad_help = GetSystemParam(SYS_PADHELP);
    SetSystemParamStr(SYS_STR_PADHELP_RD, g_saved_cross_label);
    new args[512];
    sprintf(args, "msg=%s", msg);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 240.0, 0.0, 0);
    AttachScript(dialog, _, "confirm_d", args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", g_focus);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    Destroy(dialog);
    SetSystemParam(SYS_PADHELP, pad_help);
    SetSystemParamStr(SYS_STR_PADHELP_RD, g_quit_label);
}

/* Like show_message, as an error, with the HTTP or internal error code if not 0. */
show_error(msg[], code)
{
    new pad_help = GetSystemParam(SYS_PADHELP);
    SetSystemParamStr(SYS_STR_PADHELP_RD, g_saved_cross_label);
    new args[512];
    sprintf(args, "msg=%s", msg);
    if (code)
    {
        new prefix[64];
        if (0 < code)
        {
            get_tag_attr("TEXT", "msgcom_error_http", "value", prefix, 64);
        }
        else
        {
            get_tag_attr("TEXT", "msgcom_error_internal", "value", prefix, 64);
        }
        new number[64];
        sprintf(number, "%d", code);
        append_string(args, "\n", 512);
        append_string(args, "(", 512);
        append_string(args, prefix, 512);
        append_string(args, number, 512);
        append_string(args, ")", 512);
    }
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 240.0, 0.0, 0);
    AttachScript(dialog, _, "confirm_err", args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", g_focus);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    Destroy(dialog);
    SetSystemParam(SYS_PADHELP, pad_help);
    SetSystemParamStr(SYS_STR_PADHELP_RD, g_quit_label);
}

/* The script of the thumbnail: decodes the image of IMG (url) into its texture, then ends. */
public decode()
{
    for (; ; )
    {
        new msg;
        new url[256];
        while (RecvMessageStr(msg, url, 256))
        {
            if (msg == TAG("IMG"))
            {
                printf("thumbnail = %s\n", url);
                imgDecStart(url, 256, 256, -1, 100, 0);
                new status = wait_image_decode();
                if (status == IMG_DONE)
                {
                    SetTexture(Self(), imgTexID(), 0, -1);
                    new Float:aspect = 1.0 * imgTexOrgWidth() / imgTexOrgHeight();
                    if (aspect > 1.0)
                    {
                        SetScale(Self(), 1.0, 1.0 / aspect, 1.0);
                    }
                    else
                    {
                        SetScale(Self(), 1.0 * aspect, 1.0, 1.0);
                    }
                    AnimColor(Self(), 1.0, 1.0, 1.0, 1.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
                }
                Abort(Self());
            }
        }
        sleep;
    }
}

/* Waits for the data that moviedataReadStart asked for; returns the status of moviedataReadSync. */
wait_moviedata(title[], maxlen)
{
    for (; ; )
    {
        new result = moviedataReadSync(title, maxlen);
        if (0 != result)
        {
            return result;
        }
        sleep;
    }
    return 0;
}

get_psm_title(file[], psm_title[], maxlen)
{
    printf("get_psm_title 0\n");
    moviedataReadStart(file, 0, 0);
    printf("get_psm_title 1\n");
    if (wait_moviedata(psm_title, maxlen) > 0)
    {
        printf("psm title : %s\n", psm_title);
        return 1;
    }
    printf("get_psm_title 2\n");
    return 0;
}

/* Sets the title to save the download with: the title of a PSM movie, else the title on one line. */
set_download_title()
{
    if (get_content_type() == CONTENT_MOVIE)
    {
        new psm_title[256];
        new tmp_file[256];
        downloadGetData(DL_TMP_FILE, tmp_file, 256);
        printf("tmp file = %s\n", tmp_file);
        new found = get_psm_title(tmp_file, psm_title, 256);
        if (found)
        {
            downloadSetData(DL_TITLE, psm_title);
        }
        else
        {
            downloadSetData(DL_TITLE, g_title_line);
        }
    }
    else
    {
        downloadSetData(DL_TITLE, g_title_line);
    }
}

frames_to_seconds(frames)
{
    return frames / FrameRate();
}

create_string_label(parent)
{
    new label = CreatePlane(parent);
    SetColor(label, 1.0, 1.0, 1.0, 0.0);
    AttachScript(label, "lw_label_self");
    return label;
}

send_value(obj, msg, number)
{
    SendMessage(obj, msg, number, 0, 0, 0);
}

send_text(obj, msg, text[])
{
    SendMessageStr(obj, msg, text);
}

/* The script of a text label: draws the text of "UTF8" (text) with the size of "FSIZ" (size). */
public lw_label_self()
{
    new dirty;
    new font_size;
    new text[64];
    for (; ; )
    {
        new msg;
        new number;
        new str[64];
        while (RecvMessage(msg, number))
        {
            if (msg == TAG("FSIZ"))
            {
                font_size = number;
            }
        }
        while (RecvMessageStr(msg, str, 64))
        {
            if (msg == TAG("UTF8"))
            {
                text = str;
                dirty = 1;
            }
        }
        if (dirty)
        {
            imgDecStringStart(text, font_size, 64, 64, 0);
            if (wait_image_decode() == IMG_DONE)
            {
                SetTexture(Self(), imgTexID(), 0, -1);
                SetLocalScale(Self(), float(imgTexWidth()), float(imgTexHeight()), 1.0);
                SetColor(Self(), 1.0, 1.0, 1.0, 1.0);
            }
            dirty = 0;
        }
        sleep;
    }
}

#include "button.inc"
