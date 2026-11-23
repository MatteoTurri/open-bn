/*
 * /opt0/bn/script/utility/init_setup.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The initial setup: reads the boot mode from the kernel command line (arguments in ROT13, Base64 and triple DES),
 * then sends the user to the steps still to do (system settings, database conversion, network settings, ISP
 * settings, system update), each of which comes back here with "mode", and at last to the top page.
 */

#include <core>
#include <bn>
#include <float>

#pragma dynamic 2560

/* The steps of the setup, in their order; the argument "mode" names the step that came back here. */
enum
{
    STEP_START,     /* "START": none yet */
    STEP_SYSCONF,   /* "InitialSetup" */
    STEP_DBCONVERT, /* "DBConvert" */
    STEP_NETCONF,   /* "NetworkSetup" */
    STEP_ISP,       /* "GetIspChannel" */
    STEP_SYSUPDATE  /* "SystemUpdate" */
};

/* The error of avalDeleteFile that is a file system error (-EIO); the native of bn always returns 0. */
const ERR_EIO = -5;

#include "tag.inc"

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

#include "xmlsync.inc"

#include "text.inc"

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

new g_common_bg_name[] = "SCEI:CommonBG";

/* Same as wait_scnf of text.inc. */
wait_scnf2()
{
    new status;
    for (; ; )
    {
        status = scnfSync();
        if (0 != status)
        {
            return status;
        }
        sleep;
    }
    return 0;
}

#include "disc.inc"

/* Loads the page of "LINK" link_id as the common background, unless the background window already shows it. */
load_common_bg(link_id[], Float:fade_time)
{
    {
        new name[256];
        if (wndGetName(WND_BG, name, 256) && 0 == strcmp(name, g_common_bg_name))
        {
            return;
        }
    }
    new url[256];
    static link_tag[2] = "LINK";
    get_tag_attr(link_tag, link_id, "href", url, 256);
    xmlLoadStart(url, g_common_bg_name, _, XMLLOAD_BG);
    wait_xml_load();
    fade_in_background(fade_time);
}

/* "STRT", the message of fade_in_background, then the permuted choice 1 of DES. */
new g_des_pc1[58] = {
    0x53545254, 0, 57, 49, 41, 33, 25, 17, 9, 1, 58, 50, 42, 34, 26, 18, 10, 2, 59, 51, 43, 35, 27,
    19, 11, 3, 60, 52, 44, 36, 63, 55, 47, 39, 31, 23, 15, 7, 62, 54, 46, 38, 30, 22, 14, 6, 61, 53,
    45, 37, 29, 21, 13, 5, 28, 20, 12, 4
};
/* The shifts of the DES key schedule, then the permutations PC-2, IP, E, P and the inverse of IP. */
new g_des_tables[272] = {
    1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1, 14, 17, 11, 24, 1, 5, 3, 28, 15, 6, 21, 10, 23,
    19, 12, 4, 26, 8, 16, 7, 27, 20, 13, 2, 41, 52, 31, 37, 47, 55, 30, 40, 51, 45, 33, 48, 44, 49,
    39, 56, 34, 53, 46, 42, 50, 36, 29, 32, 58, 50, 42, 34, 26, 18, 10, 2, 60, 52, 44, 36, 28, 20,
    12, 4, 62, 54, 46, 38, 30, 22, 14, 6, 64, 56, 48, 40, 32, 24, 16, 8, 57, 49, 41, 33, 25, 17, 9,
    1, 59, 51, 43, 35, 27, 19, 11, 3, 61, 53, 45, 37, 29, 21, 13, 5, 63, 55, 47, 39, 31, 23, 15, 7,
    32, 1, 2, 3, 4, 5, 4, 5, 6, 7, 8, 9, 8, 9, 10, 11, 12, 13, 12, 13, 14, 15, 16, 17, 16, 17, 18,
    19, 20, 21, 20, 21, 22, 23, 24, 25, 24, 25, 26, 27, 28, 29, 28, 29, 30, 31, 32, 1, 16, 7, 20, 21,
    29, 12, 28, 17, 1, 15, 23, 26, 5, 18, 31, 10, 2, 8, 24, 14, 32, 27, 3, 9, 19, 13, 30, 6, 22, 11,
    4, 25, 40, 8, 48, 16, 56, 24, 64, 32, 39, 7, 47, 15, 55, 23, 63, 31, 38, 6, 46, 14, 54, 22, 62,
    30, 37, 5, 45, 13, 53, 21, 61, 29, 36, 4, 44, 12, 52, 20, 60, 28, 35, 3, 43, 11, 51, 19, 59, 27,
    34, 2, 42, 10, 50, 18, 58, 26, 33, 1, 41, 9, 49, 17, 57, 25
};
new g_des_sboxes[8][64] = {
    {14, 4, 13, 1, 2, 15, 11, 8, 3, 10, 6, 12, 5, 9, 0, 7, 0, 15, 7, 4, 14, 2, 13, 1, 10, 6, 12, 11, 9, 5, 3, 8, 4, 1,
            14, 8, 13, 6, 2, 11, 15, 12, 9, 7, 3, 10, 5, 0, 15, 12, 8, 2, 4, 9, 1, 7, 5, 11, 3, 14, 10, 0, 6, 13},
    {15, 1, 8, 14, 6, 11, 3, 4, 9, 7, 2, 13, 12, 0, 5, 10, 3, 13, 4, 7, 15, 2, 8, 14, 12, 0, 1, 10, 6, 9, 11, 5, 0, 14,
            7, 11, 10, 4, 13, 1, 5, 8, 12, 6, 9, 3, 2, 15, 13, 8, 10, 1, 3, 15, 4, 2, 11, 6, 7, 12, 0, 5, 14, 9},
    {10, 0, 9, 14, 6, 3, 15, 5, 1, 13, 12, 7, 11, 4, 2, 8, 13, 7, 0, 9, 3, 4, 6, 10, 2, 8, 5, 14, 12, 11, 15, 1, 13, 6,
            4, 9, 8, 15, 3, 0, 11, 1, 2, 12, 5, 10, 14, 7, 1, 10, 13, 0, 6, 9, 8, 7, 4, 15, 14, 3, 11, 5, 2, 12},
    {7, 13, 14, 3, 0, 6, 9, 10, 1, 2, 8, 5, 11, 12, 4, 15, 13, 8, 11, 5, 6, 15, 0, 3, 4, 7, 2, 12, 1, 10, 14, 9, 10, 6,
            9, 0, 12, 11, 7, 13, 15, 1, 3, 14, 5, 2, 8, 4, 3, 15, 0, 6, 10, 1, 13, 8, 9, 4, 5, 11, 12, 7, 2, 14},
    {2, 12, 4, 1, 7, 10, 11, 6, 8, 5, 3, 15, 13, 0, 14, 9, 14, 11, 2, 12, 4, 7, 13, 1, 5, 0, 15, 10, 3, 9, 8, 6, 4, 2,
            1, 11, 10, 13, 7, 8, 15, 9, 12, 5, 6, 3, 0, 14, 11, 8, 12, 7, 1, 14, 2, 13, 6, 15, 0, 9, 10, 4, 5, 3},
    {12, 1, 10, 15, 9, 2, 6, 8, 0, 13, 3, 4, 14, 7, 5, 11, 10, 15, 4, 2, 7, 12, 9, 5, 6, 1, 13, 14, 0, 11, 3, 8, 9, 14,
            15, 5, 2, 8, 12, 3, 7, 0, 4, 10, 1, 13, 11, 6, 4, 3, 2, 12, 9, 5, 15, 10, 11, 14, 1, 7, 6, 0, 8, 13},
    {4, 11, 2, 14, 15, 0, 8, 13, 3, 12, 9, 7, 5, 10, 6, 1, 13, 0, 11, 7, 4, 9, 1, 10, 14, 3, 5, 12, 2, 15, 8, 6, 1, 4,
            11, 13, 12, 3, 7, 14, 10, 15, 6, 8, 0, 5, 9, 2, 6, 11, 13, 8, 1, 4, 10, 7, 9, 5, 0, 15, 14, 2, 3, 12},
    {13, 2, 8, 4, 6, 15, 11, 1, 10, 9, 3, 14, 5, 0, 12, 7, 1, 15, 13, 8, 10, 3, 7, 4, 12, 5, 6, 11, 0, 14, 9, 2, 7, 11,
            4, 1, 9, 12, 14, 2, 0, 6, 10, 13, 15, 3, 5, 8, 2, 1, 14, 7, 4, 10, 8, 13, 15, 12, 9, 0, 3, 5, 6, 11}
};
/* g_masks[n] has the n + 1 low bits set. */
new g_masks[32] = {
    0x1, 0x3, 0x7, 0xF, 0x1F, 0x3F, 0x7F, 0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF, 0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF, 0x1FFFF,
    0x3FFFF, 0x7FFFF, 0xFFFFF, 0x1FFFFF, 0x3FFFFF, 0x7FFFFF, 0xFFFFFF, 0x1FFFFFF, 0x3FFFFFF, 0x7FFFFFF, 0xFFFFFFF,
    0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF
};

/* Same as fade_in_background of background.inc, with "STRT" in g_des_pc1. */
fade_in_background(Float:fade_time)
{
    new scene = wndGetScene(WND_BG);
    if (scene)
    {
        SendMessage(scene, TAG(g_des_pc1), fade_time, 0, 0, 0);
    }
}

/* Decodes len Base64 characters of src into dest; returns the number of bytes. */
decode_base64(src[], len, dest[], maxlen)
{
    maxlen = maxlen * 4;
    new out;
    new size;
    new end;
    for (new i; i < len; i = i + 4)
    {
        new values[4];
        new bytes[1];
        new j;
        j = 0;
        for (; j < 4; j++)
        {
            if (!src{i + j})
            {
                break;
            }
            values[j] = get_base64_value(src{i + j});
            if (values[j] == -1)
            {
                end = 1;
                break;
            }
            bytes[0] |= values[j] << 18 - j * 6 + 8;
        }
        new count = 3;
        if (4 != j)
        {
            if (0 == j)
            {
                count = 0;
            }
            else if (j == 1)
            {
                count = 1;
            }
            else if (j == 2)
            {
                count = 2;
            }
            else if (j == 3)
            {
                count = 3;
            }
        }
        for (new k; k < count; k++)
        {
            dest{out} = bytes{k};
            out++;
        }
        if (j > 0)
        {
            size = size + (j - 1);
        }
        if (0 != end)
        {
            break;
        }
    }
    return size;
}

/* Returns the 6-bit value of a Base64 digit, -1 for the padding "=". */
get_base64_value(c)
{
    if ('A' <= c <= 'Z')
    {
        return c - 'A';
    }
    if ('a' <= c <= 'z')
    {
        return c - 'a' + 26;
    }
    if ('0' <= c <= '9')
    {
        return c - '0' + 52;
    }
    if (c == '+')
    {
        return 62;
    }
    if (c == '/')
    {
        return 63;
    }
    if (c == '=')
    {
        return -1;
    }
    return 0;
}

#include "misc.inc"

/* Rotates a 28-bit half of the DES key left by shift (1 or 2) bits. */
rotate_half(half[], shift)
{
    if (shift == 1)
    {
        half[0] = (half[0] >> 27 & 1 | half[0] << 1) & 0x0FFFFFFF;
    }
    else
    {
        half[0] = (half[0] >> 26 & 3 | half[0] << 2) & 0x0FFFFFFF;
    }
    half[1] = 0;
}

/* Returns the index in an S-box of 6 bits: row from the outer bits, column from the inner ones. */
get_sbox_index(bits)
{
    return bits & 0x20 | bits >> 1 & 0xF | bits << 4 & 0x10;
}

copy_block(dest[], src[])
{
    dest[0] = src[0];
    dest[1] = src[1];
}

/*
 * Encrypts or decrypts one 64-bit block (two cells, low word first) with DES. err10101 takes the permutations
 * by their address in the data: 56 is g_des_pc1, 344 to 1112 the parts of g_des_tables.
 */
des_block(out[], in[], key[], decrypt)
{
    new round;
    new i;
    new cd[2];
    new block[2];
    new tmp[2];
    new c[2];
    new d[2];
    new subkeys[16][2];
    cd[1] = 0;
    cd[0] = 0;
    err10101(cd, key, 56, 56);
    c[1] = 0;
    c[0] = cd[1] >> 4 & 0x0FFFFFFF;
    d[1] = 0;
    d[0] = (cd[0] >> 8 & g_masks[23] | cd[1] << 24) & 0x0FFFFFFF;
    round = 0;
    for (; round < 16; round++)
    {
        rotate_half(c, g_des_tables[round]);
        rotate_half(d, g_des_tables[round]);
        block[1] = c[0] << 4 | d[1] << 8 | d[0] >> 24 & g_masks[7];
        block[0] = d[0] << 8;
        tmp[1] = 0;
        tmp[0] = 0;
        err10101(tmp, block, 344, 48);
        copy_block(subkeys[round], tmp);
    }
    block[1] = 0;
    block[0] = 0;
    err10101(block, in, 536, 64);
    new left[2];
    new right[2];
    left[1] = block[1];
    left[0] = 0;
    right[1] = block[0];
    right[0] = 0;
    round = 0;
    for (; round < 16; round++)
    {
        tmp[1] = 0;
        tmp[0] = 0;
        err10101(tmp, right, 792, 48);
        if (decrypt)
        {
            tmp[1] ^= subkeys[15 - round][1];
            tmp[0] ^= subkeys[15 - round][0];
        }
        else
        {
            tmp[1] ^= subkeys[round][1];
            tmp[0] ^= subkeys[round][0];
        }
        tmp[0] = tmp[0] >> 16 & g_masks[15] | tmp[1] << 16;
        tmp[1] = tmp[1] >> 16 & g_masks[15];
        block[1] = 0;
        block[0] = 0;
        i = 0;
        for (; i < 8; i++)
        {
            block[1] |= g_des_sboxes[7 - i][get_sbox_index(tmp[0] & 0x3F)] << i * 4;
            tmp[0] = tmp[0] >> 6 & g_masks[25] | tmp[1] << 26;
            tmp[1] = tmp[1] >> 6 & g_masks[25];
        }
        tmp[1] = 0;
        tmp[0] = 0;
        err10101(tmp, block, 984, 32);
        new saved[2];
        saved[1] = right[1];
        saved[0] = right[0];
        right[1] = left[1] ^ tmp[1];
        right[0] = left[0] ^ tmp[0];
        left[1] = saved[1];
        left[0] = saved[0];
    }
    block[1] = right[1];
    block[0] = right[0] | left[1];
    tmp[1] = 0;
    tmp[0] = 0;
    err10101(tmp, block, 1112, 64);
    out[1] = tmp[1];
    out[0] = tmp[0];
    printf("des out %08x%08x\n", out[1], out);
    return;
}

/* The second key of the triple DES of the boot arguments; the first one comes with them. */
new g_des_key2[2] = {0x8DB2CDB9, 0x53042DCB};

/*
 * Decodes the words of src after the first one into args (ROT13, Base64, then triple DES in CBC mode with the key
 * in the first word and g_des_key2); count[0] is the room in args and gets the number of words. Returns 0, or -1
 * if there are too many.
 */
decode_boot_args(src[], count[], args[][], maxlen)
{
    new j;
    new pos;
    new key[2] = {0xDDB14339, 0xCB54312D};
    new size;
    new word[16];
    while (' ' != (word{j++} = src{pos++}))
    {
    }
    {
        new decoded[16];
        size = decode_base64(word, strlen(word), decoded, 16);
        key{0} = decoded{3};
        key{1} = decoded{2};
        key{2} = decoded{1};
        key{3} = decoded{0};
        key{4} = decoded{7};
        key{5} = decoded{6};
        key{6} = decoded{5};
        key{7} = decoded{4};
        static key_format[5] = "dec_key %08x%08x\n";
        printf(key_format, key[1], key);
    }
    new block;
    new prev[2];
    prev[1] = 0;
    prev[0] = 0;
    new plain[16] = "[]";
    new n;
    for (; ; )
    {
        while (src{pos++} == ' ')
        {
        }
        if (0 == src{--pos})
        {
            printf("end of args %d \n", n);
            count[0] = n;
            return 0;
        }
        n++;
        if (n > count[0])
        {
            return -1;
        }
        j = 0;
        do
        {
            word{j++} = src{pos++};
        }
        while (' ' != src{pos} && 0 != src{pos});
        word{j} = 0;
        printf("new enc_arg '%s'\n", word);
        apply_rot13(word);
        printf("rot13 dec_str '%s'\n", word);
        {
            new decoded[16];
            size = decode_base64(word, strlen(word), decoded, 16);
            copy_string(word, decoded, 16);
            printf("base64 dec_size %d data %08x%08x\n", size, word[1], word);
        }
        new blocks = (size + 7 & -8) >> 3;
        block = 0;
        for (; block < blocks; block++)
        {
            new cipher[2];
            new out[2];
            cipher{7} = word{block * 2 * 4 + 4};
            cipher{6} = word{block * 2 * 4 + 5};
            cipher{5} = word{block * 2 * 4 + 6};
            cipher{4} = word{block * 2 * 4 + 7};
            cipher{3} = word{block * 2 * 4 + 0};
            cipher{2} = word{block * 2 * 4 + 1};
            cipher{1} = word{block * 2 * 4 + 2};
            cipher{0} = word{block * 2 * 4 + 3};
            printf("dec step%d in %08x%08x\n", block, cipher[1], cipher);
            des_block(out, cipher, key, 1);
            des_block(out, out, g_des_key2, 0);
            des_block(out, out, key, 1);
            printf("dec step%d out %08x%08x\n", block, out[1], out);
            out[1] ^= prev[1];
            out[0] ^= prev[0];
            plain{block * 2 * 4 + 7} = out{4};
            plain{block * 2 * 4 + 6} = out{5};
            plain{block * 2 * 4 + 5} = out{6};
            plain{block * 2 * 4 + 4} = out{7};
            plain{block * 2 * 4 + 3} = out{0};
            plain{block * 2 * 4 + 2} = out{1};
            plain{block * 2 * 4 + 1} = out{2};
            plain{block * 2 * 4 + 0} = out{3};
            prev[1] = cipher[1];
            prev[0] = cipher[0];
        }
        copy_string(args[n - 1], plain, maxlen);
        printf("decoded dec_str '%s'\n", args[n - 1]);
    }
    return 0;
}

new g_resume_key[] = "resume_netbrowse";
public focus_glowfly; /* unused, but part of the layout */

main()
{
    start_top_page(0);
    load_common_bg("bg", 1000.0);
    fade_in_background(1000.0);
    new mode; /* the step that came back here, to go on from (STEP_*) */
    {
        new arg[64];
        GetArg("mode", arg, 64);
        printf("mode %s\n", arg);
        if (!strcmp(arg, "InitialSetup"))
        {
            mode = STEP_SYSCONF;
        }
        else if (!strcmp(arg, "DBConvert"))
        {
            mode = STEP_DBCONVERT;
        }
        else if (!strcmp(arg, "NetworkSetup"))
        {
            mode = STEP_NETCONF;
        }
        else if (!strcmp(arg, "GetIspChannel"))
        {
            mode = STEP_ISP;
        }
        else if (!strcmp(arg, "SystemUpdate"))
        {
            mode = STEP_SYSUPDATE;
        }
        else if (!strcmp(arg, "START"))
        {
            mode = STEP_START;
        }
    }
    if (0 == avalGetNum("InitSetup"))
    {
        if (mode < STEP_SYSCONF)
        {
            check_sysconf();
        }
        if (mode < STEP_DBCONVERT)
        {
            check_dbconvert();
        }
        if (mode < STEP_NETCONF)
        {
            check_netconf();
        }
        if (mode < STEP_ISP)
        {
            check_isp();
        }
        if (mode < STEP_SYSUPDATE)
        {
            check_sysupdate();
        }
    }
    finish_setup();
}

/*
 * Reads the boot mode and the system settings; goes to the system settings page if the console has not been set
 * up yet, or if the date is not set (to set the clock).
 */
check_sysconf()
{
    new date_ok;
    scnfCheckDate(date_ok);
    read_boot_mode();
    new ret = scnfRead();
    if (ret >= 0)
    {
        scnfSetSystemOption();
        wait_scnf2();
    }
    if (ret == 1 || 0 == date_ok)
    {
        new url[256];
        get_tag_attr("LINK", "SysConf", "href", url, 256);
        if (0 == date_ok)
        {
            xmlLoadStart(url, _, "time=1", XMLLOAD_MAIN);
        }
        else
        {
            xmlLoadStart(url, _, _, XMLLOAD_MAIN);
        }
        wait_xml_load();
        Destroy(Self());
    }
    else
    {
        scnfWrite();
        scnfSetSystemOption();
        wait_scnf2();
    }
    return;
}

/* Goes to the database conversion if the databases need it. */
check_dbconvert()
{
    new ret;
    ret = scnfDBConvCheck();
    if (0 == ret)
    {
        new url[256];
        get_tag_attr("LINK", "DBConvert", "href", url, 256);
        xmlLoadStart(url, _, _, XMLLOAD_MAIN);
        wait_xml_load();
        Destroy(Self());
    }
    return;
}

/* Goes to the network settings if there are none. */
check_netconf()
{
    new mode;
    new size;
    if (0 != fileLstat("file:/etc/ps2net.conf", mode, size))
    {
        new url[256];
        get_tag_attr("LINK", "NetConf", "href", url, 256);
        xmlLoadStart(url, _, _, XMLLOAD_MAIN);
        wait_xml_load();
        Destroy(Self());
    }
    return;
}

/* Goes to the ISP settings if their file is on the hard disk and the network is set and up. */
check_isp()
{
    new mode;
    new size;
    if (0 == fileLstat("pfs:/__contents/bn.conf/default_isp.dat", mode, size)
            && 0 == fileLstat("file:/etc/ps2net.conf", mode, size) && GetSystemParam(SYS_NETWORK_UP) == 1)
    {
        new url[256];
        get_tag_attr("LINK", "GetISP", "href", url, 256);
        xmlLoadStart(url, _, _, XMLLOAD_MAIN);
        wait_xml_load();
        Destroy(Self());
    }
    return;
}

/* Goes to the system update if the network allows it, else records why not in UpdateErrorFlag. */
check_sysupdate()
{
    new mode;
    new size;
    avalDelete("UpdateErrorFlag");
    if (0 != fileLstat("file:/etc/ps2net.conf", mode, size))
    {
        avalSet("UpdateErrorFlag", "SysupNG014");
    }
    else if (0 == fileLstat("pfs:/__contents/bn.conf/default_isp.dat", mode, size))
    {
        avalSet("UpdateErrorFlag", "SysupNG002");
    }
    else if (1 != GetSystemParam(SYS_NETWORK_UP))
    {
        avalSet("UpdateErrorFlag", "SysupNG015");
    }
    else
    {
        new url[256];
        get_tag_attr("LINK", "SysUpdate", "href", url, 256);
        xmlLoadStart(url, _, "mode=Init", XMLLOAD_MAIN);
        wait_xml_load();
        Destroy(Self());
    }
    return;
}

/* Marks the setup as running and goes to the top page, without booting the disc if disc_boot is 0. */
go_to_top(disc_boot)
{
    avalSet("InitSetup", "Running");
    new url[256];
    get_tag_attr("LINK", "Top", "href", url, 256);
    if (disc_boot == 1)
    {
        xmlLoadStart(url, "", "first=1", XMLLOAD_MAIN);
    }
    else
    {
        xmlLoadStart(url, "", "first=1;discboot=0", XMLLOAD_MAIN);
    }
    wait_xml_load();
    Destroy(Self());
    return;
}

/* Goes back to the page left for the browser, or to the top page (without booting the disc in the init mode 1). */
finish_setup()
{
    new buf1[64]; /* unused, but part of the layout */
    new buf2[64]; /* unused, but part of the layout */
    new buf3[64]; /* unused, but part of the layout */
    new buf4[64]; /* unused, but part of the layout */
    new buf5[64]; /* unused, but part of the layout */
    new ini_mode;
    new ret; /* unused, but part of the layout */
    if (avalGetNum(g_resume_key) > 0)
    {
        resume_page();
    }
    ini_mode = scnfGetIniMode();
    switch (ini_mode)
    {
        case 1:
        {
            go_to_top(0);
        }
    }
    go_to_top(1);
}

/* Shows the file system error, deletes the aval file and goes to the top page. */
report_fsck_error(disc_boot)
{
    show_fsck_error();
    delete_aval_and_go_top(1, disc_boot);
}

/* Deletes the aval file (on a file system error, showing it if not error_shown) and goes to the top page. */
delete_aval_and_go_top(error_shown, disc_boot)
{
    new ret = avalDeleteFile();
    if (ret == ERR_EIO && 0 == error_shown)
    {
        ret = show_fsck_error();
    }
    go_to_top(disc_boot);
}

/* Returns 1 if the boot argument asks for the browser. */
is_browser_boot(arg[])
{
    if (0 == strcmp("BootBrowser", arg))
    {
        return 1;
    }
    if (0 == strcmp("BootError", arg) || 0 == strcmp("BootClock", arg) || 0 == strcmp("BootOpening", arg)
            || 0 == strcmp("Initialize", arg) || 0 == strcmp("BootWarning", arg) || 0 == strcmp("BootIllegal", arg))
    {
        return 0;
    }
    return 0;
}

/* Shows the file system error in a message box and returns its result. */
show_fsck_error()
{
    new line1[32];
    new line2[32];
    new line3[32];
    new line4[32];
    new msg[128];
    new ok_label[32]; /* unused, but part of the layout */
    get_tag_attr("TEXT", "msgutil_error_fsck1", "value", line1, 32);
    get_tag_attr("TEXT", "msgutil_error_fsck2", "value", line2, 32);
    get_tag_attr("TEXT", "msgutil_error_fsck3", "value", line3, 32);
    get_tag_attr("TEXT", "msgutil_error_fsck4", "value", line4, 32);
    sprintf(msg, "%s%s\n%s%s", line1, line2, line3, line4);
    get_tag_attr("TEXT", "msgutil_common_ok", "value", ok_label, 32);
    new ret = show_msgbox(0, msg, "", "", 0);
    return ret;
}

/* Goes to the page recorded under g_resume_key, and forgets it. */
resume_page()
{
    new url[256];
    avalGetValue(g_resume_key, 0, url, 256);
    avalDelete(g_resume_key);
    xmlLoadStart(url, _, _, XMLLOAD_MAIN);
    wait_xml_load();
    Destroy(Self());
}

/*
 * Sets the init mode from the boot arguments in the kernel command line: 1 to boot into the browser, else 0 (also
 * when they cannot be read, deleting the aval file).
 */
read_boot_mode()
{
    new ini_mode;
    new word1[64];
    new word2[64];
    new word3[64];
    new word4[64];
    new word5[64];
    new ret;
    new value; /* unused, but part of the layout */
    ret = scnfGetCmdline(word1, word2, word3, word4, word5, 64, 64, 64, 64, 64);
    if (ret < 0)
    {
        ini_mode = 0;
        scnfSetIniMode(ini_mode);
        ret = avalDeleteFile();
        if (ret == ERR_EIO)
        {
            report_fsck_error(1);
        }
        return;
    }
    new count[1];
    new args[8][32];
    count[0] = 8;
    if (strlen(word5) > 0)
    {
        ret = decode_boot_args(word5, count, args, 32);
        if (ret < 0)
        {
            ini_mode = 0;
            scnfSetIniMode(ini_mode);
            ret = avalDeleteFile();
            if (ret == ERR_EIO)
            {
                report_fsck_error(1);
            }
            return;
        }
    }
    else
    {
        ini_mode = 0;
        scnfSetIniMode(ini_mode);
        ret = avalDeleteFile();
        if (ret == ERR_EIO)
        {
            report_fsck_error(1);
        }
        return;
    }
    if (is_browser_boot(args[0]) == 1)
    {
        ini_mode = 1;
    }
    else
    {
        ini_mode = 0;
    }
    scnfSetIniMode(ini_mode);
    return;
}
