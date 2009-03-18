#include "includes.h"

VOID
DirectoryFreeAttributeValues(
    PATTRIBUTE_VALUE pAttrValues,
    DWORD            dwNumValues
    )
{
    DWORD iValue = 0;

    for (; iValue < dwNumValues; iValue++)
    {
        PATTRIBUTE_VALUE pAttrValue = &pAttrValues[iValue];

        switch (pAttrValue->Type)
        {
            case DIRECTORY_ATTR_TYPE_UNICODE_STRING:

                if (pAttrValue->pwszStringValue)
                {
                    DirectoryFreeMemory(pAttrValue->pwszStringValue);
                }

                break;

            case DIRECTORY_ATTR_TYPE_ANSI_STRING:

                if (pAttrValue->pszStringValue)
                {
                    DirectoryFreeMemory(pAttrValue->pszStringValue);
                }

                break;

            default:

                break;
        }
    }

    DirectoryFreeMemory(pAttrValues);
}
