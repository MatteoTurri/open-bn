/*
 * /opt0/bn/script/movie/movie_import.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * Moves the movies downloaded to file:/net/download (RealMedia and PSS files) into a new album on the hard disk,
 * after asking; sql_process, on an object of its own, adds a row of the movie database for each file moved.
 */

#include <core>
#include <bn>

#pragma dynamic 1500

#include "moviedb.inc"

/* As in text.inc. */
const EDIT_MODE_USER_ID = 1;
const EDIT_MODE_PASSWORD = 2;
const EDIT_MODE_KATAKANA = 3;
const EDIT_MODE_NUMBER = 4;
const EDIT_MODE_KATAKANA_FIXED = 7;
const EDIT_MODE_QWERTY = 9;

/* The fields of datGetData for a file found by datFind. */
const FILE_COL_NAME = 1;
const FILE_COL_PATH = 2;
const FILE_COL_TYPE = 4; /* the content type */

#include "wait.inc"

#include "tag.inc"

#include <float>

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

#include "sound.inc"

#include "xmlsync.inc"

/* Same as get_tag_attr2 of text.inc. */
get_tag_attr2(tag[], id[], attr[], value[], maxlen)
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

#include "wndvar.inc"

#include "imgdec.inc"

#include "misc.inc"

new g_db_name[] = "movie";

/* Same as run_edit_dialog of text.inc. */
run_edit_dialog(title[], text[], edit_mode, maxlen, max_chars)
{
    new frame_tex = load_texture("file:/opt0/bn/script/common/texture/textfield.tm2", 0, 0, -1);
    if (maxlen * 4 < max_chars)
    {
        max_chars = maxlen * 4;
    }
    new backdrop = CreateDefocus(Disp2D(0));
    SetOptionI(backdrop, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionI(backdrop, TAG("LEVL"), 3);
    SetColor(backdrop, 1.0, 1.0, 1.0, 0.0);
    AnimColor(backdrop, 0.3, 0.3, 0.3, 1.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
    wait_ms(200);
    new title_label = CreateLabel(backdrop);
    SetPos(title_label, 95.0, 85.0, 0.0, 0);
    SetColor(title_label, 1.0, 1.0, 1.0, 1.0);
    SetColor(title_label, 1.0, 1.0, 1.0, 1.0);
    SetOptionI(title_label, TAG("HALN"), ALIGN_LEFT);
    SetOptionI(title_label, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(title_label, TAG("FSIZ"), 16);
    SetOptionS(title_label, TAG("UTF8"), title);
    new edit = CreateEdit(backdrop, 1024);
    if (edit_mode == EDIT_MODE_PASSWORD)
    {
        SetOptionI(edit, TAG("PWMD"), '*');
    }
    SetPos(edit, 100.0, 110.0, 0.0, 0);
    SetColor(edit, 1.0, 1.0, 1.0, 1.0);
    SetOptionF(edit, TAG("ZOFS"), 2.0);
    SetOptionI(edit, TAG("HALN"), ALIGN_LEFT);
    SetOptionI(edit, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(edit, TAG("FSIZ"), 15);
    SetOptionS(edit, TAG("UTF8"), text);
    SetOptionI(edit, TAG("W64 "), 396 * 64);
    SetOptionI(edit, TAG("H64 "), 20 * 64);
    SetOptionI(edit, TAG("LM64"), 60 * 64);
    SetOptionI(edit, TAG("RM64"), 60 * 64);
    {
        new bg_colors[4];
        static bg_bytes[16];
        pack_color_bytes(bg_colors, bg_bytes);
        SetOptionP(edit, TAG("BGC4"), bg_colors, 4);
    }
    new frame;
    frame = CreatePlane3x3(backdrop);
    SetPos(frame, 300.0, 120.0, 0.0, 0);
    SetScale(frame, 420.0, 48.0, 1.0);
    SetColor(frame, 1.0, 1.0, 1.0, 1.0);
    SetTexture(frame, frame_tex, 0, -1);
    SetOptionI(frame, TAG("ALPH"), BLEND_ADD);
    SetOptionF(frame, TAG("ZOFS"), 1.0);
    {
        new Float:tex_margins[4] = {0.5, 0.5, 0.5, 0.5};
        SetOptionP(frame, TAG("MGNT"), tex_margins, 4);
        new Float:scale[3];
        GetScale(frame, scale[0], scale[1], scale[2]);
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
        SetOptionP(frame, TAG("MGN"), margins, 4);
    }
    save_pad_help();
    new skb;
    /* The keyboard, the input mode and the lock flags of skbStart are those of TextIM::Open of textim.cpp. */
    switch (edit_mode)
    {
        case EDIT_MODE_USER_ID:
        {
            new ascii_chars[96] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q',
                    'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', ' ', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
                    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '1', '2', '3', '4',
                    '5', '6', '7', '8', '9', '0', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '-', '=', '\\', '`',
                    '_', '+', '|', '~', '[', ']', '{', '}', ';', ':', '\'', '"', ',', '.', '<', '>', '/', '?', 0};
            skb = skbStart(edit, SKB_KEYBOARD_QWERTY, SKB_INPUT_DIRECT,
                    SKB_LOCK_KEYBOARD | SKB_LOCK_INPUT_MODE | SKB_KANA_LOCK_FIXED, SKB_LENGTH_BYTES, max_chars,
                    ascii_chars, 1);
        }
        case EDIT_MODE_PASSWORD:
        {
            new password_chars[96] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
                    'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', ' ', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',
                    'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '1', '2', '3',
                    '4', '5', '6', '7', '8', '9', '0', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '-', '=', '\\',
                    '`', '_', '+', '|', '~', '[', ']', '{', '}', ';', ':', '\'', '"', ',', '.', '<', '>', '/', '?', 0};
            skb = skbStart(edit, SKB_KEYBOARD_QWERTY, SKB_INPUT_DIRECT,
                    SKB_LOCK_KEYBOARD | SKB_LOCK_INPUT_MODE | SKB_KANA_LOCK_FIXED, SKB_LENGTH_BYTES, max_chars,
                    password_chars, 1);
        }
        case EDIT_MODE_KATAKANA:
        {
            skb = skbStart(edit, SKB_KEYBOARD_KATAKANA, SKB_INPUT_KATAKANA,
                    SKB_LOCK_KEYBOARD | SKB_LOCK_INPUT_MODE | SKB_KANA_LOCK_FIXED, SKB_LENGTH_BYTES, max_chars, _, 0);
        }
        case 5:
        {
            skb = skbStart(edit, SKB_KEYBOARD_KATAKANA, SKB_INPUT_KATAKANA_FIXED,
                    SKB_LOCK_KEYBOARD | SKB_LOCK_INPUT_MODE | SKB_KANA_LOCK_FIXED | SKB_FIXED_INPUT, SKB_LENGTH_BYTES,
                    max_chars, _, 0);
        }
        case 6:
        {
            skb = skbStart(edit, SKB_KEYBOARD_KATAKANA, SKB_INPUT_HALF_KATAKANA_FIXED,
                    SKB_LOCK_KEYBOARD | SKB_LOCK_INPUT_MODE | SKB_KANA_LOCK_FIXED | SKB_FIXED_INPUT | SKB_HALF_WIDTH,
                    SKB_LENGTH_BYTES, max_chars, _, 0);
        }
        case EDIT_MODE_KATAKANA_FIXED:
        {
            skb = skbStart(edit, SKB_KEYBOARD_KATAKANA, SKB_INPUT_KATAKANA_FIXED,
                    SKB_LOCK_KEYBOARD | SKB_LOCK_INPUT_MODE | SKB_KANA_LOCK_FIXED | SKB_FIXED_INPUT | SKB_FULL_WIDTH,
                    SKB_LENGTH_BYTES, max_chars, _, 0);
        }
        case 8:
        {
            skb = skbStart(edit, SKB_KEYBOARD_HIRAGANA, SKB_INPUT_HIRAGANA_FIXED,
                    SKB_LOCK_KEYBOARD | SKB_LOCK_INPUT_MODE | SKB_KANA_LOCK_FIXED | SKB_FIXED_INPUT, SKB_LENGTH_BYTES,
                    max_chars, _, 0);
        }
        case EDIT_MODE_NUMBER:
        {
            new digits[11] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 0};
            skb = skbStart(edit, SKB_KEYBOARD_NUMBERS, SKB_INPUT_DIRECT,
                    SKB_LOCK_KEYBOARD | SKB_LOCK_INPUT_MODE | SKB_KANA_LOCK_FIXED, SKB_LENGTH_BYTES, max_chars, digits,
                    1);
        }
        case EDIT_MODE_QWERTY:
        {
            printf("_EDIT_MODE_ASCII == option = %x\n", SKB_LOCK_INPUT_MODE | SKB_HALF_WIDTH);
            skb = skbStart(edit, SKB_KEYBOARD_ALPHANUMERIC, SKB_INPUT_DIRECT, SKB_LOCK_INPUT_MODE | SKB_HALF_WIDTH,
                    SKB_LENGTH_BYTES, max_chars, _, 0);
        }
        default:
        {
            skb = skbStart(edit, SKB_KEYBOARD_DEFAULT, SKB_INPUT_DEFAULT, 0, SKB_LENGTH_BYTES, max_chars, _, 0);
        }
    }
    new ok;
    if (skb == -1)
    {
        printf("edit:");
        getstring(text, maxlen);
        sleep;
        ok = 1;
    }
    else if (wait_skb() == 1)
    {
        skbGetString(text, maxlen);
        ok = 1;
    }
    else
    {
        ok = 0;
    }
    Destroy(title_label);
    Destroy(edit);
    Destroy(frame);
    AnimColor(backdrop, 0.3, 0.3, 0.3, 0.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
    wait_ms(200);
    Destroy(backdrop);
    restore_pad_help();
    return ok;
}

#include "dom.inc"

#include "pfs.inc"

#include "background.inc"

new g_sql[128];
new g_album_name[32];
new g_album_uri[64];
new g_text[64];
new g_title[64];
new g_album_date[8];
new g_album_id[128];
new g_album_year[1];
new g_album_month[1];
new g_album_day[1];
new g_album_hour[1];
new g_album_minute[1];
new g_album_second;

/*
 * Adds an album (a favorite album for ALBUM_FAVORITE) named name, or the first free "<new album>N", asked with the
 * software keyboard if ask_name; an album gets a folder of pfs:/__contents/contents/video named by its date. Sets
 * the window variables title, uri and album_id; returns 0 or ADD_ALBUM_*.
 */
add_album(album_mode, ask_name, name[])
{
    new status;
    new count;
    new i;
    status = datOpen(DAT_DATABASE, g_db_name, DAT_ORDER_MTIME_DESC);
    if (0 != status)
    {
        return ADD_ALBUM_FAILED;
    }
    status = wait_database();
    if (status < 0)
    {
        return ADD_ALBUM_FAILED;
    }
    if (ALBUM_FAVORITE != album_mode)
    {
        sprintf(g_sql, "select * from sce_movie where content_attr = %d", ATTR_ALBUM);
    }
    else
    {
        sprintf(g_sql, "select * from sce_movie where content_attr = %d", ATTR_FAV_ALBUM);
    }
    status = datSQLExec(g_sql);
    if (0 != status)
    {
        return ADD_ALBUM_FAILED;
    }
    status = wait_database();
    if (status < 0)
    {
        return ADD_ALBUM_FAILED;
    }
    count = datGetNum();
    if (count >= MAX_ITEMS)
    {
        return ADD_ALBUM_FULL;
    }
    g_text = "";
    if (ALBUM_FAVORITE != album_mode)
    {
        get_tag_attr2("TEXT", "msgmovie_add_album_new", "value", g_text, 64);
    }
    else
    {
        get_tag_attr2("TEXT", "msgmovie_favalbum", "value", g_text, 64);
    }
    err20007();
    i = 0;
    for (; i < count; i++)
    {
        datGetData(i, COL_TITLE, g_title, 64);
        err20008(g_title);
        if (0 == i % 200)
        {
            sleep;
        }
    }
    i = 0;
    for (; i < MAX_ITEMS; i++)
    {
        g_album_name = "";
        if (ALBUM_FAVORITE != album_mode)
        {
            if (strlen(name) > 0)
            {
                sprintf(g_album_name, name);
            }
            else
            {
                sprintf(g_album_name, "%s%d", g_text, i + 1);
            }
        }
        else
        {
            sprintf(g_album_name, "%s%d", g_text, i + 1);
        }
        if (0 == err20009(g_album_name))
        {
            break;
        }
        if (0 == i % 50)
        {
            sleep;
        }
    }
    err20007();
    if (0 != ask_name)
    {
        get_tag_attr2("TEXT", "msgmovie_albumname", "value", g_album_id, 128);
        sprintf(g_text, g_album_name);
        if (!run_edit_dialog(g_album_id, g_text, 0, 64, 256))
        {
            return ADD_ALBUM_FAILED;
        }
        if (strlen(g_text) > 0)
        {
            escape_sql(g_album_name, g_text, 32);
        }
        else
        {
            return ADD_ALBUM_NO_NAME;
        }
    }
    date(g_album_year[0], g_album_month[0], g_album_day[0], 0);
    time(g_album_hour[0], g_album_minute[0], g_album_second, 0);
    sprintf(g_album_date, "%04d%02d%02d%02d%02d%02d", g_album_year, g_album_month, g_album_day, g_album_hour,
            g_album_minute, g_album_second);
    if (ALBUM_FAVORITE != album_mode)
    {
        sprintf(g_text, "pfs:/__contents/contents/video");
        fileMkdir(g_text);
        sprintf(g_album_uri, "pfs:/__contents/contents/video/%s", g_album_date);
        fileMkdir(g_album_uri);
        sprintf(g_album_id, "%s%s%s", g_album_name, g_album_date, g_album_uri);
        sprintf(g_sql,
                "insert into sce_movie (title,album_name,create_date,uri,album_id,content_type,content_subtype,content_attr,table_version) values('%s','%s','%s','%s','%s',%d,%d,%d,%d)",
                g_album_name, g_album_name, g_album_date, g_album_uri, g_album_id, 0, 0, ATTR_ALBUM, 512);
    }
    else
    {
        sprintf(g_album_uri, "%s%s", g_album_name, g_album_date);
        sprintf(g_sql,
                "insert into sce_movie (title,album_name,create_date,uri,album_id,content_type,content_subtype,content_attr,table_version) values('%s','%s','%s','%s','%s',%d,%d,%d,%d)",
                g_album_name, g_album_name, g_album_date, g_album_uri, g_album_id, 0, 0, ATTR_FAV_ALBUM, 512);
    }
    status = datSQLExec(g_sql);
    if (0 != status)
    {
        return ADD_ALBUM_FAILED;
    }
    status = wait_database();
    if (status < 0)
    {
        return ADD_ALBUM_FAILED;
    }
    set_wnd_var_str("title", g_album_name);
    set_wnd_var_str("uri", g_album_uri);
    set_wnd_var_str("album_id", g_album_id);
    return 0;
}

public focus_glowfly; /* the focus object */
new g_dest_path[64];
new g_src_path[64];
new g_file_name[64];
new g_import_uri[64];
new g_movie_title[32];
new g_import_album[32];
new g_movie_uri[64];
new g_import_album_id[64];
new g_movie_date[8];
new g_movie_type[4];
new g_sql2[64];
new g_file_type[1];
new g_file_type_text[8];
new g_movie_year[1];
new g_movie_month[1];
new g_movie_day[1];
new g_movie_hour[1];
new g_movie_minute[1];
new g_movie_second;

/*
 * Copies src into dest doubling each single quote and backslash, for a quoted SQL string; returns -1 when it wrote
 * maxlen characters or more.
 */
escape_sql(dest[], src[], maxlen)
{
    new len = strlen(src);
    new out;
    for (new i; i < len && out < maxlen * 4 - 2; i++)
    {
        if (src{i} == '\'')
        {
            dest{out++} = '\'';
            dest{out++} = '\'';
        }
        else if (src{i} == '\\')
        {
            dest{out++} = '\\';
            dest{out++} = '\\';
        }
        else
        {
            dest{out++} = src{i};
        }
    }
    dest{out++} = 0;
    if (out >= maxlen)
    {
        return -1;
    }
    return 0;
}

/* Shows msg in a dialog and waits until it is closed. */
show_message(msg[])
{
    new args[128];
    sprintf(args, "msg=%s", msg);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 224.0, 0.0, 0);
    SetOptionI(dialog, TAG("ZSRT"), ZSORT_OVERLAY);
    AttachScript(dialog, _, "confirm_d", args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", focus_glowfly);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    Destroy(dialog);
}

/* Asks msg with a yes/no dialog; returns its select, 0 for yes (1 when it is closed without an answer). */
ask_yes_no(msg[], yes[], no[])
{
    new args[512];
    sprintf(args, "msg=%s;yes=%s;no=%s", msg, yes, no);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 224.0, 0.0, 0);
    AttachScript(dialog, _, "confirm_yesno", args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", focus_glowfly);
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
    return answer;
}

/*
 * Waits for three "TITL" strings (album title, album uri, album id), then adds a row of sce_movie for each "FILE"
 * (name), "URL" (path) and "TYPE" (content type) that import_process sends, titled from the movie file when it has
 * a title, until "FNSH".
 */
public sql_process()
{
    new msg;
    new value;
    new status;
    status = datOpen(DAT_DATABASE, g_db_name, DAT_ORDER_MTIME_DESC);
    if (0 != status)
    {
        return -1;
    }
    status = wait_database();
    if (status < 0)
    {
        return -1;
    }
    g_import_album = "";
    wait_message_str(TAG("TITL"), g_import_album, 32);
    g_movie_uri = "";
    wait_message_str(TAG("TITL"), g_movie_uri, 64);
    g_import_album_id = "";
    wait_message_str(TAG("TITL"), g_import_album_id, 64);
    for (; ; )
    {
        if (FindMessageStr(TAG("FILE"), g_movie_title, 32))
        {
            if (FindMessageStr(TAG("URL"), g_movie_uri, 64))
            {
                if (FindMessageStr(TAG("TYPE"), g_movie_type, 4))
                {
                    moviedataReadStart(g_movie_uri, 0, 0);
                    if (wait_moviedata(g_sql2, 64) > 0)
                    {
                        escape_sql(g_movie_title, g_sql2, 32);
                    }
                    date(g_movie_year[0], g_movie_month[0], g_movie_day[0], 0);
                    time(g_movie_hour[0], g_movie_minute[0], g_movie_second, 0);
                    sprintf(g_movie_date, "%04d%02d%02d%02d%02d%02d", g_movie_year, g_movie_month, g_movie_day,
                            g_movie_hour, g_movie_minute, g_movie_second);
                    sprintf(g_sql2,
                            "insert into sce_movie (title,album_name,create_date,uri,album_id,content_type,content_subtype,content_attr,table_version) values('%s','%s','%s','%s','%s',%s,%d,%d,%d)",
                            g_movie_title, g_import_album, g_movie_date, g_movie_uri, g_import_album_id, g_movie_type,
                            0, ATTR_MOVIE, 512);
                    status = datSQLExec(g_sql2);
                    if (0 != status)
                    {
                        return -1;
                    }
                    status = wait_database();
                    if (status < 0)
                    {
                        return -1;
                    }
                }
            }
        }
        if (RecvMessage(msg, value))
        {
            if (msg == TAG("FNSH"))
            {
                break;
            }
        }
        sleep;
    }
    status = wait_database();
    if (status < 0)
    {
        return -1;
    }
    datClose();
    return 0;
}

/*
 * Asks to import the downloaded movies, creates the album and moves the files into it with a progress window;
 * returns 0, or a negative code for the step that failed or was refused.
 */
public import_process()
{
    new status;
    status = datOpen(DAT_FILE_LIST, "", DAT_ORDER_MTIME_DESC);
    if (0 != status)
    {
        return -1;
    }
    status = wait_database();
    if (status < 0)
    {
        return -2;
    }
    status = datFind("real,pss", "file:/net/download");
    if (0 != status)
    {
        return -3;
    }
    status = wait_database();
    if (status < 0)
    {
        return -4;
    }
    if (datGetNum() <= 0)
    {
        datClose();
        return -5;
    }
    new found1[64];
    new found2[64];
    new question[64];
    new msg[256];
    {
        new yes[32];
        new no[32];
        get_tag_attr2("TEXT", "msgmovie_hddimport_copy_found1", "value", found1, 64);
        get_tag_attr2("TEXT", "msgmovie_hddimport_copy_found2", "value", found2, 64);
        get_tag_attr2("TEXT", "msgmovie_hddimport_copy_ask1", "value", question, 64);
        get_tag_attr2("TEXT", "msgmovie_common_yes", "value", yes, 32);
        get_tag_attr2("TEXT", "msgmovie_common_no", "value", no, 32);
        sprintf(msg, "%s\n%s\n%s", found1, found2, question);
        new answer = ask_yes_no(msg, yes, no);
        if (0 != answer)
        {
            return -6;
        }
    }
    new progress_win = CreateNull(Disp2D(0));
    get_tag_attr2("TEXT", "msgmovie_hddimport_copy", "value", msg, 256);
    {
        new args[128];
        sprintf(args, "msg=%s;cancel=%s", msg, "");
        SetPos(progress_win, 320.0, 224.0, 0.0, 0);
        AttachScript(progress_win, _, "progress_window", args);
        SetPubVar(progress_win, "manager", Self());
        SetPubVar(progress_win, "focus", focus_glowfly);
        SetPubVar(progress_win, "progress", 0);
    }
    status = add_album(ALBUM_COLLECTION, 0, "");
    if (0 == status)
    {
        get_wnd_var_str2("title", g_import_album, 32);
        get_wnd_var_str2("uri", g_import_uri, 64);
        get_wnd_var_str2("album_id", g_import_album_id, 64);
    }
    else
    {
        if (status == ADD_ALBUM_FULL)
        {
            status = -1;
            goto finish;
        }
        status = -1;
        goto finish;
    }
    new sql_obj = CreateNull(Scene(0));
    AttachScript(sql_obj, "sql_process", "", "");
    SendMessageStr(sql_obj, TAG("TITL"), g_import_album);
    SendMessageStr(sql_obj, TAG("TITL"), g_import_uri);
    SendMessageStr(sql_obj, TAG("TITL"), g_import_album_id);
    status = datOpen(DAT_FILE_LIST, "", DAT_ORDER_MTIME_DESC);
    if (0 != status)
    {
        return -1;
    }
    status = wait_database();
    if (status < 0)
    {
        return -2;
    }
    status = datFind("real,pss", "file:/net/download");
    if (0 != status)
    {
        return -3;
    }
    status = wait_database();
    if (status < 0)
    {
        return -4;
    }
    new count = datGetNum();
    new move_status;
    new percent;
    status = 0;
    {
        new i;
        for (; i < count; i++)
        {
            datGetData(i, FILE_COL_PATH, g_src_path, 64);
            datGetData(i, FILE_COL_NAME, g_file_name, 64);
            datGetData(i, FILE_COL_TYPE, g_file_type, 1);
            sprintf(g_dest_path, "%s/%s", g_import_uri, g_file_name);
            sprintf(g_file_type_text, "%d", g_file_type);
            if (ScriptState(sql_obj) == SCRIPT_FINISHED)
            {
                status = -1;
                break;
            }
            status = fileMove(g_src_path, g_dest_path);
            if (status < 0)
            {
                goto finish;
            }
            for (; ; )
            {
                status = fileSync(percent, move_status);
                if (status == FILESYNC_DONE)
                {
                    break;
                }
                else if (0 != status)
                {
                    goto finish;
                }
                if (0 != move_status)
                {
                    goto finish;
                }
                SetPubVar(progress_win, "progress", i * 100 / count + percent / count);
                sleep;
            }
            SendMessageStr(sql_obj, TAG("FILE"), g_file_name);
            SendMessageStr(sql_obj, TAG("URL"), g_dest_path);
            SendMessageStr(sql_obj, TAG("TYPE"), g_file_type_text);
        }
    }
    while (SCRIPT_FINISHED != ScriptState(progress_win))
    {
        new progress = GetPubVar(progress_win, "progress");
        progress = progress + 1;
        progress = clamp(progress, 0, 100);
        SetPubVar(progress_win, "progress", progress);
        sleep;
    }
    SetPubVar(progress_win, "progress", 100);
    while (SCRIPT_FINISHED != ScriptState(progress_win))
    {
        SetSystemParam(SYS_PADHELP, 0);
        sleep;
    }
    finish:
    if (progress_win)
    {
        Destroy(progress_win);
    }
    if (status < 0)
    {
        get_tag_attr2("TEXT", "msgmovie_hddimport_copy_fail", "value", msg, 256);
    }
    else if (0 == move_status)
    {
        get_tag_attr2("TEXT", "msgmovie_hddimport_copy_complete", "value", msg, 256);
    }
    else
    {
        get_tag_attr2("TEXT", "msgmovie_hddimport_copy_fail", "value", msg, 256);
    }
    show_message(msg);
    SendMessage(sql_obj, TAG("FNSH"), 0, 0, 0, 0);
    Destroy(sql_obj);
    if (FILESYNC_DONE != status)
    {
        return -7;
    }
    if (0 != move_status)
    {
        return -8;
    }
    return 0;
}

/* Waits for the data that moviedataReadStart asked for; returns the status of moviedataReadSync. */
wait_moviedata(buf[], maxlen)
{
    for (; ; )
    {
        new result = moviedataReadSync(buf, maxlen);
        if (0 != result)
        {
            return result;
        }
        sleep;
    }
    return 0;
}

#include "layout.inc"
#include "title.inc"
#include "validate.inc"
