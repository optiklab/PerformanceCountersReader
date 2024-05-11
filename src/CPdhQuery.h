#pragma once

#ifndef __CPdhQuery_h__
#define __CPdhQuery_h__

#include <windows.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <string>
#include <map>
#include <sstream>
#include <vector>
#include "Common.h"
#pragma comment(lib, "pdh.lib")

using namespace std;

class CPdhQuery
{
private:
    DWORD m_serviceAssignedPID;

    // Must have process metric
    std::tstring m_processName;
    std::tstring m_searchInstance;
    PDH_HQUERY m_pdhQuery;
    PDH_STATUS m_pdhStatus;
    PDH_HCOUNTER m_pdhCounter;

    // User defined/desired process metrics.
    vector<std::tstring> m_processMetrics;

    // User defined/desired metrics queries and counters.
    vector<PDH_HQUERY> m_queries;
    vector<PDH_HCOUNTER> m_counters;

public:

    // Inner exception class to report error.
    class CException
    {
    public:
        CException(std::tstring const& errorMsg) : m_errorMsg(errorMsg) {}
        std::tstring What() const { return m_errorMsg; }
    private:
        std::tstring m_errorMsg;
    };

    //! Constructor
    explicit CPdhQuery(vector<std::tstring> processMetrics, DWORD serviceAssignedPID, tstring processName)
        : m_pdhQuery(NULL)
        , m_pdhStatus(ERROR_SUCCESS)
        , m_pdhCounter(NULL)
        , m_processMetrics(processMetrics)
        , m_serviceAssignedPID(serviceAssignedPID)
        , m_processName(processName)
    {
        m_searchInstance = _T("\\Process(") + m_processName + _T(")\\ID Process");

        for (int i = 0; i < m_processMetrics.size(); i++)
        {
            m_processMetrics[i] = _T("\\Process(") + m_processName + _T(")\\") + m_processMetrics[i];
        }

        if (m_pdhStatus = PdhOpenQuery(NULL, 0, &m_pdhQuery))
        {
            throw CException(GetErrorString(m_pdhStatus, m_searchInstance));
        }

        // Specify a counter object with a wildcard for the instance.
        if (m_pdhStatus = PdhAddCounter(
                m_pdhQuery,
                m_searchInstance.c_str(),
                0,
                &m_pdhCounter)
            )
        {
            GetErrorString(m_pdhStatus, m_searchInstance);
            throw CException(GetErrorString(m_pdhStatus, m_searchInstance));
        }

        // TODO
        // If m_parentProcessPID <> 0
        //   Find process with Parent PID == m_parentProcessPID
        //   Track metrics on this only process
        // else
        //   Do whatether metrics prescribes

        for (int i = 0; i < m_processMetrics.size(); i++)
        {
            PDH_HQUERY pdhQuery;
            PDH_STATUS pdhStatus;
            PDH_HCOUNTER pdhCounter;

            if (pdhStatus = PdhOpenQuery(NULL, 0, &pdhQuery))
            {
                throw CException(GetErrorString(pdhStatus, m_processMetrics[i]));
            }

            // Specify a counter object with a wildcard for the instance.
            if (pdhStatus = PdhAddCounter(
                    pdhQuery,
                    m_processMetrics[i].c_str(),
                    0,
                    &pdhCounter)
                )
            {
                GetErrorString(pdhStatus, m_processMetrics[i]);
                throw CException(GetErrorString(pdhStatus, m_processMetrics[i]));
            }

            m_queries.push_back(pdhQuery);
            m_counters.push_back(pdhCounter);
        }
    }

    //! Destructor. The counter and query handle will be closed.
    ~CPdhQuery()
    {
        for (int i = 0; i < m_queries.size(); i++)
        {
            PdhCloseQuery(m_queries[i]);
        }

        m_counters.clear();

        m_pdhCounter = NULL;
        if (m_pdhQuery)
            PdhCloseQuery(m_pdhQuery);
    }

    //! Collect all the data since the last sampling period.
    std::map<std::tstring, double> CollectQueryData()
    {
        std::map<std::tstring, double> collectedData;

        while (true)
        {
            // Collect the sampling data. This might cause
            // PdhGetFormattedCounterArray to fail because some query type
            // requires two collections (or more?). If such scenario is
            // detected, the while loop will retry.
            if (m_pdhStatus = PdhCollectQueryData(m_pdhQuery))
            {
                throw CException(GetErrorString(m_pdhStatus, m_searchInstance));
            }

            // Size of the pItems buffer
            DWORD bufferSize = 0;

            // Number of items in the pItems buffer
            DWORD itemCount = 0;

            PDH_FMT_COUNTERVALUE_ITEM* pdhItems = NULL;

            // Call PdhGetFormattedCounterArray once to retrieve the buffer
            // size and item count. As long as the buffer size is zero, this
            // function should return PDH_MORE_DATA with the appropriate
            // buffer size.
            m_pdhStatus = PdhGetFormattedCounterArray(
                m_pdhCounter,
                PDH_FMT_DOUBLE,
                &bufferSize,
                &itemCount,
                pdhItems);

            // If the returned value is nto PDH_MORE_DATA, the function
            // has failed.
            if (PDH_MORE_DATA != m_pdhStatus)
            {
                throw CException(GetErrorString(m_pdhStatus, m_searchInstance));
            }

            std::vector<unsigned char> buffer(bufferSize);
            pdhItems = (PDH_FMT_COUNTERVALUE_ITEM*)(&buffer[0]);

            m_pdhStatus = PdhGetFormattedCounterArray(
                m_pdhCounter,
                PDH_FMT_DOUBLE,
                &bufferSize,
                &itemCount,
                pdhItems);

            if (ERROR_SUCCESS != m_pdhStatus)
            {
                continue;
            }

            tstring name;
            // Everything is good, mine the data.
            for (DWORD i = 0; i < itemCount; i++) {
                //name = tstring(pdhItems[i].szName);
                name = m_searchInstance;

                // Add no after duplicate processes eg chrome, chrome#2
                map<std::tstring, double>::iterator it;
                it = collectedData.find(name);
                u_int count = 2;
                string appData = "";
                wstring tmp;
                while (it != collectedData.end()) {
                    appData = "#" + to_string(count);
                    count++;
                    tmp = tstring(appData.begin(), appData.end());
                    it = collectedData.find(name + tmp);
                }

                if ((DWORD)pdhItems[i].FmtValue.doubleValue != m_serviceAssignedPID)
                    break;

                collectedData.insert(
                    std::make_pair(
                        name, //std::tstring(pdhItems[i].szName),
                        pdhItems[i].FmtValue.doubleValue)
                );
            }

            for (int i = 0; i < m_processMetrics.size(); i++)
            {
                pdhItems = NULL;
                bufferSize = itemCount = 0;

                PDH_STATUS pdhStatus;

                // Collect the sampling data. This might cause
                // PdhGetFormattedCounterArray to fail because some query type
                // requires two collections (or more?). If such scenario is
                // detected, the while loop will retry.
                if (pdhStatus = PdhCollectQueryData(m_queries[i]))
                {
                    throw CException(GetErrorString(pdhStatus, m_processMetrics[i]));
                }

                // Size of the pItems buffer
                DWORD bufferSize = 0;

                // Number of items in the pItems buffer
                DWORD itemCount = 0;

                PDH_FMT_COUNTERVALUE_ITEM* pdhItems = NULL;

                // Call PdhGetFormattedCounterArray once to retrieve the buffer
                // size and item count. As long as the buffer size is zero, this
                // function should return PDH_MORE_DATA with the appropriate
                // buffer size.
                pdhStatus = PdhGetFormattedCounterArray(
                    m_counters[i],
                    PDH_FMT_DOUBLE,
                    &bufferSize,
                    &itemCount,
                    pdhItems);

                // If the returned value is nto PDH_MORE_DATA, the function
                // has failed.
                if (PDH_MORE_DATA != pdhStatus)
                {
                    throw CException(GetErrorString(pdhStatus, m_processMetrics[i]));
                }

                std::vector<unsigned char> buffer(bufferSize);
                pdhItems = (PDH_FMT_COUNTERVALUE_ITEM*)(&buffer[0]);

                pdhStatus = PdhGetFormattedCounterArray(
                    m_counters[i],
                    PDH_FMT_DOUBLE,
                    &bufferSize,
                    &itemCount,
                    pdhItems);

                if (ERROR_SUCCESS != pdhStatus)
                {
                    continue;
                }

                tstring name = m_processMetrics[i];
                // Everything is good, mine the data.
                for (DWORD i = 0; i < itemCount; i++) 
                {
                    //name += tstring(pdhItems[i].szName);

                    // Add no after duplicate processes eg chrome, chrome#2
                    map<std::tstring, double>::iterator it;
                    it = collectedData.find(name);
                    u_int count = 2;
                    string appData = "";
                    wstring tmp;
                    while (it != collectedData.end()) {
                        appData = "#" + to_string(count);
                        count++;
                        tmp = tstring(appData.begin(), appData.end());
                        it = collectedData.find(name + tmp);
                    }

                    collectedData.insert(
                        std::make_pair(
                            name, //std::tstring(pdhItems[i].szName),
                            pdhItems[i].FmtValue.doubleValue)
                    );
                }

                pdhItems = NULL;
                bufferSize = itemCount = 0;
            }

            break;
        }
        return collectedData;
    }

private:
    //! Helper function that translate the PDH error code into
    //! an useful message.
    std::tstring GetErrorString(PDH_STATUS errorCode, tstring searchInstance)
    {
        HANDLE hPdhLibrary = NULL;
        LPTSTR pMessage = NULL;
        DWORD_PTR pArgs[] = { (DWORD_PTR)searchInstance.c_str() };
        std::tstring errorString;

        hPdhLibrary = LoadLibrary(_T("pdh.dll"));
        if (NULL == hPdhLibrary)
        {
            std::tstringstream ss;
            ss
                << _T("Format message failed with ")
                << std::hex << GetLastError();
            return ss.str();
        }

        if (!FormatMessage(FORMAT_MESSAGE_FROM_HMODULE |
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            /*FORMAT_MESSAGE_IGNORE_INSERTS |*/
            FORMAT_MESSAGE_ARGUMENT_ARRAY,
            hPdhLibrary,
            errorCode,
            0,
            (LPTSTR)&pMessage,
            0,
            (va_list*)pArgs))
        {
            std::tstringstream ss;
            ss
                << searchInstance
                << _T(" ")
                << _T("Format message failed with ")
                << std::hex
                << GetLastError()
                << std::endl;
            errorString = ss.str();
        }
        else
        {
            errorString += searchInstance;
            errorString += _T(" ");
            errorString += pMessage;
            LocalFree(pMessage);
        }

        return errorString;
    }
};

#endif __CPdhQuery_h__