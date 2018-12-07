#include <winsock2.h>
#include <Iphlpapi.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <dce/dce.h>
#include <dce/dce_utils.h>

static int
GetAdapterEthernetAddr(unsigned char *addr,
                       unsigned int addrlen)
{
    PIP_ADAPTER_ADDRESSES pIpAdapters = NULL;
    unsigned long  err = 0;
    unsigned long  size = 0;
    unsigned long  totalSize = 0;
    unsigned long  flags = GAA_FLAG_SKIP_ANYCAST |
                           GAA_FLAG_SKIP_MULTICAST |
                           GAA_FLAG_SKIP_DNS_SERVER;

    err = GetAdaptersAddresses(
              AF_INET,
              0,
              NULL,
              NULL,
              &size);
    if (err == ERROR_BUFFER_OVERFLOW)
    {
        totalSize += size;
    }

    size = 0;
    err = GetAdaptersAddresses(
              AF_INET6,
              0,
              NULL,
              NULL,
              &size);
    if (err == ERROR_BUFFER_OVERFLOW)
    {
        totalSize += size;
    }
    pIpAdapters = calloc(1, totalSize);

    if (pIpAdapters)
    {
        size = totalSize;
        err = GetAdaptersAddresses(
                  AF_UNSPEC,
                  flags,
                  NULL,
                  pIpAdapters,
                  &size);
        if (err == ERROR_SUCCESS && size > 0)
        {
            addrlen = addrlen < pIpAdapters->PhysicalAddressLength ?
                      addrlen : pIpAdapters->PhysicalAddressLength;
            memcpy(addr, pIpAdapters->PhysicalAddress, addrlen);
        }
        free(pIpAdapters);
    }
    return err;
}


void dce_get_802_addr(dce_802_addr_t *addr, error_status_t *st)
{
    PIP_ADAPTER_INFO pAdapterInfo = NULL;
    unsigned32 dwError = 0;
    unsigned char eth_addr[6];

    dwError = GetAdapterEthernetAddr(eth_addr, sizeof(eth_addr));
    if (dwError)
    {
        *st = utils_s_802_cant_read;
    }
    else
    {
        memcpy(addr->eaddr,
               (void *) eth_addr,
               sizeof(eth_addr));
        *st = error_status_ok;
    }
}
