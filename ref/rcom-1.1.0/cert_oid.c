/*
 * SSLCERT_OID_to_string.
 */

/* Maps a 3-byte DER OID of the 2.5.4.x branch (id-at, prefix 55 04) to the name of the attribute,
 * long or short according to the third argument; 0 for the others. */
const char *SSLCERT_OID_to_string(oid, len, shortform)
    const unsigned char *oid;
    int len;
    int shortform;
{
    if (len != 3)
        return 0;

    if (oid[0] != 0x55 || oid[1] != 4)
        return 0;

    switch (oid[2]) {
    case 3:
        return shortform == 0 ? "commonName" : "CN";
    case 4:
        return shortform == 0 ? "surname" : "S";
    case 5:
        return shortform == 0 ? "serialNumber" : "SN";
    case 6:
        return shortform == 0 ? "countryName" : "C";
    case 7:
        return shortform == 0 ? "localityName" : "L";
    case 8:
        return shortform == 0 ? "stateOrProvinceName" : "ST";
    case 10:
        return shortform == 0 ? "organizationName" : "O";
    case 11:
        return shortform == 0 ? "organizationalUnitName" : "OU";
    case 12:
        return shortform == 0 ? "title" : "T";
    case 13:
        return shortform == 0 ? "description" : "D";
    case 42:
        return shortform == 0 ? "givenName" : "G";
    case 43:
        return shortform == 0 ? "initials" : "I";
    case 45:
        return shortform == 0 ? "uniqueIdentifier" : "UID";
    default:
        break;
    }

    return 0;
}
