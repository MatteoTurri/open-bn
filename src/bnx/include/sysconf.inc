/*
 * The items of the settings menus mapped to the system configuration, the label focus message, and the record of
 * the current channel.
 */

/* Sends the focus parameters of a label to a focus object ("FOC0" to "FOC2"). */
stock send_label_focus(obj, target, z_offset, Float:sharp, Float:extra_w, Float:extra_h, Float:move_time, smoothing,
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

/*
 * Forgets the PS1 settings and records channel as the current one in the aval file (three attempts); for "Top" the
 * top menu comes back with its guide item selected. Returns the result of the last write.
 */
stock write_current_channel(channel[])
{
    new tries;
    new ret;
    avalDelete("PS1DiscSpeed");
    avalDelete("PS1TextureMapping");
    if (0 == strcmp(channel, "Top"))
    {
        avalSet("ChannelIntoID", "PSBB-GUIDE");
        avalSet("ChannelCategory", "GROUP-TOPMENU");
    }
    avalSet("CurrentCh", channel);
    tries = 0;
    for (; tries < 3; tries++)
    {
        ret = avalWriteFile();
        if (ret >= 0)
        {
            break;
        }
    }
    return ret;
}

/* The groups of the settings (the menu sysconf_menu, the images sysconf_img_<group>). */
const GROUP_CONSOLE = 0;
const GROUP_CLOCK = 1;
const GROUP_SCREEN = 2;
const GROUP_SOUND = 3;
const GROUP_LANGUAGE = 4;
const GROUP_TEXT_INPUT = 5; /* ATOK */
const GROUP_KEYBOARD = 6;
const GROUP_MOUSE = 7;
const GROUP_REMOTE = 8;
/* The items of each group (the menus sysconf_menu_<group>, sysconf_menu_<group>_<sub> for a subgroup). */
const CONSOLE_MODEL = 0;
const CONSOLE_MAC_ADDRESS = 1;
const CONSOLE_DVD_PLAYER = 2;
const CONSOLE_PS1_DRIVER = 3;
const CONSOLE_AUTO_DIAG = 4;
const DVD_PROGRESSIVE = 0; /* the reset of the progressive setting */
const CLOCK_SET = 0;
const CLOCK_TIME_NOTATION = 1;
const CLOCK_DATE_NOTATION = 2;
const CLOCK_SUMMER_TIME = 3;
const CLOCK_TIME_ZONE = 4;
const SCREEN_SIZE = 0;
const SCREEN_COMPONENT_OUT = 1;
const SOUND_OPTICAL_OUT = 0;
const LANGUAGE_DISPLAY = 0;
const INPUT_KANA_MODE = 0; /* romaji or kana input */
const INPUT_KEY_BIND = 1;
const KEYBOARD_TYPE = 0;
const KEYBOARD_REPEAT_WAIT = 1;
const KEYBOARD_REPEAT_SPEED = 2;
const MOUSE_HAND = 0;
const MOUSE_SPEED = 1;
const MOUSE_DOUBLE_CLICK = 2;
const REMOTE_GAME_PLAY = 0; /* also the item of the remote control subgroup */
/* The subgroups of the console group (sub): the item that opens each, or none. */
const SUBGROUP_NONE = -1;
const SUBGROUP_REMOTE = 0;     /* the remote control: CONSOLE_MODEL */
const SUBGROUP_DVD_PLAYER = 2; /* CONSOLE_DVD_PLAYER: the progressive output */

/*
 * Returns the value of a settings menu item as the menu shows it, -1 for an unknown item. Where the menu lists the
 * values in the other order, 1 - x and 2 - x reverse them, and x ^ 3 swaps the date notations 1 and 2.
 */
stock get_sysconf_item(group, sub, item)
{
    new notation;
    switch (group)
    {
        case GROUP_CONSOLE:
        {
            switch (sub)
            {
                case SUBGROUP_NONE:
                {
                    switch (item)
                    {
                        case CONSOLE_AUTO_DIAG:
                        {
                            return 1 - scnfGetAutoDiag();
                        }
                    }
                }
                case SUBGROUP_REMOTE:
                {
                    switch (item)
                    {
                        case REMOTE_GAME_PLAY:
                        {
                            if (scnfGetRmGA() == 1)
                            {
                                return 0;
                            }
                            return 1;
                        }
                    }
                }
                case SUBGROUP_DVD_PLAYER:
                {
                    switch (item)
                    {
                        case DVD_PROGRESSIVE:
                        {
                            if (scnfGetDVDProg() == 1)
                            {
                                return 1;
                            }
                            return 0;
                        }
                    }
                }
            }
        }
        case GROUP_CLOCK:
        {
            switch (item)
            {
                case CLOCK_SET:
                {
                    return -1;
                }
                case CLOCK_TIME_NOTATION:
                {
                    return 1 - scnfGetTimeNotation();
                }
                case CLOCK_DATE_NOTATION:
                {
                    if (0 != (notation = scnfGetDateNotation()))
                    {
                        notation = notation ^ 3;
                    }
                    return notation;
                }
                case CLOCK_SUMMER_TIME:
                {
                    return scnfGetSummerTime();
                }
                case CLOCK_TIME_ZONE:
                {
                    return scnfGetTimeZoneCity();
                }
            }
        }
        case GROUP_SCREEN:
        {
            switch (item)
            {
                case SCREEN_SIZE:
                {
                    return scnfGetAspect();
                }
                case SCREEN_COMPONENT_OUT:
                {
                    return 1 - scnfGetVideo();
                }
            }
        }
        case GROUP_SOUND:
        {
            switch (item)
            {
                case SOUND_OPTICAL_OUT:
                {
                    return scnfGetSpdif();
                }
            }
        }
        case GROUP_LANGUAGE:
        {
            switch (item)
            {
                case LANGUAGE_DISPLAY:
                {
                    return scnfGetLang();
                }
            }
        }
        case GROUP_TEXT_INPUT:
        {
            switch (item)
            {
                case INPUT_KANA_MODE:
                {
                    return scnfGetATOKMode();
                }
                case INPUT_KEY_BIND:
                {
                    return scnfGetATOKBind();
                }
            }
        }
        case GROUP_KEYBOARD:
        {
            switch (item)
            {
                case KEYBOARD_TYPE:
                {
                    return scnfGetKBType();
                }
                case KEYBOARD_REPEAT_WAIT:
                {
                    return scnfGetKBRepeatW();
                }
                case KEYBOARD_REPEAT_SPEED:
                {
                    return 2 - scnfGetKBRepeatS();
                }
            }
        }
        case GROUP_MOUSE:
        {
            switch (item)
            {
                case MOUSE_HAND:
                {
                    return scnfGetMSLr();
                }
                case MOUSE_SPEED:
                {
                    return 2 - scnfGetMSSpeed();
                }
                case MOUSE_DOUBLE_CLICK:
                {
                    return 2 - scnfGetMSDblclk();
                }
            }
        }
        case GROUP_REMOTE:
        {
            switch (item)
            {
                case REMOTE_GAME_PLAY:
                {
                    if (scnfGetRmGA() == 1)
                    {
                        return 0;
                    }
                    return 1;
                }
            }
        }
    }
    return -1;
}

/* Sets a settings menu item from the value the menu shows. */
stock set_sysconf_item(group, sub, item, value)
{
    switch (group)
    {
        case GROUP_CONSOLE:
        {
            switch (sub)
            {
                case SUBGROUP_NONE:
                {
                    switch (item)
                    {
                        case CONSOLE_AUTO_DIAG:
                        {
                            return scnfSetAutoDiag(1 - value);
                        }
                    }
                }
                case SUBGROUP_REMOTE:
                {
                    switch (item)
                    {
                        case REMOTE_GAME_PLAY:
                        {
                            return scnfSetRmGA(1 - value);
                        }
                    }
                }
                case SUBGROUP_DVD_PLAYER:
                {
                    switch (item)
                    {
                        case DVD_PROGRESSIVE:
                        {
                            return scnfSetDVDProg(value);
                        }
                    }
                }
            }
        }
        case GROUP_CLOCK:
        {
            switch (item)
            {
                case CLOCK_SET:
                {
                    return -1;
                }
                case CLOCK_TIME_NOTATION:
                {
                    return scnfSetTimeNotation(1 - value);
                }
                case CLOCK_DATE_NOTATION:
                {
                    if (value)
                    {
                        value = value ^ 3;
                    }
                    return scnfSetDateNotation(value);
                }
                case CLOCK_SUMMER_TIME:
                {
                    return scnfSetSummerTime(value);
                }
                case CLOCK_TIME_ZONE:
                {
                    return scnfSetTZCityAndVal(value);
                }
            }
        }
        case GROUP_SCREEN:
        {
            switch (item)
            {
                case SCREEN_SIZE:
                {
                    return scnfSetAspect(value);
                }
                case SCREEN_COMPONENT_OUT:
                {
                    return scnfSetVideo(1 - value);
                }
            }
        }
        case GROUP_SOUND:
        {
            switch (item)
            {
                case SOUND_OPTICAL_OUT:
                {
                    return scnfSetSpdif(value);
                }
            }
        }
        case GROUP_LANGUAGE:
        {
            switch (item)
            {
                case LANGUAGE_DISPLAY:
                {
                    return scnfSetLang(value);
                }
            }
        }
        case GROUP_TEXT_INPUT:
        {
            switch (item)
            {
                case INPUT_KANA_MODE:
                {
                    return scnfSetATOKMode(value);
                }
                case INPUT_KEY_BIND:
                {
                    return scnfSetATOKBind(value);
                }
            }
        }
        case GROUP_KEYBOARD:
        {
            switch (item)
            {
                case KEYBOARD_TYPE:
                {
                    return scnfSetKBType(value);
                }
                case KEYBOARD_REPEAT_WAIT:
                {
                    return scnfSetKBRepeatW(value);
                }
                case KEYBOARD_REPEAT_SPEED:
                {
                    return scnfSetKBRepeatS(2 - value);
                }
            }
        }
        case GROUP_MOUSE:
        {
            switch (item)
            {
                case MOUSE_HAND:
                {
                    return scnfSetMSLr(value);
                }
                case MOUSE_SPEED:
                {
                    return scnfSetMSSpeed(2 - value);
                }
                case MOUSE_DOUBLE_CLICK:
                {
                    return scnfSetMSDblclk(2 - value);
                }
            }
        }
        case GROUP_REMOTE:
        {
            switch (item)
            {
                case REMOTE_GAME_PLAY:
                {
                    return scnfSetRmGA(1 - value);
                }
            }
        }
    }
    return -1;
}

/* Returns the index-th item of the menu with that id, 0 if there is none. */
stock find_menu_item(menu_id[], index)
{
    new node = domFindTag(domRoot(), "menu", menu_id, 0);
    if (node)
    {
        node = domFirstChild(node, "item");
        for (; node && index; node = domNext(node, "item"), index--)
        {
        }
    }
    return node;
}

stock stop_bgm()
{
    soundBgmStop();
}

