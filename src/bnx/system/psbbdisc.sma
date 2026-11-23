/*
 * /opt0/bn/script/system/psbbdisc.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * Starts the disc in the drive, a PlayStation or PlayStation 2 game (argument BOOT=GAME) or a DVD video
 * (BOOT=DVD): a message to wait and the reboot into the disc, or the error and back to the top menu.
 */

#include <core>
#include <bn>
#include <float>

#pragma dynamic 1280

/* Values of the argument BOOT. */
const BOOT_GAME = 0;
const BOOT_DVD = 1;
const BOOT_NONE = 2; /* anything else */

#include "xmlsync.inc"

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

#include "disc.inc"

#include "misc.inc"

#include "dom.inc"

#include "sysconf.inc"

new g_msg[128];
new g_wait[32];
new g_wait2[32];
new g_power[32];
new g_tray[32];

check_game_boot()
{
    new result;
    err11012();
    sleep;
    result = err10032();
    sleep;
    err11013();
    return result;
}

/* Reboots into the game disc of that type; returns -1 if it cannot. */
boot_game(type)
{
    new result = -1;
    err11012();
    sleep;
    switch (type)
    {
        case CDVD_PS1_CD:
        {
            result = boot_ps1_disc();
        }
        case CDVD_PS1_CDDA:
        {
            result = boot_ps1_disc();
        }
        case CDVD_PS2_CD:
        {
            result = boot_ps2_disc(type);
        }
        case CDVD_PS2_CDDA:
        {
            result = boot_ps2_disc(type);
        }
        case CDVD_PS2_DVD:
        {
            result = boot_ps2_disc(type);
        }
    }
    sleep;
    err11013();
    return result;
}

/*
 * Reboots into a PlayStation disc with the PS1DiscSpeed and PS1TextureMapping settings: download slot 0 gets the
 * new boot DES keys, the others the options, the boot mode and the boot path, encrypted.
 */
boot_ps1_disc()
{
    new plain[4][32];
    new encoded[4][64];
    new texture_mapping;
    new disc_speed;
    new len;
    new cipher[256];
    new i;
    new value[4];
    disc_speed = 0;
    if (0 == avalGetValue("PS1DiscSpeed", 0, value, 4))
    {
        disc_speed = atoi(value);
        if (0 != disc_speed && 1 != disc_speed)
        {
            disc_speed = 0;
        }
    }
    texture_mapping = 0;
    if (0 == avalGetValue("PS1TextureMapping", 0, value, 4))
    {
        texture_mapping = atoi(value);
        if (0 != texture_mapping && 1 != texture_mapping)
        {
            texture_mapping = 0;
        }
    }
    if (disc_speed == 1)
    {
        disc_speed = 1;
    }
    else
    {
        disc_speed = 0;
    }
    if (texture_mapping == 1)
    {
        texture_mapping = 0x10;
    }
    else
    {
        texture_mapping = 0;
    }
    sprintf(plain[1], "Opt%08x", texture_mapping | disc_speed);
    plain[2] = "BootPs1Cd";
    if (0 != err10002(plain[3]))
    {
        return -1;
    }
    sleep;
    SetSystemParam(SYS_PADHELP, 0);
    sleep;
    get_tag_attr("TEXT", "msggame_warn_wait", "value", g_wait, 32);
    get_tag_attr("TEXT", "msggame_warn_wait2", "value", g_wait2, 32);
    get_tag_attr("TEXT", "msggame_warn_console_power", "value", g_power, 32);
    get_tag_attr("TEXT", "msggame_warn_console_disctray", "value", g_tray, 32);
    sprintf(g_msg, "%s%s\n\n%s\n%s", g_wait, g_wait2, g_power, g_tray);
    show_moment(g_msg, 5000);
    err10019(plain[0]);
    sleep;
    encode_base64(plain[0], 16, encoded[0], 64);
    sleep;
    err10020();
    sleep;
    new count;
    if (strlen(plain[3]) <= 0)
    {
        count = 3;
    }
    else
    {
        count = 4;
    }
    encoded[1] = "";
    encoded[2] = "";
    encoded[3] = "";
    i = 1;
    for (; i < count; i++)
    {
        len = err10021(plain[i], cipher);
        encode_base64(cipher, len, encoded[i], 64);
        apply_rot13(encoded[i]);
        sleep;
    }
    switch (count)
    {
        case 3:
        {
            err10022(0, encoded[0]);
            err10022(1, encoded[1]);
            err10022(2, encoded[2]);
        }
        case 4:
        {
            err10022(0, encoded[0]);
            err10022(1, encoded[1]);
            err10022(2, encoded[2]);
            err10022(3, encoded[3]);
        }
    }
    sleep;
    write_current_channel("Top");
    err10023(count);
    return 0;
}

/* Reboots into a PlayStation 2 disc, as boot_ps1_disc; returns -2 for a PS2 CD that is an update disc. */
boot_ps2_disc(type)
{
    new plain[4][32];
    new encoded[4][64];
    new options;
    new len;
    new cipher[256];
    new i;
    new result = -1;
    switch (GetSystemParam(SYS_AUTO_DIAG))
    {
        case 0:
        {
            options = 0x500;
        }
        case 1:
        {
            options = 0x400;
        }
    }
    sprintf(plain[1], "Opt%08x", options);
    switch (type)
    {
        case CDVD_PS2_CD:
        {
            plain[2] = "BootPs2Cd";
            result = err10003(plain[3]);
        }
        case CDVD_PS2_CDDA:
        {
            plain[2] = "BootPs2Cd";
            result = err10003(plain[3]);
        }
        case CDVD_PS2_DVD:
        {
            plain[2] = "BootPs2Dvd";
            result = err10004(plain[3]);
        }
        default:
        {
            result = -1;
        }
    }
    sleep;
    if (result < 0)
    {
        new disc_type = GetSystemParam(SYS_CDVD_TYPE);
        if (disc_type == CDVD_PS2_CD)
        {
            if (0 != err10034())
            {
                return -2;
            }
        }
        return -1;
    }
    SetSystemParam(SYS_PADHELP, 0);
    sleep;
    get_tag_attr("TEXT", "msggame_warn_wait", "value", g_wait, 32);
    get_tag_attr("TEXT", "msggame_warn_wait2", "value", g_wait2, 32);
    get_tag_attr("TEXT", "msggame_warn_console_power", "value", g_power, 32);
    get_tag_attr("TEXT", "msggame_warn_console_disctray", "value", g_tray, 32);
    sprintf(g_msg, "%s%s\n\n%s\n%s", g_wait, g_wait2, g_power, g_tray);
    show_moment(g_msg, 5000);
    err10024(plain[3], plain[3]);
    sleep;
    err10019(plain[0]);
    sleep;
    encode_base64(plain[0], 16, encoded[0], 64);
    sleep;
    err10020();
    sleep;
    new count = 4;
    i = 1;
    for (; i < count; i++)
    {
        len = err10021(plain[i], cipher);
        encode_base64(cipher, len, encoded[i], 64);
        apply_rot13(encoded[i]);
        sleep;
    }
    err10022(0, encoded[0]);
    err10022(1, encoded[1]);
    err10022(2, encoded[2]);
    err10022(3, encoded[3]);
    sleep;
    write_current_channel("Top");
    err10023(count);
    return 0;
}

/* Shows msg for life ms. */
show_moment(msg[], life)
{
    new args[64];
    sprintf(args, "msg=%s", msg);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 240.0, 0.0, 0);
    AttachScript(dialog, _, "confirm_moment", args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "life", life);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
}

/* Fills the download slots of the reboot into a DVD video; returns -1 if there is no DVD player. */
arm_dvd_boot()
{
    new plain[2][32];
    new encoded[2][64];
    new len;
    new buffer[256];
    new i;
    err11012();
    buffer = "";
    sverGetDVDVer(buffer);
    sleep;
    err11013();
    if (strlen(buffer) <= 2)
    {
        return -1;
    }
    err11012();
    err10019(plain[0]);
    sleep;
    encode_base64(plain[0], 16, encoded[0], 64);
    sleep;
    err10020();
    sleep;
    plain[1] = "BootDvdVideo";
    i = 1;
    for (; i < 2; i++)
    {
        len = err10021(plain[i], buffer);
        encode_base64(buffer, len, encoded[i], 64);
        apply_rot13(encoded[i]);
        sleep;
    }
    err10022(0, encoded[0]);
    err10022(1, encoded[1]);
    sleep;
    err11013();
    return 0;
}

/* Shows the message to wait and reboots into the DVD video. */
boot_dvd()
{
    new msg[128];
    new wait[32];
    new wait2[32];
    new power[32];
    new tray[32];
    err11012();
    get_tag_attr("TEXT", "msgmovie_warn_wait", "value", wait, 32);
    get_tag_attr("TEXT", "msgmovie_warn_wait2", "value", wait2, 32);
    get_tag_attr("TEXT", "msgmovie_warn_console_power", "value", power, 32);
    get_tag_attr("TEXT", "msgmovie_warn_console_disctray", "value", tray, 32);
    sprintf(msg, "%s\n%s\n\n%s\n%s", wait, wait2, power, tray);
    show_moment2(msg, 5000);
    write_current_channel("Top");
    scnfwriteDVDProgBit();
    err10023(2);
    err11013();
}

/* Same as show_moment. */
show_moment2(msg[], life)
{
    new args[64];
    sprintf(args, "msg=%s", msg);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 240.0, 0.0, 0);
    AttachScript(dialog, _, "confirm_moment", args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "life", life);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
}

new g_boot_mode;
new g_disc_type;

/* Checks that the disc can be started for g_boot_mode; returns 0, or -1 or -2 (no DVD player). */
check_disc()
{
    new result;
    if (BOOT_GAME == g_boot_mode)
    {
        result = check_game_boot();
        if (0 == result)
        {
            g_boot_mode = BOOT_GAME;
        }
        else
        {
            result = -1;
        }
    }
    else if (g_boot_mode == BOOT_DVD)
    {
        result = arm_dvd_boot();
        if (result < 0)
        {
            result = -2;
        }
        g_disc_type = GetSystemParam(SYS_CDVD_TYPE);
        if (CDVD_DVD_VIDEO != g_disc_type && CDVD_DVD_VR != g_disc_type)
        {
            result = -1;
        }
    }
    return result;
}

start_game()
{
    return boot_game(g_disc_type);
}

start_dvd()
{
    return boot_dvd();
}

/* Shows why the disc did not start for mode, from result and the disc in the drive. */
show_boot_error(mode, result)
{
    new msg[64];
    g_disc_type = GetSystemParam(SYS_CDVD_TYPE);
    if (mode == BOOT_NONE)
    {
        if (CDVD_NO_DISC == g_disc_type)
        {
            get_tag_attr("TEXT", "msggame_error_nodisc", "value", msg, 64);
        }
        else
        {
            get_tag_attr("TEXT", "msggame_execdisc_loadingfail", "value", msg, 64);
        }
        show_message(msg);
    }
    else if (BOOT_GAME == mode)
    {
        if (CDVD_NO_DISC == g_disc_type)
        {
            get_tag_attr("TEXT", "msggame_error_nodisc", "value", msg, 64);
        }
        else
        {
            switch (result)
            {
                case -2:
                {
                    get_tag_attr("TEXT", "msggame_error_notplaydisc", "value", msg, 64);
                }
                default:
                {
                    get_tag_attr("TEXT", "msggame_execdisc_loadingfail", "value", msg, 64);
                }
            }
        }
        show_message(msg);
    }
    else if (mode == BOOT_DVD)
    {
        switch (result)
        {
            case -1:
            {
                if (CDVD_NO_DISC == g_disc_type)
                {
                    get_tag_attr("TEXT", "msgmovie_dvd_none", "value", msg, 64);
                }
                else
                {
                    get_tag_attr("TEXT", "msgmovie_dvd_notplay", "value", msg, 64);
                }
            }
            case -2:
            {
                get_tag_attr("TEXT", "msgmovie_dvd_noboot", "value", msg, 64);
            }
        }
        show_message(msg);
    }
}

main()
{
    start_top_page(0);
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 0);
    SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
    {
        new value[64];
        GetArg("BOOT", value, 64);
        if (!strcmp(value, "GAME"))
        {
            g_boot_mode = BOOT_GAME;
        }
        else if (!strcmp(value, "DVD"))
        {
            g_boot_mode = BOOT_DVD;
        }
        else
        {
            g_boot_mode = BOOT_NONE;
        }
        GetArg("MEDIA", value, 64);
        g_disc_type = atoi(value);
    }
    new result = check_disc();
    if (result >= 0)
    {
        switch (g_boot_mode)
        {
            case BOOT_GAME:
            {
                result = start_game();
            }
            case BOOT_DVD:
            {
                result = start_dvd();
            }
            case BOOT_NONE:
            {
                result = -1;
            }
        }
    }
    show_boot_error(g_boot_mode, result);
    xmlLoadStart("$$BUILDIN_TOPMENU$$", "", "", XMLLOAD_MAIN);
    wait_xml_load();
}

/* Shows msg in a dialog and waits until it is closed. */
show_message(msg[])
{
    new args[128];
    sprintf(args, "msg=%s", msg);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 240.0, 0.0, 0);
    AttachScript(dialog, _, "confirm_d", args);
    SetPubVar(dialog, "manager", Self());
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    Destroy(dialog);
}
