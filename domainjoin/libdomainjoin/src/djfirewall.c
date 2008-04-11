/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software    2007-2008
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as 
 * published by the Free Software Foundation; either version 2.1 of 
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 */

#include "domainjoin.h"
#include "djfirewall.h"
#include <djmodule.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlsave.h>

#define GCE(x) GOTO_CLEANUP_ON_CENTERROR((x))
#define DC_PORT_COUNT 6

#define CONNECTION_TIMEOUT  15

static const char *servicesPath = "/etc/vmware/firewall/services.xml";

typedef struct
{
    struct sockaddr_in to;
    BOOLEAN udp;
    time_t startTime;
    int retryCount;
    int success;
    const char *sendData;
    size_t sendDataLen;

    int sock;
} PortCheck;

static CENTERROR CloseSocket(int *sock)
{
    int error;
    if(*sock == -1)
        return CENTERROR_SUCCESS;
    error = close(*sock);
    *sock = -1;
    if(error < 0)
        return CTMapSystemError(errno);
    return CENTERROR_SUCCESS;
}

static CENTERROR SetNonblocking(int sock)
{
    if(fcntl(sock, F_SETFL, O_NONBLOCK) < 0)
        return CTMapSystemError(errno);
    return CENTERROR_SUCCESS;
}

static CENTERROR CreateSocket(BOOLEAN udp, int *sock)
{
    int sockType = SOCK_STREAM;
    if(udp)
        sockType = SOCK_DGRAM;

    *sock = socket(PF_INET, sockType, 0);
    if(*sock < 0)
        return CTMapSystemError(errno);
    return CENTERROR_SUCCESS;
}

static CENTERROR ConnectSocket(int sock, struct sockaddr_in *to)
{
    if(connect(sock, (struct sockaddr *)to, sizeof(*to)) < 0)
    {
        return CTMapSystemError(errno);
    }
    return CENTERROR_SUCCESS;
}

static CENTERROR GetSocketError(int sock, CENTERROR *error)
{
    int errnoError;
    socklen_t optLen = sizeof(errnoError);
    if(getsockopt(sock, SOL_SOCKET, SO_ERROR, &errnoError, &optLen) < 0)
    {
        return CTMapSystemError(errno);
    }
    if(optLen != sizeof(errnoError))
        return CENTERROR_INVALID_VALUE;

    *error = CTMapSystemError(errnoError);
    return CENTERROR_SUCCESS;
}

static CENTERROR CheckPorts(PortCheck *checks, int checkNum, int tcptimeout)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    int i;
    time_t currentTime;
    fd_set dgramFds, streamFds;
    int nfds = 0;
    struct timeval timeout;
    char recvBuffer[1024];

    for(i = 0; i < checkNum; i++)
    {
        checks[i].startTime = 0;
        if(checks[i].retryCount < 1)
            checks[i].retryCount = 1;
        if(checks[i].udp)
        {
            GCE(ceError = CreateSocket(checks[i].udp, &checks[i].sock));
            GCE(ceError = SetNonblocking(checks[i].sock));
            GCE(ceError = ConnectSocket(checks[i].sock, &checks[i].to));
        }
        else
            checks[i].sock = -1;
    }

    while(1)
    {
        FD_ZERO(&dgramFds);
        FD_ZERO(&streamFds);
        time(&currentTime);
        nfds = -1;
        for(i = 0; i < checkNum; i++)
        {
            if(!checks[i].udp &&
                checks[i].sock != -1 &&
                currentTime - checks[i].startTime > tcptimeout)
            {
                DJ_LOG_INFO("Timing out connection request");
                GCE(ceError = CloseSocket(&checks[i].sock));
            }
            if(!checks[i].udp &&
                checks[i].sock == -1 && !checks[i].success)
            {
                if(checks[i].retryCount == 0)
                    continue;
                GCE(ceError = CreateSocket(checks[i].udp, &checks[i].sock));
                GCE(ceError = SetNonblocking(checks[i].sock));
                DJ_LOG_INFO("Starting non-blocking tcp connection");
                ceError = ConnectSocket(checks[i].sock, &checks[i].to);
                if(ceError == CTMapSystemError(EINPROGRESS))
                    ceError = CENTERROR_SUCCESS;
                GCE(ceError);

                checks[i].startTime = currentTime;
                checks[i].retryCount--;
            }

            if(checks[i].sock == -1)
                continue;
            if(currentTime - checks[i].startTime > 1 &&
                checks[i].udp)
            {
                if(checks[i].retryCount == 0)
                {
                    GCE(ceError = CloseSocket(&checks[i].sock));
                    continue;
                }

                DJ_LOG_INFO("Sending UDP packet");
                send(checks[i].sock, checks[i].sendData, checks[i].sendDataLen, 0);
                checks[i].retryCount--;
                checks[i].startTime = currentTime;
            }

            if(checks[i].udp)
            {
                FD_SET(checks[i].sock, &dgramFds);
            }
            else
            {
                FD_SET(checks[i].sock, &streamFds);
            }
            if(checks[i].sock > nfds)
            {
                nfds = checks[i].sock;
            }
        }
        nfds++;
        if(nfds < 1)
        {
            DJ_LOG_INFO("Results obtained for all ports");
            break;
        }
        
        timeout.tv_sec = 0;
        timeout.tv_usec = 500000;
        if(select(nfds, &dgramFds, &streamFds, NULL, &timeout) < 0)
            GCE(ceError = CTMapSystemError(errno));

        for(i = 0; i < checkNum; i++)
        {
            if(checks[i].sock == -1)
                continue;
            if(FD_ISSET(checks[i].sock, &dgramFds))
            {
                ssize_t recvLen = recv(checks[i].sock,
                    recvBuffer, sizeof(recvBuffer), 0);
                if(recvLen > 0)
                {
                    DJ_LOG_INFO("Received UDP packet");
                    checks[i].success = 1;
                    GCE(ceError = CloseSocket(&checks[i].sock));
                }
                else
                    ceError = CTMapSystemError(errno);

                if(ceError == CTMapSystemError(ECONNREFUSED))
                {
                    DJ_LOG_INFO("udp recv failed with connection refused\n");
                    //This eats the previous error and declares the port
                    //unreachable
                    GCE(ceError = CloseSocket(&checks[i].sock));
                }
                GCE(ceError);
            }
            else if(FD_ISSET(checks[i].sock, &streamFds))
            {
                CENTERROR subError = CENTERROR_SUCCESS;
                GCE(ceError = GetSocketError(checks[i].sock, &subError));

                if(CENTERROR_IS_OK(subError))
                {
                    DJ_LOG_INFO("Successfully connected");
                    checks[i].success = 1;
                }
                else if(subError == CTMapSystemError(EHOSTUNREACH))
                    DJ_LOG_INFO("host unreachable");
                else if(subError == CTMapSystemError(ECONNREFUSED))
                    DJ_LOG_INFO("connection refused");
                else
                {
                    //Don't fail this function because the connect failed
                    DJ_LOG_INFO("connect failed %d", subError);
                }

                GCE(ceError = CloseSocket(&checks[i].sock));
            }
        }
    }

cleanup:
    for(i = 0; i < checkNum; i++)
    {
        if(checks[i].sock != -1)
            CloseSocket(&checks[i].sock);
    }
    return ceError;
}

typedef struct
{
    PSTR dc;
    PortCheck ports[DC_PORT_COUNT];
} DCPortCheck;

void CheckDCPorts(DCPortCheck *check, int timeout, LWException **exc)
{
    int i;
    struct in_addr dcIp;
	char ntpRequest[] = {
		0xdb,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
	};
    struct hostent* pHostent = NULL;

    for(i = 0; i < 3; i++)
    {
        pHostent = gethostbyname(check->dc);
        if (pHostent == NULL) {
            if (h_errno == TRY_AGAIN) {
                sleep(1);
                continue;
            }
            i = 3;
            break;
        }
        dcIp = *((struct in_addr **)pHostent->h_addr_list)[0];
        break;
    }
    if(i == 3)
    {
        LW_RAISE_EX(exc, CENTERROR_DOMAINJOIN_UNRESOLVED_DOMAIN_NAME, "Unable to resolve DC name", "Resolving '%s' failed. Check that the domain name is correctly entered. Also check that your DNS server is reachable, and that your system is configured to use DNS in nsswitch.", check->dc);
        goto cleanup;
    }
    DJ_LOG_INFO("Looked up domain");

	memset(check->ports, 0, sizeof(*check->ports) * DC_PORT_COUNT);

	check->ports[0].to.sin_port = htons(123);
	check->ports[0].sendData = ntpRequest;
	check->ports[0].sendDataLen = sizeof(ntpRequest);
	check->ports[0].retryCount = timeout;
    check->ports[0].udp = TRUE;

	check->ports[1].to.sin_port = htons(88);
	check->ports[2].to.sin_port = htons(139);
	check->ports[3].to.sin_port = htons(389);
	check->ports[4].to.sin_port = htons(445);
	check->ports[5].to.sin_port = htons(464);

	for(i = 0; i < DC_PORT_COUNT; i++)
	{
	    check->ports[i].to.sin_family = AF_INET;
	    check->ports[i].to.sin_addr = dcIp;
	}

	LW_CLEANUP_CTERR(exc, CheckPorts(check->ports, DC_PORT_COUNT, timeout));

cleanup:
    ;
}

static xmlNodePtr GetNewXmlNode(xmlDocPtr destDoc)
{
    xmlDocPtr doc = NULL;
    xmlNodePtr root = NULL;
    const char *addXmlContent =
"  <service id='LikewiseEnterprise'>\n"
"    <id>LikewiseEnterprise</id>\n"
"    <rule id='0000'>\n"
"      <direction>outbound</direction>\n"
"      <protocol>udp</protocol>\n"
"      <port type='dst'>53</port>\n"
"    </rule>\n"
"    <rule id='0001'>\n"
"      <direction>outbound</direction>\n"
"      <protocol>tcp</protocol>\n"
"      <port type='dst'>53</port>\n"
"    </rule>\n"
"    <rule id='0002'>\n"
"      <direction>outbound</direction>\n"
"      <protocol>tcp</protocol>\n"
"      <port type='dst'>88</port>\n"
"    </rule>\n"
"    <rule id='0003'>\n"
"      <direction>outbound</direction>\n"
"      <protocol>udp</protocol>\n"
"      <port type='dst'>88</port>\n"
"    </rule>\n"
"    <rule id='0004'>\n"
"      <direction>outbound</direction>\n"
"      <protocol>udp</protocol>\n"
"      <port type='dst'>123</port>\n"
"    </rule>\n"
"    <rule id='0005'>\n"
"      <direction>outbound</direction>\n"
"      <protocol>udp</protocol>\n"
"      <port type='dst'>137</port>\n"
"    </rule>\n"
"    <rule id='0006'>\n"
"      <direction>outbound</direction>\n"
"      <protocol>tcp</protocol>\n"
"      <port type='dst'>139</port>\n"
"    </rule>\n"
"    <rule id='0007'>\n"
"      <direction>outbound</direction>\n"
"      <protocol>udp</protocol>\n"
"      <port type='dst'>389</port>\n"
"    </rule>\n"
"    <rule id='0008'>\n"
"      <direction>outbound</direction>\n"
"      <protocol>tcp</protocol>\n"
"      <port type='dst'>389</port>\n"
"    </rule>\n"
"    <rule id='0009'>\n"
"      <direction>outbound</direction>\n"
"      <protocol>tcp</protocol>\n"
"      <port type='dst'>445</port>\n"
"    </rule>\n"
"    <rule id='0010'>\n"
"      <direction>outbound</direction>\n"
"      <protocol>udp</protocol>\n"
"      <port type='dst'>464</port>\n"
"    </rule>\n"
"    <rule id='0011'>\n"
"      <direction>outbound</direction>\n"
"      <protocol>tcp</protocol>\n"
"      <port type='dst'>464</port>\n"
"    </rule>\n"
"  </service>\n";

    doc = xmlReadMemory(addXmlContent, strlen(addXmlContent), "", NULL,
            XML_PARSE_NONET);
    if(doc == NULL)
        return NULL;
    root = xmlDocGetRootElement(doc);
    if(root != NULL)
        root = xmlDocCopyNode(root, destDoc, 1);
    xmlFreeDoc(doc);

    return root;
}

CENTERROR
DJUpdateServicesFile(
    PCSTR filename,
    BOOLEAN enable,
    BOOLEAN *modified
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR tempFilename = NULL;
    BOOLEAN same;

    xmlDocPtr doc = NULL;
    xmlNodePtr root;
    xmlNodePtr child;
    xmlNodePtr addNode = NULL;
    xmlSaveCtxtPtr saveContext = NULL;

    LIBXML_TEST_VERSION;

    DJ_LOG_INFO("Reading %s", filename);
    doc = xmlReadFile(filename, NULL, XML_PARSE_NONET);
    if(doc == NULL)
        GCE(ceError = CENTERROR_DOMAINJOIN_INVALID_FIREWALLCFG);
    root = xmlDocGetRootElement(doc);
    if(root == NULL)
        GCE(ceError = CENTERROR_DOMAINJOIN_INVALID_FIREWALLCFG);
    if(strcmp((const char *)root->name, "ConfigRoot"))
    {
        DJ_LOG_ERROR("Expected node '%s' instead of '%s'", "ConfigRoot", root->name);
        GCE(ceError = CENTERROR_DOMAINJOIN_INVALID_FIREWALLCFG);
    }

    child = root->children;
    while(child != NULL)
    {
        if(child->type == XML_ELEMENT_NODE &&
                !strcmp((const char *)child->name, "service"))
        {
            xmlChar *id = xmlGetProp(child, (const xmlChar *)"id");
            BOOLEAN equals = FALSE;
            if(id != NULL)
            {
                equals = !strcmp((const char *)id, "LikewiseEnterprise");
                xmlFree(id);
            }
            if(equals)
            {
                DJ_LOG_INFO("Removing existing LikewiseEnterprise firewall entry");
                xmlUnlinkNode(child);
                xmlFreeNode(child);
            }
        }
        child = child->next;
    }

    if(enable)
    {
        DJ_LOG_INFO("Adding LikewiseEnterprise firewall entry");
        addNode = GetNewXmlNode(doc);
        if(addNode == NULL)
            GCE(ceError = CENTERROR_OUT_OF_MEMORY);

        if(xmlAddChild(root, addNode) == NULL)
            GCE(ceError = CENTERROR_DOMAINJOIN_INVALID_FIREWALLCFG);
        addNode = NULL;
    }

    GCE(ceError = CTAllocateStringPrintf(&tempFilename,
                "%s.new", filename));
    saveContext = xmlSaveToFilename(tempFilename, NULL, 0);
    if(saveContext == NULL)
        GCE(ceError = CENTERROR_OUT_OF_MEMORY);

    if(xmlSaveDoc(saveContext, doc) < 0)
    {
        GCE(ceError = CTMapSystemError(errno));
    }
    
    if(xmlSaveClose(saveContext) < 0)
    {
        GCE(ceError = CTMapSystemError(errno));
    }
    saveContext = NULL;

    GCE(ceError = CTFileContentsSame(filename, tempFilename, &same));
    if(same)
    {
        DJ_LOG_INFO("File %s unmodified", filename);
        GCE(ceError = CTRemoveFile(tempFilename));
    }
    else
    {
        DJ_LOG_INFO("File %s modified", filename);
        GCE(ceError = CTCloneFilePerms(filename, tempFilename));
        GCE(ceError = CTBackupFile(filename));
        GCE(ceError = CTMoveFile(tempFilename, filename));
    }
    if(modified != NULL)
        *modified = !same;

cleanup:
    if(doc != NULL)
        xmlFreeDoc(doc);
    if(addNode != NULL)
        xmlFreeNode(addNode);
    if(saveContext != NULL)
        xmlSaveClose(saveContext);
    CT_SAFE_FREE_STRING(tempFilename);
    return ceError;
}

CENTERROR
DJConfigureFirewallForAuth(
    const char * testPrefix,
    BOOLEAN enable
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR finalFilename = NULL;
    BOOLEAN exists;

    if(testPrefix == NULL)
        testPrefix = "";

    if(enable)
        DJ_LOG_INFO("Configuring ESX firewall for Likewise Enterprise");
    else
        DJ_LOG_INFO("Deconfiguring ESX firewall for Likewise Enterprise");

    GCE(ceError = CTAllocateStringPrintf(&finalFilename,
                "%s/etc/vmware/firewall/services.xml", testPrefix));

    GCE(ceError = CTCheckFileOrLinkExists(finalFilename, &exists));
    if(!exists)
    {
        DJ_LOG_INFO("Skipping because '%s' doesn't exist", finalFilename);
        goto cleanup;
    }

    if(!enable)
    {
        // Remove our product from the enabled services list before it is
        // removed from the list of products.
        ceError = CTCaptureOutputWithStderr("/usr/sbin/esxcfg-firewall -d LikewiseEnterprise", TRUE, NULL);
        if(ceError == CENTERROR_COMMAND_FAILED)
            ceError = CENTERROR_SUCCESS;
        GCE(ceError);
    }

    GCE(ceError = DJUpdateServicesFile(finalFilename, enable, NULL));

    if(enable)
    {
        // Add our product to the list of enabled services. Let standard error
        // and standard out pass through to the screen (normally this doesn't
        // output anything).
        DJ_LOG_VERBOSE("Running '/usr/sbin/esxcfg-firewall -e LikewiseEnterprise'");
        ceError = CTRunCommand("/usr/sbin/esxcfg-firewall -e LikewiseEnterprise");
        GCE(ceError);
    }

cleanup:
    CT_SAFE_FREE_STRING(finalFilename);
    return ceError;
}

static CENTERROR DupDCPortCheck(DCPortCheck *dest, const DCPortCheck *src)
{
    memcpy(dest->ports, src->ports, sizeof(src->ports));
    return CTStrdup(src->dc, &dest->dc);
}

static void FreeDCPortCheckContents(DCPortCheck *check)
{
    if(check == NULL)
        return;
    CT_SAFE_FREE_STRING(check->dc);
}

static void CachePortCheck(PCSTR domainName, ModuleState *state, int timeout, DCPortCheck *check, LWException **exc)
{
    memset(check, 0, sizeof(*check));

    if(state != NULL && state->moduleData != NULL)
    {
        //Free the cached results
        FreeDCPortCheckContents(((DCPortCheck *)state->moduleData));
        CT_SAFE_FREE_MEMORY(state->moduleData);
        goto cleanup;
    }

    DJ_LOG_INFO("Getting DC");
    LW_CLEANUP_CTERR(exc, DJGetDomainDC(domainName, &check->dc));
    if(IsNullOrEmptyString(check->dc))
    {
        //Maybe cldap is blocked? We'll default to the domain name
        CT_SAFE_FREE_STRING(check->dc);
        LW_CLEANUP_CTERR(exc, CTStrdup(domainName, &check->dc));
    }
    DJ_LOG_INFO("Starting port check");
    LW_TRY(exc, CheckDCPorts(check, timeout, &LW_EXC));
    if(state != NULL)
    {
        LW_CLEANUP_CTERR(exc, CTAllocateMemory(sizeof(DCPortCheck), &state->moduleData));
        LW_CLEANUP_CTERR(exc, DupDCPortCheck((DCPortCheck *)state->moduleData, check));
    }

cleanup:
    ;
}

static void ReadCachedPortCheck(PCSTR domainName, ModuleState *state, DCPortCheck *check, LWException **exc)
{
    memset(check, 0, sizeof(*check));

    if(state != NULL && state->moduleData != NULL)
    {
        //Use the cached results
        LW_CLEANUP_CTERR(exc, DupDCPortCheck(check, ((DCPortCheck *)state->moduleData)));
        goto cleanup;
    }

    LW_TRY(exc, CachePortCheck(domainName, state, 15, check, &LW_EXC));

cleanup:
    ;
}

static QueryResult QueryFirewall(const JoinProcessOptions *options, LWException **exc)
{
    PSTR tempFilename = NULL;
    BOOLEAN modified = FALSE;
    BOOLEAN exists;
    PSTR tempDir = NULL;
    QueryResult result = CannotConfigure;
    DCPortCheck check;
    int i;
    ModuleState *state = DJGetModuleStateByName(
            (JoinProcessOptions *)options, "firewall");

    memset(&check, 0, sizeof(check));

    if(!options->joiningDomain)
    {
        result = NotApplicable;
        goto cleanup;
    }

    if(options->domainName == NULL)
    {
        LW_RAISE_EX(exc, CENTERROR_INVALID_DOMAINNAME, "Invalid domain name", "Please specifiy a non-blank domain name");
        goto cleanup;
    }

    LW_CLEANUP_CTERR(exc, CTCheckFileOrLinkExists(servicesPath, &exists));
    if(exists)
    {
        LW_CLEANUP_CTERR(exc, CTCreateTempDirectory(&tempDir));
        LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&tempFilename,
                    "%s/%s", tempDir, strrchr(servicesPath, '/') + 1));
        LW_CLEANUP_CTERR(exc, CTCopyFileWithOriginalPerms(servicesPath, tempFilename));
        LW_CLEANUP_CTERR(exc, DJUpdateServicesFile(tempFilename, options->joiningDomain, &modified));
    }
    
    if(options->joiningDomain)
    {
        int timeout = 15;

        //We know that the vmware esx firewall configuration needs to be
        //updated, so let's let this port check fail fast. If the user found
        //some other way of opening the ports, this still gives them a
        //chance.
        if(modified)
            timeout = 2;

        LW_TRY(exc, CachePortCheck(options->domainName,
                    state, timeout, &check, &LW_EXC));

        for(i = 0; i < DC_PORT_COUNT; i++)
        {
            if(!check.ports[i].success)
                break;
        }
        if(i == DC_PORT_COUNT)
        {
            //All ports are open
            result = FullyConfigured;
            goto cleanup;
        }
    }
    else
        result = FullyConfigured;

    if(!exists)
    {
        goto cleanup;
    }

    if(!modified)
    {
        goto cleanup;
    }

    result = NotConfigured;

cleanup:
    if(tempDir != NULL)
    {
        CTRemoveDirectory(tempDir);
        CT_SAFE_FREE_STRING(tempDir);
    }
    CT_SAFE_FREE_STRING(tempFilename);
    FreeDCPortCheckContents(&check);
    return result;
}

static void DoFirewall(JoinProcessOptions *options, LWException **exc)
{
    LW_CLEANUP_CTERR(exc, DJConfigureFirewallForAuth(NULL, options->joiningDomain));

cleanup:
    ;
}

static PSTR GetFirewallDescription(const JoinProcessOptions *options, LWException **exc)
{
    PSTR ret = NULL;
    PSTR temp = NULL;
    DCPortCheck check;
    int i;
    QueryResult configured;
    ModuleState *optionsState = DJGetModuleStateByName(
            (JoinProcessOptions *)options, "firewall");

    memset(&check, 0, sizeof(check));

    if(optionsState)
        configured = optionsState->lastResult;
    else
    {
        LW_TRY(exc, configured = QueryFirewall(options, &LW_EXC));
    }
    switch(configured)
    {
        case FullyConfigured:
            LW_CLEANUP_CTERR(exc, CTStrdup("All necessary ports are currently open to the DC.\n", &ret));
            goto cleanup;
        case NotConfigured:
            LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&ret,
                "The domain controller could not be reached. The following ports must be opened to the domain controller:\n"
                "\t53  UDP/TCP\n"
                "\t88  UDP/TCP\n"
                "\t123 UDP\n"
                "\t137 UDP\n"
                "\t139 TCP\n"
                "\t389 UDP/TCP\n"
                "\t445 TCP\n"
                "\t464 UDP/TCP\n"
                "By default, all outgoing ports are blocked on VMware ESX. This program will open those ports by adding a service to %s, and enable the service by running '%s'.\n",
                servicesPath,
                "/usr/sbin/esxcfg-firewall -e LikewiseEnterprise"));
            goto cleanup;
        default:
            break;
    }

    LW_TRY(exc, ReadCachedPortCheck(options->domainName, optionsState, &check, &LW_EXC));

    LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&ret, "Some required ports on the domain controller could not be contacted. Please update your firewall settings to ensure that the following ports are open to '%s':\n"
                "\t88  UDP\n"
                "\t137 UDP\n"
                "\t389 UDP\n"
                "\t464 UDP"
                , check.dc));

    for(i = 0; i < DC_PORT_COUNT; i++)
    {
        if(!check.ports[i].success)
        {
            PCSTR type = "TCP";
            if(check.ports[i].udp)
                type = "UDP";
            temp = ret;
            ret = NULL;
            LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&ret, "%s\n\t%d %s",
                        temp, ntohs(check.ports[i].to.sin_port), type));
            CT_SAFE_FREE_STRING(temp);
        }
    }

cleanup:
    FreeDCPortCheckContents(&check);
    CT_SAFE_FREE_STRING(temp);
    return ret;
}

void FreeFirewallData(const JoinProcessOptions *options, ModuleState *state)
{
    FreeDCPortCheckContents((DCPortCheck *)state->moduleData);
    CT_SAFE_FREE_MEMORY(state->moduleData);
}

const JoinModule DJFirewall = { TRUE, "firewall", "open ports to DC", QueryFirewall, DoFirewall, GetFirewallDescription, FreeFirewallData };
