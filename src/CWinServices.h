#pragma once

#ifndef __CWinServices_h__
#define __CWinServices_h__

#include <windows.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <string>
#include <unordered_map>
#include <sstream>
#include "Common.h"
#pragma comment(lib, "pdh.lib")

using namespace std;

class CWinServices
{
private:
    std::unordered_map<DWORD, std::tstring> _cache;
    std::unordered_map<std::tstring, DWORD> _reverseCache;

public:
    std::tstring GetServiceDisplayName(DWORD processId)
    {
        if (_cache.empty())
        {
            EnumerateServices();
        }

        if (_cache.find(processId) == _cache.end())
        {
            return NULL;
        }

        return _cache[processId];
    }

    DWORD GetServicePID(std::tstring serviceName)
    {
        if (_cache.empty())
        {
            EnumerateServices();
        }

        if (_reverseCache.find(serviceName) == _reverseCache.end())
        {
            return NULL;
        }

        return _reverseCache[serviceName];
    }

private:
    void EnumerateServices()
    {
        SC_HANDLE hSCM = OpenSCManager(NULL, NULL,
            SC_MANAGER_ENUMERATE_SERVICE | SC_MANAGER_CONNECT);

        if (hSCM == NULL)
        {
            return;
        }

        DWORD bufferSize = 0;
        DWORD requiredBufferSize = 0;
        DWORD totalServicesCount = 0;
        EnumServicesStatusEx(hSCM,
            SC_ENUM_PROCESS_INFO,
            SERVICE_WIN32,
            SERVICE_STATE_ALL,
            nullptr,
            bufferSize,
            &requiredBufferSize,
            &totalServicesCount,
            nullptr,
            nullptr);

        std::vector<BYTE> buffer(requiredBufferSize);
        EnumServicesStatusEx(hSCM,
            SC_ENUM_PROCESS_INFO,
            SERVICE_WIN32,
            SERVICE_STATE_ALL,
            buffer.data(),
            buffer.size(),
            &requiredBufferSize,
            &totalServicesCount,
            nullptr,
            nullptr);

        LPENUM_SERVICE_STATUS_PROCESS services =
            reinterpret_cast<LPENUM_SERVICE_STATUS_PROCESS>(buffer.data());

        for (unsigned int i = 0; i < totalServicesCount; ++i)
        {
            ENUM_SERVICE_STATUS_PROCESS service = services[i];

            _cache.insert({ service.ServiceStatusProcess.dwProcessId, service.lpServiceName });
            _reverseCache.insert({ service.lpServiceName, service.ServiceStatusProcess.dwProcessId });
        }

        (void)CloseServiceHandle(hSCM);
    }
};

#endif __CWinServices_h__