//------------------------------------------------------------------------------
//  nstring.cc
//------------------------------------------------------------------------------
#include "util/nstring.h"
#include "mathlib/nmath.h"

const nString nString::Empty = nString();

//------------------------------------------------------------------------------
/**
*/
size_t
nString::GetFormattedStringLength(const char* format, va_list argList) const
{
    n_assert(format!=NULL);

    va_list argListSave;
    va_copy(argListSave, argList);
    int max_len = 0;

    for (const char* fm_tmp = format; *fm_tmp != '\0'; ++fm_tmp)
    {
        // find '%' character, but watch out for '%%'
        if (*fm_tmp != '%' || *(++fm_tmp) == '%')
        {
            ++max_len;
            continue;
        }

        int item_len = 0;

        // handle format
        int width = 0;
        for (; *fm_tmp != '\0'; ++fm_tmp)
        {
            // check for valid flags
            if (*fm_tmp == '#')
            {
                max_len += 2;   // for '0x'
            }
            else if (*fm_tmp == '*')
            {
                width = va_arg(argList, int);
            }
            else if (*fm_tmp == '-' || *fm_tmp == '+' || *fm_tmp == '0' || *fm_tmp == ' ')
            {
            }
            else
            {
                break;
            }
        }

        // get width
        if (width == 0)
        {
            width = atoi(fm_tmp);

            // and skip it
            while (*fm_tmp != '\0' && isdigit(*fm_tmp))
            {
                ++fm_tmp;
            }
        }
        n_assert(width >= 0);

        int precision = 0;
        if (*fm_tmp == '.')
        {
            ++fm_tmp; // skip '.'

            // get precision and skip it
            if (*fm_tmp == '*')
            {
                precision = va_arg(argList, int);
                ++fm_tmp;
            }
            else
            {
                precision = atoi(fm_tmp);

                while (*fm_tmp != '\0' && isdigit(*fm_tmp))
                {
                       ++fm_tmp;
                }
            }

            n_assert(precision >= 0);
        }

        // handle type modifier
        switch (*fm_tmp)
        {
            case 'h':
            case 'l':
            {
                ++fm_tmp;
                break;
                       }

            case 'F':
            case 'N':
            case 'L':
            case 'I':
                n_assert(false);  // unsupport
                break;
        }

        // handle type specifier
        switch (*fm_tmp)
        {
            // single-byte character
            case 'c':
            case 'C':
            {
                item_len = 2;
                va_arg(argList, int/*char promotion*/);
                break;
            }

            // single-byte character string
            case 's':
            case 'S':
            {
                const char* next_arg = va_arg(argList, const char*);
                if (next_arg == NULL)
                {
                    item_len = 6;  // "(null)"
                }
                else
                {
                    item_len = strlen(next_arg);
                    item_len = n_max(1, item_len);
                }

                break;
            }
        }

        // adjust item_len for strings
        if (item_len != 0)
        {
            if (precision != 0)
            {
                item_len = n_min(item_len, precision);
            }

            item_len = n_max(item_len, width);
        }
        else
        {
            switch (*fm_tmp)
            {
                // decimal/hexadecimal/octal  integers
                case 'd':
                case 'i':
                case 'u':
                case 'x':
                case 'X':
                case 'o':
                {
                    va_arg(argList, int);
                    item_len = 32;
                    item_len = n_max(item_len, width+precision);
                    break;
                }

                // floats
                case 'e':
                case 'g':
                case 'G':
                {
                    va_arg(argList, double);
                    item_len = 128;
                    item_len = n_max(item_len, width+precision);
                    break;
                }

                case 'f':
                {
                    double f;
                    char* str_temp;

                    // 312 == strlen("-1+(309 zeroes).")
                    // 309 zeroes == max precision of a double
                    // 6 == adjustment in case precision is not specified,
                    //   which means that the precision defaults to 6
                    str_temp = (char*)n_malloc(n_max(width, 312+precision+6));

                    f = va_arg(argList, double);
                    sprintf(str_temp, "%*.*f", width, precision+6, f);
                    item_len = strlen(str_temp);

                    n_free(str_temp);

                    break;
                }

                // pointer to void
                case 'p':
                {
                    va_arg(argList, void*);
                    item_len = 32;
                    item_len = n_max(item_len, width+precision);
                    break;
                }

                // skip (no output)
                case 'n':
                {
                    va_arg(argList, int*);
                    break;
                }

                default:
                    n_assert(false);  // unknown formatting option
            }
        }

        max_len += item_len;
    }

    va_end(argListSave);

    return max_len;
}
