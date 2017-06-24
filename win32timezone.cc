#define ICI_CORE
#include "fwd.h"
#include "object.h"
#include "str.h"

#ifdef  _WIN32
#include <windows.h>

namespace ici
{

static char *
convert(WCHAR *s)
{
    char buffer[256];
    int len;
    len = WideCharToMultiByte
    (
        CP_UTF8,
        WC_DEFAULTCHAR | WC_SEPCHARS,
        s,
        -1,
        buffer,
        sizeof buffer,
        NULL,
        NULL
    );
    return len == 0 ? NULL : buffer;
}

int set_timezone_vals(ici_struct *s)
{
    TIME_ZONE_INFORMATION		info;
    char                                *zone;
    long                                gmtoff;
    DWORD                               result;

    result = GetTimeZoneInformation(&info);
    switch (result)
    {
    case TIME_ZONE_ID_UNKNOWN:
        zone = convert(info.StandardName);
        gmtoff = info.Bias * 60;
        break;
    case TIME_ZONE_ID_STANDARD:
        zone = convert(info.StandardName);
        gmtoff = (info.Bias + info.StandardBias) * 60;
        break;
    case TIME_ZONE_ID_DAYLIGHT:
        zone = convert(info.DaylightName);
        gmtoff = (info.Bias + info.DaylightBias) * 60;
        break;
    case TIME_ZONE_ID_INVALID:
        return ici_get_last_win32_error();
    default:
        return set_error("unexpected result, %ld, from GetDynamicTimeZoneInformation", result);
    }
    if (zone == NULL)
    {
        return ici_get_last_win32_error();
    }
    if (set_val(objwsupof(s), SS(zone), 's', zone) || set_val(objwsupof(s), SS(gmtoff), 'i', &gmtoff))
    {
        return 1;
    }
    return 0;
}

} // namespace ici

#endif /* _WIN32 */
