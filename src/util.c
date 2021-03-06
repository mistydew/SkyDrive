// Copyright (c) 2017-2019 mistydew
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>

void ParseParameters(int argc, char* argv[])
{
    for (int idx = 1; idx != argc; ++idx)
    {
        if (argv[idx][0] != '-')
            break;

        char str[1024] = {0};
        strcpy(str, argv[idx]);
        char strValue[1024] = {0};
        int is_idx = 1;
        int strLen = strlen(str);
        for (; is_idx != strLen && str[is_idx] != '='; ++is_idx);
        if (is_idx != strLen)
        {
            str[is_idx] = '\0';
            for (int i = 0; is_idx + 1 != strLen; ++i, ++is_idx)
                strValue[i] = str[is_idx + 1];
        }

        if (!strcmp(str, "-conf"))
            strcpy(mapArgs.sConf, strValue);
        else if (!strcmp(str, "-datadir"))
            strcpy(mapArgs.sDataDir, strValue);
        else if (!strcmp(str, "-daemon"))
            mapArgs.fDaemon = atoi(strValue);
        else if (!strcmp(str, "-printtoconsole"))
            mapArgs.fPrinttoConsole = atoi(strValue);
        else if (!strcmp(str, "-ip"))
            strcpy(mapArgs.sIP, strValue);
        else if (!strcmp(str, "-conn"))
            mapArgs.nConn = atoi(strValue);
        else if (!strcmp(str, "-port"))
            mapArgs.nPort = atoi(strValue);
        else if (!strcmp(str, "-threads"))
            mapArgs.nThreads = atoi(strValue);
        else if (!strcmp(str, "-mysqlusername"))
            strcpy(mapArgs.sMysqlUsername, strValue);
        else if (!strcmp(str, "-mysqlpassword"))
            strcpy(mapArgs.sMysqlPassword, strValue);
        else if (!strcmp(str, "-mysqldb"))
            strcpy(mapArgs.sMysqlDB, strValue);
    }
}

void LicenseInfo()
{
    char versionInfo[] = {"netCloud Core Daemon version v0.0.1.0\n"};
    fprintf(stdout, "%s", versionInfo);
}

void HelpMessage()
{
    char strUsage[] = {"Usage:\n\n"
                       "  ftpd [options]       Start netCloud Core Daemon\n\n"
                       "Options:\n\n"
                       "  -? / -h / -help\n"
                       "       This help message\n\n"
                       "  -version\n"
                       "       Print version and exit\n\n"
                       "  -conf=<file>\n"
                       "       Specify configuration file (default: netcloud.conf)\n\n"
                       "  -daemon\n"
                       "       Run in the background as a daemon and accept commands\n\n"
                       "  -datadir=<dir>\n"
                       "       Specify data directory\n\n"
                       "Connection options:\n\n"
                       "  -bind=<addr>\n"
                       "       Bind to given address and always listen to it (default: 127.0.0.1)\n\n"
                       "  -listen\n"
                       "       Accept connections from outside (default: 2)\n\n"
                       "  -port=<port>\n"
                       "       Listen for connections on <port> (default: 8888)\n\n"
                       "  -threads\n"
                       "       Set the number of threads to service RPC calls (default: 2)\n"};
    fprintf(stdout, "%s", strUsage);
}

void GetDefaultDataDir()
{
    // Unix: ~/.netCloud
    char* pszHome = getenv("HOME");
    if (pszHome == NULL || strlen(pszHome) == 0)
        strcpy(mapArgs.sDataDir, "/");
    else
        sprintf(mapArgs.sDataDir, "%s%s", pszHome, "/");

    sprintf(mapArgs.sDataDir, "%s%s%s", mapArgs.sDataDir, ".netCloud", "/");
}

void GetDataDir()
{
    GetDefaultDataDir();
    mkdir(mapArgs.sDataDir, 0775);
}

void GetConfigFile()
{
    sprintf(mapArgs.sConf, "%s%s", mapArgs.sDataDir, "netcloud.conf");
}

void ReadConfigFile(char* pathConfigFile)
{
    int fdConfig = open(pathConfigFile, O_RDONLY);

    char buf[4096] = {0};
    read(fdConfig, buf, sizeof(char) * 4096);

    int lens = 0;
    for (int idx = 0; idx != strlen(buf); ++idx)
    {
        if (buf[idx] == '\n')
            ++lens;
    }

    char ** argvConf = (char**)calloc(lens, sizeof(char*));
    for (int idx = 0; idx != lens; ++idx)
        argvConf[idx] = (char*)calloc(1, sizeof(char) * 64);

    for (int idx = 0, x = 0, y = 0; idx != strlen(buf) && y != lens; ++idx)
    {
        if (buf[idx] != '\n')
        {
            argvConf[y][x] = buf[idx];
            ++x;
        }
        else
        {
            ++y;
            x = 0;
        }
    }

    for (int idx = 0; idx != lens; ++idx)
    {
        char str[64] = {0};
        strcpy(str, argvConf[idx]);
        char strValue[16] = {0};
        int is_idx = 1;
        int strLen = strlen(str);
        for (; is_idx != strLen && str[is_idx] != '='; ++is_idx);
        if (is_idx != strLen)
        {
            str[is_idx] = '\0';
            for (int i = 0; is_idx + 1 != strLen; ++i, ++is_idx)
                strValue[i] = str[is_idx + 1];
        }

        if (!strcmp(str, "daemon"))
            mapArgs.fDaemon = atoi(strValue);
        else if (!strcmp(str, "printtoconsole"))
            mapArgs.fPrinttoConsole = atoi(strValue);
        else if (!strcmp(str, "ip"))
            strcpy(mapArgs.sIP, strValue);
        else if (!strcmp(str, "conn"))
            mapArgs.nConn = atoi(strValue);
        else if (!strcmp(str, "port"))
            mapArgs.nPort = atoi(strValue);
        else if (!strcmp(str, "threads"))
            mapArgs.nThreads = atoi(strValue);
        else if (!strcmp(str, "mysqlusername"))
            strcpy(mapArgs.sMysqlUsername, strValue);
        else if (!strcmp(str, "mysqlpassword"))
            strcpy(mapArgs.sMysqlPassword, strValue);
        else if (!strcmp(str, "mysqldb"))
            strcpy(mapArgs.sMysqlDB, strValue);
    }

    for (int idx = 0; idx != lens; ++idx)
        free(argvConf[idx]);
    free(argvConf);
    close(fdConfig);
}

int InitSocket()
{
    // Socket
    int sfd = socket(AF_INET, SOCK_STREAM, 0);

    // Set socket port reuse
    int reuse = 1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&reuse, sizeof(reuse));

    // Bind
    struct sockaddr_in ser;
    bzero(&ser, sizeof(struct sockaddr_in));
    ser.sin_family = AF_INET;
    ser.sin_port = htons(mapArgs.nPort);
    ser.sin_addr.s_addr = inet_addr(mapArgs.sIP);
    bind(sfd, (struct sockaddr*)&ser, sizeof(ser));

    // Listen
    listen(sfd, 20);

    return sfd;
}
