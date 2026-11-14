/*
 * The BERDecode/DEREncode layer of BSAFE: the value handlers of the ASN.1 templates, converting
 * between the contents of an element and integers, bit strings, strings and times.
 */
#include "bsafe.h"
#include "berder.h"
#include "asn.h"


/* A decoded time (BERDecodeUTCTime, BERDecodeGeneralizedTime): the fields of the date, the
 * seconds, the microseconds and the time zone offset in minutes (cleared once applied). */
typedef struct BerTime {
    unsigned short year;
    unsigned short month;
    unsigned short day;
    unsigned short hour;
    unsigned short min;
    unsigned short sec;
    unsigned int usec;
    short tzmin;
} BerTime;

/* Days per month, indexed by month - 1; February of a leap year is handled by the macro. */
static int ber_days_in_month[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

#define BER_DAYS_IN_MONTH(y, m) \
    ((((y) % 4) == 0 && (m) == 2) ? 29 : ber_days_in_month[(m) - 1])

#define BER_SECS_IN_YEAR(y) ((((y) & 3) ? 365u : 366u) * 86400u)

static int der_encode_utctime_body(unsigned char *buf, unsigned int *outlen,
                                   unsigned int value);
static int ber_decode_utctime_body(int *out, const unsigned char *data,
                                   unsigned int len);
static int der_encode_gentime_body(unsigned char *buf, unsigned int *outlen,
                                   BerTime *in);
static int ber_decode_gentime_body(BerTime *out, const unsigned char *data,
                                   unsigned int len);
static int ber_time_apply_offset(BerTime *t);
static int ber_time_parse_fraction(BerTime *out, const unsigned char *buf,
                                   unsigned int len, unsigned int *pos,
                                   unsigned int unit);

int BERDecodeItem(BER_ELEMENT *item, ITEM *out)
{
    out->data = item->data;
    out->len = item->len;
    return 0;
}

/* DEREncode*: the encoding side. Each takes (ctx, tag, tagBytes, value) and hands the contents to
 * ASN_AddElement. */

int DEREncodeItem(void *ctx, unsigned int tag, unsigned char *tagBytes, ITEM *in)
{
    if (in == 0 || in->data == 0)
        return 0;
    return ASN_AddElement(ctx, tag, tagBytes, in->data, in->len);
}


int BERDecodeBitString(BER_ELEMENT *item, BerBitString *out)
{
    unsigned char unused = *item->data;
    if (unused >= 8)
        return BER_ERR_INPUT_DATA;
    if (unused != 0) {
        if (item->len < 2)
            return BER_ERR_INPUT_DATA;
    }
    if (item->len == 1) {
        out->data = 0;
        out->len = 0;
        out->unused = 0;
    } else {
        out->data = item->data + 1;
        out->len = item->len - 1;
        out->unused = *item->data;
    }
    return 0;
}


/* As DEREncodeBitString, but first trims the trailing zero bits (a NamedBitString drops the bits
 * after the last 1), updating the length and the unused count. An all-zero string becomes one
 * content byte 0, without the unused-count byte. */
int DEREncodeNamedBitString(void *ctx, unsigned int tag, unsigned char *tagBytes, DerBitStringIn *in)
{
    unsigned char unused;
    unsigned char last;
    unsigned int len;
    int status;

    unused = in->unused;
    if (in->unused != 0) {
        if (unused >= 8)
            return BER_ERR_DATA;
        if (in->len == 0)
            return BER_ERR_DATA;
    }
    len = in->len;
    for (;;) {
        if (len == 0)
            goto short_zero;
        if ((in->data[len - 1] >> unused) & 1)
            break;
        unused = unused + 1;
        if (unused == 8) {
            unused = 0;
            len = len - 1;
        }
    }
    if (len != 0)
        goto emit;
short_zero:
    unused = 0;
    len = 1;
    return ASN_AddElement(ctx, tag, tagBytes, &unused, len);
emit:
    status = ASN_AddElement(ctx, tag | 0x800, tagBytes, &unused, 1);
    if (status != 0)
        return status;
    if (unused == 0)
        goto plain_full;
    if (len >= 2) {
        status = ASN_AddElement(ctx, tag | 0x800, tagBytes, in->data, len - 1);
        if (status != 0)
            return status;
    }
    last = in->data[len - 1] & -(1 << unused);
    return ASN_AddElement(ctx, tag, tagBytes, &last, 1);
plain_full:
    return ASN_AddElement(ctx, tag, tagBytes, in->data, len);
}

int DEREncodeBitString(void *ctx, unsigned int tag, unsigned char *tagBytes, DerBitStringIn *in)
{
    unsigned char unused;
    unsigned char last;
    int status;

    unused = in->unused;
    if (in->unused != 0) {
        if (unused >= 8)
            return BER_ERR_DATA;
        if (in->len == 0)
            return BER_ERR_DATA;
    }
    status = ASN_AddElement(ctx, tag | 0x800, tagBytes, &unused, 1);
    if (status != 0)
        return status;
    if (unused == 0)
        goto simple;
    if (in->len >= 2) {
        status = ASN_AddElement(ctx, tag | 0x800, tagBytes, in->data, in->len - 1);
        if (status != 0)
            return status;
    }
    last = in->data[in->len - 1] & -(1 << unused);
    return ASN_AddElement(ctx, tag, tagBytes, &last, 1);
simple:
    return ASN_AddElement(ctx, tag, tagBytes, in->data, in->len);
}

int BERDecodeUint2(BER_ELEMENT *item, unsigned short *out)
{
    ITEM octets;
    unsigned char buf[2];
    int status;
    octets.data = buf;
    octets.len = 2;
    status = BERDecodeUnsignedInt(item, &octets);
    if (status != 0)
        return status;
    *out = (unsigned short)((buf[0] << 8) | buf[1]);
    return 0;
}

int DEREncodeUint2(void *ctx, unsigned int tag, unsigned char *tagBytes, unsigned short *v)
{
    ITEM octets;
    unsigned char buf[2];
    if (v == 0)
        return 0;
    octets.data = buf;
    octets.len = 2;
    buf[1] = *(unsigned char *)v;
    buf[0] = *v >> 8;
    return DEREncodeUnsignedInt(ctx, tag, tagBytes, &octets);
}

int BERDecodeUint(BER_ELEMENT *item, unsigned int *out)
{
    ITEM octets;
    unsigned char buf[4];
    int status;
    unsigned int i;
    int shift;
    octets.data = buf;
    octets.len = 4;
    status = BERDecodeUnsignedInt(item, &octets);
    if (status != 0)
        return status;
    *out = 0;
    for (i = 0; i < 4; i++)
        *out = *out | (buf[i] << (24 - i * 8));
    return 0;
}

int DEREncodeUint(void *ctx, unsigned int tag, unsigned char *tagBytes, unsigned int *v)
{
    ITEM octets;
    unsigned char buf[4];
    unsigned int value;
    unsigned int i;
    int shift;
    if (v == 0)
        return 0;
    for (i = 0; i < 4; i++)
        buf[i] = *v >> (24 - i * 8);
    octets.data = buf;
    octets.len = 4;
    return DEREncodeUnsignedInt(ctx, tag, tagBytes, &octets);
}

/* Decodes a uint2 and checks that it equals the value expected in *out (BER_ERR_INPUT_DATA otherwise). */
int BERConstraintUint2(BER_ELEMENT *item, unsigned short *out)
{
    unsigned short v;
    int status;

    status = BERDecodeUint2(item, &v);
    if (status != 0)
        return status;
    return (v == *out) ? 0 : BER_ERR_INPUT_DATA;
}

int BERDecodeUint4(BER_ELEMENT *item, unsigned int *out)
{
    unsigned char buf[4];
    ITEM octets;
    int status;
    octets.data = buf;
    octets.len = 4;
    status = BERDecodeUnsignedInt(item, &octets);
    if (status != 0)
        return status;
    *out = ((unsigned int)buf[0] << 24) + ((unsigned int)buf[1] << 16) + ((unsigned int)buf[2] << 8) + buf[3];
    return 0;
}

int DEREncodeUint4(void *ctx, unsigned int tag, unsigned char *tagBytes, unsigned int *v)
{
    ITEM octets;
    unsigned char buf[4];
    unsigned int value;
    if (v == 0)
        return 0;
    value = *v;
    octets.data = buf;
    octets.len = 4;
    buf[3] = value;
    value >>= 8;
    buf[2] = value;
    value >>= 8;
    buf[1] = value;
    value >>= 8;
    buf[0] = value;
    return DEREncodeUnsignedInt(ctx, tag, tagBytes, &octets);
}

/* out is {buf, capacity}: the value is copied at the end of the buffer, zero-padded on the left. */
int BERDecodeUnsignedInt(BER_ELEMENT *item, ITEM *out)
{
    ITEM local;
    unsigned int pad;
    int status = BERDecodeUnsignedIntPointer(item, &local);
    if (status != 0)
        return status;
    if (local.len > out->len)
        return BER_ERR_INPUT_DATA;
    pad = out->len - local.len;
    T_memset(out->data, 0, pad);
    T_memcpy(out->data + pad, local.data, local.len);
    return 0;
}

/* Strips the leading 0x00 octets of an unsigned DER integer; an error when the length is 0 or when
 * the first remaining byte has its high bit set (too large, or negative). */
int BERDecodeUnsignedIntPointer(BER_ELEMENT *item, ITEM *out)
{
    unsigned char *data = item->data;
    unsigned int len;
    out->data = data;
    len = item->len;
    out->len = len;
    if (len == 0)
        goto error;
    if (!(data[0] & 0x80))
        goto cont;
error:
    return BER_ERR_INPUT_DATA;
cont:
    if (data[0] != 0)
        goto out;
    for (;;) {
        unsigned int remaining = out->len;
        unsigned char *ptr = out->data;
        remaining--;
        out->len = remaining;
        out->data = ptr + 1;
        if (remaining == 0)
            break;
        if (ptr[1] != 0)
            break;
    }
out:
    return 0;
}

static unsigned char der_zero_pad = 0;

int DEREncodeUnsignedInt(void *ctx, unsigned int tag, unsigned char *tagBytes, ITEM *in)
{
    ITEM local;
    unsigned char first;
    unsigned int l;
    unsigned char *p;
    int status;

    local.data = in->data;
    local.len = in->len;
    if (local.len != 0) {
        first = *local.data;
        if (first == 0) {
            do {
                p = local.data;
                l = local.len;
                l--;
                local.len = l;
                local.data = p + 1;
                if (l == 0)
                    goto pad;
            } while (p[1] == 0);
        }
    }
    if (local.len != 0) {
        if (!(*local.data & 0x80))
            goto plain;
    }
pad:
    status = ASN_AddElement(ctx, tag | 0x800, tagBytes, (void *)&der_zero_pad, 1);
    if (status != 0)
        return status;
plain:
    return ASN_AddElement(ctx, tag, tagBytes, local.data, local.len);
}


int BERDecodeUTCTime(BER_ELEMENT *item, void *out)
{
    return ber_decode_utctime_body((int *)out, item->data, item->len);
}

int DEREncodeUTCTime(void *ctx, unsigned int tag, unsigned char *tagBytes, ITEM *in)
{
    unsigned char buf[32];
    unsigned int len;
    int status;
    if (in == 0)
        return 0;
    status = der_encode_utctime_body(buf, &len, (unsigned int)in->data);
    if (status != 0)
        return status;
    return ASN_AddElement(ctx, tag, tagBytes, buf, len);
}

int BERDecodeGeneralizedTime(BER_ELEMENT *item, void *out)
{
    if (item->len == 0 || item->data == 0)
        return BER_ERR_DATA;
    return ber_decode_gentime_body((BerTime *)out, item->data,
                                   item->len);
}

int DEREncodeGeneralizedTime(void *ctx, unsigned int tag, unsigned char *tagBytes, ITEM *in)
{
    unsigned char buf[32];
    unsigned int len;
    int status;
    if (in == 0)
        return 0;
    status = der_encode_gentime_body(buf, &len, (BerTime *)in);
    if (status != 0)
        return status;
    return ASN_AddElement(ctx, tag, tagBytes, buf, len);
}

int DEREncodeFull(void *ctx, unsigned int tag, unsigned char *tagBytes, ITEM *in)
{
    return ASN_AddElement(ctx, 256, tagBytes, in->data, in->len);
}

/* Encodes a DirectoryString CHOICE (PrintableString, T61String, UniversalString, BMPString...). in
 * is {tag, count, data} with count in code units: the length is count * 4 for UniversalString (28),
 * count * 2 for BMPString (30), count for the others. Nothing is emitted when in is 0, its tag
 * differs or data is 0. When ASN_AddElement fails the name of the type goes to
 * ctx->encodeFieldName; the status is that of ASN_AddElement. */
typedef struct DerStringIn {
    unsigned int tag;   /* the string type the caller asks for */
    unsigned int count; /* code units, not bytes */
    void *data;
} DerStringIn;


/* The decoding twin of DEREncodeDirectoryString: from the tag, contents and byte length of the
 * element to {tag, count, data}, count in code units. 2184 for a tag that is not allowed or a
 * length that is not a multiple of the unit; then the name of the type also goes to
 * decodeFieldName. */

int BERDecodeDirectoryString(BER_ELEMENT *item, DerStringIn *out)
{
    char *def;
    int status;
    int tag;

    def = 0;
    status = 0;
    tag = item->tag;
    out->tag = tag;

    switch (tag) {
    case 19:
        out->count = item->len;
        out->data = item->data;
        break;
    case 20:
        out->count = item->len;
        out->data = item->data;
        break;
    case 28:
        if (item->len & 3) {
            def = "universalString";
            status = 2184;
        } else {
            out->count = item->len / 4;
            out->data = item->data;
        }
        break;
    case 30:
        if (item->len & 1) {
            def = "bmpString";
            status = 2184;
        } else {
            out->count = item->len / 2;
            out->data = item->data;
        }
        break;
    default:
        status = 2184;
        break;
    }

    if (def != 0) {
        unsigned int len = T_strlen(def);
        char *buf = T_malloc(len + 1);
        item->decodeFieldName = buf;
        if (buf != 0)
            T_memcpy(buf, def, len + 1);
    }

    return status;
}

int DEREncodeDirectoryString(void *ctx, unsigned int tag, unsigned char *tagBytes, DerStringIn *in)
{
    unsigned int len;
    int status;

    status = 0;
    if (in != 0 && tag == in->tag && in->data != 0) {
        switch (tag) {
        case 28:
            len = in->count * 4;
            break;
        case 30:
            len = in->count * 2;
            break;
        default:
            len = in->count;
            break;
        }
        status = ASN_AddElement(ctx, tag, tagBytes, in->data, len);
    }

    if (status != 0) {
        char *def;

        switch (tag) {
        case 19:
            def = "printableString";
            break;
        case 20:
            def = "t61String";
            break;
        case 28:
            def = "universalString";
            break;
        case 30:
            def = "bmpString";
            break;
        default:
            def = 0;
            break;
        }
        if (def != 0) {
            char *buf;

            len = T_strlen(def) + 1;
            buf = T_malloc(len);
            ((AsnEncodeCtx *)ctx)->encodeFieldName = buf;
            if (buf != 0)
                T_memcpy(buf, def, len);
        }
    }

    return status;
}

int BERDecodeSequence(BER_ELEMENT *item, ITEM *out)
{
    return ASN_EncodeAnyAlloc(48, item->data, item->len, &out->data, &out->len);
}

int DEREncodeSequence(void *ctx, unsigned int tag, unsigned char *tagBytes, ITEM *in)
{
    ITEM local;
    int status;
    if (in == 0 || in->data == 0)
        return 0;
    status = ASN_DecodeAny(48, in->data, in->len, &local);
    if (status != 0)
        return status;
    return ASN_AddElement(ctx, tag, tagBytes, local.data, local.len);
}

/* Decodes a BER INTEGER of at most 4 bytes into an int: drops the redundant leading sign bytes,
 * fails with BER_ERR_INPUT_DATA when 5 or more remain, and sign-extends the rest. */
int BERDecodeInt(BER_ELEMENT *item, int *out)
{
    ITEM local;
    unsigned int fill;
    unsigned int notfill;
    unsigned char firstbyte;
    unsigned char *ptr;
    unsigned int len;
    unsigned int i;
    int negative;
    int positive;

    local.data = item->data;
    len = item->len;
    local.len = len;
    if (len == 0)
        return BER_ERR_INPUT_DATA;
    firstbyte = *local.data;
    fill = (firstbyte & 0x80) ? 0xff : 0;
    notfill = fill ^ 0xff;
    for (;;) {
        if (local.len < 2)
            break;
        ptr = local.data;
        if (*ptr != fill)
            break;
        negative = (notfill < 1);
        if (ptr[1] & 0x80) {
            positive = negative ^ 1;
            if (positive)
                break;
        } else {
            if (negative)
                break;
        }
        local.len = local.len - 1;
        local.data = local.data + 1;
    }
    if (local.len >= 5)
        return BER_ERR_INPUT_DATA;
    *out = 0;
    i = 0;
    for (; i < local.len; i++)
        *out = *out | (local.data[i] << ((local.len - 1 - i) * 8));
    for (; i < 4; i++)
        *out = *out | (fill << (i * 8));
    return 0;
}

/* The mirror of BERDecodeInt: serializes *v big-endian and emits the minimal DER encoding. */
int DEREncodeInt(void *ctx, unsigned int tag, unsigned char *tagBytes, int *v)
{
    ITEM local;
    unsigned char buf[4];
    unsigned int i;
    unsigned int fill;
    unsigned int notfill;
    unsigned char firstbyte;
    unsigned char *ptr;
    int negative;
    int positive;

    if (v == 0)
        return 0;
    for (i = 0; i < 4; i++)
        buf[i] = *v >> (24 - i * 8);
    local.data = buf;
    local.len = 4;
    firstbyte = buf[0];
    fill = (firstbyte & 0x80) ? 0xff : 0;
    notfill = fill ^ 0xff;
    for (;;) {
        if (local.len < 2)
            break;
        ptr = local.data;
        if (*ptr != fill)
            break;
        negative = (notfill < 1);
        if (ptr[1] & 0x80) {
            positive = negative ^ 1;
            if (positive)
                break;
        } else {
            if (negative)
                break;
        }
        local.len = local.len - 1;
        local.data = local.data + 1;
    }
    return ASN_AddElement(ctx, tag, tagBytes, local.data, local.len);
}

/* Decodes a BER INTEGER of any length into out = {data, len}. The scan of the leading bytes runs as
 * in BERDecodeInt but its result is not used: out gets the whole contents; BER_ERR_INPUT_DATA when
 * the length is 0. */
int BERDecodeLargeInt(BER_ELEMENT *item, ITEM *out)
{
    ITEM local;
    unsigned int fill;
    unsigned int notfill;
    unsigned char firstbyte;
    unsigned char *ptr;
    unsigned int len;
    int negative;
    int positive;

    local.data = item->data;
    len = item->len;
    local.len = len;
    if (len == 0)
        return BER_ERR_INPUT_DATA;
    firstbyte = *local.data;
    fill = (firstbyte & 0x80) ? 0xff : 0;
    notfill = fill ^ 0xff;
    for (;;) {
        if (local.len < 2)
            break;
        ptr = local.data;
        if (*ptr != fill)
            break;
        negative = (notfill < 1);
        if (ptr[1] & 0x80) {
            positive = negative ^ 1;
            if (positive)
                break;
        } else {
            if (negative)
                break;
        }
        local.len = local.len - 1;
        local.data = local.data + 1;
    }
    out->data = item->data;
    out->len = item->len;
    return 0;
}

/* As DEREncodeInt, for an integer already in a buffer {data, len}. */
int DEREncodeLargeInt(void *ctx, unsigned int tag, unsigned char *tagBytes, ITEM *in)
{
    ITEM local;
    unsigned int fill;
    unsigned int notfill;
    unsigned char firstbyte;
    unsigned char *ptr;
    int negative;
    int positive;

    if (in == 0 || in->data == 0)
        return BER_ERR_DATA;
    local.data = in->data;
    local.len = in->len;
    firstbyte = *local.data;
    fill = (firstbyte & 0x80) ? 0xff : 0;
    notfill = fill ^ 0xff;
    for (;;) {
        if (local.len < 2)
            break;
        ptr = local.data;
        if (*ptr != fill)
            break;
        negative = (notfill < 1);
        if (ptr[1] & 0x80) {
            positive = negative ^ 1;
            if (positive)
                break;
        } else {
            if (negative)
                break;
        }
        local.len = local.len - 1;
        local.data = local.data + 1;
    }
    return ASN_AddElement(ctx, tag, tagBytes, local.data, local.len);
}

int BERDecodeOptionalInt(BER_ELEMENT *item, BerOptionalInt *out)
{
    out->present = 1;
    return BERDecodeInt(item, &out->value);
}

int BERDecodeOptionalUint2(BER_ELEMENT *item, BerOptionalUint2 *out)
{
    out->present = 1;
    return BERDecodeUint2(item, &out->value);
}

int BERDecodeOptionalUint4(BER_ELEMENT *item, BerOptionalUint4 *out)
{
    out->present = 1;
    return BERDecodeUint4(item, &out->value);
}

/* Marks an optional field as present, without decoding it. */
int BERIndicatePresent(BER_ELEMENT *item, int *present)
{
    *present = 1;
    return 0;
}

int BERDecodeChar(BER_ELEMENT *item, char *out)
{
    if (item->len != 1)
        return BER_ERR_INPUT_DATA;
    *out = *(char *)item->data;
    return 0;
}

int DEREncodeChar(void *ctx, unsigned int tag, unsigned char *tagBytes, char *data)
{
    if (data == 0 || *data == 0)
        return 0;
    return ASN_AddElement(ctx, tag, tagBytes, data, 1);
}

int BERDecodeBool(BER_ELEMENT *item, int *out)
{
    unsigned int len = item->len;
    unsigned char *data;
    if (len != 1)
        return BER_ERR_INPUT_DATA;
    data = item->data;
    if (*data != 0)
        *out = len;
    else
        *out = 0;
    return 0;
}

int DEREncodeBool(void *ctx, unsigned int tag, unsigned char *tagBytes, int *data)
{
    unsigned char b;
    if (data == 0)
        return 0;
    if (*data != 0)
        b = 0xff;
    else
        b = 0;
    return ASN_AddElement(ctx, tag, tagBytes, &b, 1);
}

/* Serializes a time_t (seconds since 1970) as UTCTime "YYMMDDhhmmssZ". The third argument is the
 * value itself, the first word of the object passed to DEREncodeUTCTime. */
static int der_encode_utctime_body(unsigned char *buf, unsigned int *outlen,
                                   unsigned int value)
{
    int year, month, day, hour, min, sec;
    unsigned int secs;

    year = 70;
    secs = BER_SECS_IN_YEAR(year);
    while (value >= secs) {
        value -= secs;
        year++;
        secs = BER_SECS_IN_YEAR(year);
    }
    if (year >= 170)
        return BER_ERR_DATA;
    month = 1;
    while (value >= (secs = (unsigned int)BER_DAYS_IN_MONTH(year, month) * 86400)) {
        value -= secs;
        month++;
    }
    day = value / 86400 + 1;
    value = value % 86400;
    hour = value / 3600;
    value = value % 3600;
    min = value / 60;
    value = value % 60;
    sec = value;
    if (year >= 100)
        year -= 100;
    buf[0] = year / 10 + '0';
    buf[1] = year % 10 + '0';
    buf[2] = month / 10 + '0';
    buf[3] = month % 10 + '0';
    buf[4] = day / 10 + '0';
    buf[5] = day % 10 + '0';
    buf[6] = hour / 10 + '0';
    buf[7] = hour % 10 + '0';
    buf[8] = min / 10 + '0';
    buf[9] = min % 10 + '0';
    buf[10] = sec / 10 + '0';
    buf[11] = sec % 10 + '0';
    buf[12] = 'Z';
    *outlen = 13;
    return 0;
}

#define BER_ISDIGIT(c) ((c) >= '0' && (c) <= '9')

/* Decodes a UTCTime "YYMMDDhhmm[ss](Z|(+|-)hhmm)" into seconds since 1970 in *out. */
static int ber_decode_utctime_body(int *out, const unsigned char *data,
                                   unsigned int len)
{
    int year, month, day, hour, min, sec;
    int offhour, offmin;
    unsigned int i;
    char c;

    offhour = 0;
    offmin = 0;
    if (!BER_ISDIGIT(data[0]) || !BER_ISDIGIT(data[1]) || !BER_ISDIGIT(data[2]) ||
        !BER_ISDIGIT(data[3]) || !BER_ISDIGIT(data[4]) || !BER_ISDIGIT(data[5]) ||
        !BER_ISDIGIT(data[6]) || !BER_ISDIGIT(data[7]) || !BER_ISDIGIT(data[8]) ||
        !BER_ISDIGIT(data[9]))
        return BER_ERR_INPUT_DATA;
    year = (data[0] - '0') * 10;
    year += data[1] - '0';
    month = (data[2] - '0') * 10;
    month += data[3] - '0';
    day = (data[4] - '0') * 10;
    day += data[5] - '0';
    hour = (data[6] - '0') * 10;
    hour += data[7] - '0';
    min = (data[8] - '0') * 10;
    min += data[9] - '0';
    if (BER_ISDIGIT(data[10])) {
        if (!BER_ISDIGIT(data[11]))
            return BER_ERR_INPUT_DATA;
        sec = (data[10] - '0') * 10;
        sec += data[11] - '0';
        i = 12;
    } else {
        sec = 0;
        i = 10;
    }
    if (year < 0 || year >= 100)
        return BER_ERR_INPUT_DATA;
    if (month < 1)
        goto bad;
    if (month > 12)
        return BER_ERR_INPUT_DATA;
    if (day < 1)
        goto bad;
    if (day > BER_DAYS_IN_MONTH(year, month))
        return BER_ERR_INPUT_DATA;
    if (hour < 0)
        goto bad;
    if (hour >= 24)
        return BER_ERR_INPUT_DATA;
    if (min < 0)
        goto bad;
    if (min >= 60)
        return BER_ERR_INPUT_DATA;
    if (sec < 0)
        goto bad;
    if (sec >= 60)
        return BER_ERR_INPUT_DATA;
    c = data[i];
    i++;
    if (c == '+' || c == '-') {
        if (!BER_ISDIGIT(data[i]) || !BER_ISDIGIT(data[i + 1]) ||
            !BER_ISDIGIT(data[i + 2]) || !BER_ISDIGIT(data[i + 3]))
            return BER_ERR_INPUT_DATA;
        offhour = (data[i++] - '0') * 10;
        offhour += data[i++] - '0';
        offmin = (data[i++] - '0') * 10;
        offmin += data[i++] - '0';
        if (offhour < 0 || offhour >= 24)
            return BER_ERR_INPUT_DATA;
        if (offmin < 0)
            goto bad;
        if (offmin >= 60)
        bad:
            return BER_ERR_INPUT_DATA;
    } else if (c != 'Z') {
        return BER_ERR_INPUT_DATA;
    }
    if (i != len)
        return BER_ERR_INPUT_DATA;
    if (c == '+') {
        hour -= offhour;
        min -= offmin;
    } else if (c == '-') {
        hour += offhour;
        min += offmin;
    }
    if (year <= 69)
        year += 100;
    if (min < 0) {
        min += 60;
        hour--;
    } else if (min >= 60) {
        min -= 60;
        hour++;
    }
    if (hour < 0) {
        hour += 24;
        day--;
        if (day <= 0) {
            month--;
            if (month <= 0) {
                month += 12;
                year--;
            }
            day += BER_DAYS_IN_MONTH(year, month);
        }
    } else if (hour >= 24) {
        hour -= 24;
        day++;
        if (day > BER_DAYS_IN_MONTH(year, month)) {
            day -= BER_DAYS_IN_MONTH(year, month);
            month++;
            if (month > 12) {
                month -= 12;
                year++;
                if (year >= 170)
                    return BER_ERR_INPUT_DATA;
            }
        }
    }
    *out = sec + min * 60 + hour * 3600 + (day - 1) * 86400;
    while (month > 1) {
        *out += BER_DAYS_IN_MONTH(year, month - 1) * 86400;
        month--;
    }
    while (year > 70) {
        *out += BER_SECS_IN_YEAR(year - 1);
        year--;
    }
    return 0;
}

/* Serializes a BerTime as GeneralizedTime "YYYYMMDDhhmmss[.ffffff]Z", without the trailing zeros
 * of the fraction. */
static int der_encode_gentime_body(unsigned char *buf, unsigned int *outlen,
                                   BerTime *in)
{
    int status;
    unsigned int len;

    T_memset(buf, 0, 26);
    status = ber_time_apply_offset(in);
    if (status != 0)
        return status;
    buf[0] = in->year / 1000 + '0';
    buf[1] = in->year % 1000 / 100 + '0';
    buf[2] = in->year % 100 / 10 + '0';
    buf[3] = in->year % 10 + '0';
    buf[4] = in->month / 10 + '0';
    buf[5] = in->month % 10 + '0';
    buf[6] = in->day / 10 + '0';
    buf[7] = in->day % 10 + '0';
    buf[8] = in->hour / 10 + '0';
    buf[9] = in->hour % 10 + '0';
    buf[10] = in->min / 10 + '0';
    buf[11] = in->min % 10 + '0';
    buf[12] = in->sec / 10 + '0';
    buf[13] = in->sec % 10 + '0';
    len = 14;
    if (in->usec != 0) {
        buf[14] = '.';
        buf[15] = in->usec / 100000 + '0';
        buf[16] = in->usec % 100000 / 10000 + '0';
        buf[17] = in->usec % 10000 / 1000 + '0';
        buf[18] = in->usec % 1000 / 100 + '0';
        buf[19] = in->usec % 100 / 10 + '0';
        buf[20] = in->usec % 10 + '0';
        len = 21;
        while (buf[len - 1] == '0')
            len--;
    }
    buf[len++] = 'Z';
    *outlen = len;
    return 0;
}

/* Decodes a GeneralizedTime "YYYYMMDDhh[mm[ss]][(.|,)f][Z|(+|-)hhmm]" into the fields of *out. */
static int ber_decode_gentime_body(BerTime *out, const unsigned char *data,
                                   unsigned int len)
{
    unsigned int i;
    unsigned int unit;
    int status;
    int done;
    unsigned int offhour, offmin;
    int tz;

    status = 0;
    done = 0;
    unit = 1;
    T_memset(out, 0, 20);
    if (!BER_ISDIGIT(data[0]) || !BER_ISDIGIT(data[1]) || !BER_ISDIGIT(data[2]) ||
        !BER_ISDIGIT(data[3]) || !BER_ISDIGIT(data[4]) || !BER_ISDIGIT(data[5]) ||
        !BER_ISDIGIT(data[6]) || !BER_ISDIGIT(data[7]) || !BER_ISDIGIT(data[8]) ||
        !BER_ISDIGIT(data[9]))
        return BER_ERR_DATA;
    out->year = (data[0] - '0') * 1000;
    out->year += (data[1] - '0') * 100;
    out->year += (data[2] - '0') * 10;
    out->year += data[3] - '0';
    out->month = (data[4] - '0') * 10;
    out->month += data[5] - '0';
    out->day = (data[6] - '0') * 10;
    out->day += data[7] - '0';
    out->hour = (data[8] - '0') * 10;
    out->hour += data[9] - '0';
    if (out->year >= 10000)
        goto bad;
    if (out->month < 1)
        goto bad;
    if (out->month > 12)
        goto bad;
    if (out->day < 1 ||
        out->day > (unsigned int)BER_DAYS_IN_MONTH(out->year, out->month))
        return BER_ERR_DATA;
    if (out->hour >= 24)
        goto bad;
    i = 10;
    while (i < len) {
        if (done) {
            status = BER_ERR_DATA;
            break;
        }
        switch (data[i]) {
        case '.':
        case ',':
            i++;
            status = ber_time_parse_fraction(out, data, len, &i, unit);
            if (status)
                goto bad;
            unit = 3;
            break;
        case 'Z':
            done = 1;
            i++;
            break;
        case '+':
        case '-':
            i++;
            if (!BER_ISDIGIT(data[i]) || !BER_ISDIGIT(data[i + 1]) ||
                !BER_ISDIGIT(data[i + 2]) || !BER_ISDIGIT(data[i + 3])) {
                status = BER_ERR_DATA;
                break;
            }
            offhour = (data[i] - '0') * 10;
            offhour += data[i + 1] - '0';
            offmin = (data[i + 2] - '0') * 10;
            offmin += data[i + 3] - '0';
            if (offhour >= 24) {
                status = BER_ERR_DATA;
                break;
            }
            if (offmin >= 60) {
                status = BER_ERR_DATA;
                break;
            }
            out->tzmin = offhour * 60 + offmin;
            tz = out->tzmin;
            if (data[i - 1] == '-')
                tz = -tz;
            out->tzmin = tz;
            done = 1;
            i += 4;
            break;
        default:
            if (!BER_ISDIGIT(data[i]) || !BER_ISDIGIT(data[i + 1])) {
                status = BER_ERR_DATA;
                break;
            }
            switch (unit) {
            case 1:
                out->min = (data[i] - '0') * 10;
                out->min += data[i + 1] - '0';
                if (out->min >= 60) {
                    status = BER_ERR_DATA;
                    break;
                }
                unit = 2;
                i += 2;
                break;
            case 2:
                out->sec = (data[i] - '0') * 10;
                out->sec += data[i + 1] - '0';
                if (out->sec >= 60) {
                    status = BER_ERR_DATA;
                    break;
                }
                unit = 3;
                i += 2;
                break;
            default:
                status = BER_ERR_DATA;
                break;
            }
            break;
        }
        if (status)
            goto done;
    }
    if (status == 0)
        return ber_time_apply_offset(out);
    goto done;
bad:
    return BER_ERR_DATA;
done:
    return status;
}

/* Applies the time zone offset (tzmin, minutes) to the decoded time, normalizes the fields and
 * clears the offset. */
static int ber_time_apply_offset(BerTime *t)
{
    int year, month, day, hour, min;
    short tz, offhour, offmin;

    tz = t->tzmin;
    if (tz == 0)
        return 0;
    year = t->year;
    month = t->month;
    day = t->day;
    hour = t->hour;
    min = t->min;
    offhour = tz / 60;
    offmin = tz % 60;
    hour -= offhour;
    min -= offmin;
    if (min < 0) {
        min += 60;
        hour--;
    } else if (min >= 60) {
        min -= 60;
        hour++;
    }
    if (hour < 0) {
        hour += 24;
        day--;
        if (day <= 0) {
            month--;
            if (month <= 0) {
                month += 12;
                year--;
            }
            day += BER_DAYS_IN_MONTH(year, month);
        }
    } else if (hour >= 24) {
        hour -= 24;
        day++;
        if (day > BER_DAYS_IN_MONTH(year, month)) {
            day -= BER_DAYS_IN_MONTH(year, month);
            month++;
            if (month > 12) {
                month -= 12;
                year++;
                if (year > 9999)
                    return BER_ERR_DATA;
            }
        }
    }
    t->year = year;
    t->month = month;
    t->day = day;
    t->hour = hour;
    t->min = min;
    t->tzmin = 0;
    return 0;
}

/* Parses the fraction of a GeneralizedTime field (the digits after the comma), scaled to six
 * decimal digits, and spreads it over the fields of *out according to the unit it divides (1 hour,
 * 2 minute, 3 second). */
static int ber_time_parse_fraction(BerTime *out, const unsigned char *buf,
                                   unsigned int len, unsigned int *pos,
                                   unsigned int unit)
{
    unsigned int value = 0;
    int status = 0;
    unsigned int i = *pos;
    unsigned int n;

    if (i < len) {
        do {
            if (buf[i] < '0' || buf[i] > '9') {
                if (buf[i] == '+' || buf[i] == '-' || buf[i] == 'Z')
                    break;
                return BER_ERR_DATA;
            }
            value = value * 10 + (buf[i] - '0');
            i++;
        } while (i < len);
    }
    n = i - *pos;
    if (n >= 7)
        return BER_ERR_DATA;
    while (n < 6) {
        value = value * 10;
        n++;
    }
    *pos = i;
    switch (unit) {
    case 1:
        value = (value % 1000000) * 3600;
        out->min = value / 60;
        out->sec = value % 60;
        break;
    case 2:
        value = (value % 1000000) * 60;
        out->sec = value / 60;
        out->usec = value % 60;
        break;
    case 3:
        out->usec = value;
        break;
    default:
        status = BER_ERR_DATA;
        break;
    }
    return status;
}

/* Packs a BER BIT STRING (data/len/unused) into a UINT4 of `nbits` bits: concatenates the bytes
 * big-endian, clears the unused bits of the last byte and shifts the field to the requested width.
 * BER_ERR_INPUT_DATA for a malformed or too wide BIT STRING. */
unsigned int ConvertBitStringToUINT4(unsigned int *out, BerBitString *bs, unsigned int nbits)
{
    unsigned int i;
    unsigned int bitCount;

    if (nbits > 32 || bs->unused >= 8 || nbits < bs->len * 8 - bs->unused)
        return BER_ERR_INPUT_DATA;

    *out = 0;
    for (i = 0; i < bs->len; i++)
        *out = (*out << 8) | bs->data[i];

    if (bs->unused != 0)
        *out = *out & ((unsigned int)-1 << bs->unused);

    bitCount = bs->len * 8;
    if (bitCount < nbits)
        *out = *out << (nbits - bitCount);
    else
        *out = *out >> (bitCount - nbits);
    return 0;
}
