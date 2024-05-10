#include "CPdhQuery.h"
#include "CAppSettings.h"
#include "CWinServices.h"
#include <nlohmann/json.hpp>

using namespace std;

#ifdef UNICODE
#define tcout std::wcout
#else
#define tcout std::cout
#endif

void DumpMap(std::map<std::tstring, double> const& m)
{
    std::map<std::tstring, double>::const_iterator itr = m.begin();
    while (m.end() != itr)
    {
        if (itr->second > 0)
        {
            tcout << itr->first << " " << itr->second << std::endl;
        }
        ++itr;
    }
    tcout << " ################################################ " << std::endl;
}

vector<tstring> GetMetricsList(tstring & metricsLine)
{
    vector<std::tstring> metrics;
    tstring buf;
    for (int i = 0; i < metricsLine.size(); i++)
    {
        if (metricsLine[i] == ';')
        {
            metrics.push_back(buf);
            buf.clear();
        }
        else
        {
            buf += metricsLine[i];
        }
    }

    if (!buf.empty())
        metrics.push_back(buf);

    return metrics;
}

//g++ -std=c++17 "main.cpp" -o main
//g++ -std=c++14 "main.cpp" -o main

int main(int argc, char** argv)
{
    std::string configFilename = "../appsettings.json";
    CAppSettings settings(configFilename);
    
    CWinServices winServices;
    DWORD parentProcessPID = winServices.GetServicePID(settings.Get()->serviceName);

    tcout << settings.Get()->serviceName << " " << parentProcessPID << std::endl;
    tcout << " ################################################ " << std::endl;

    try
    {
        // https://learn.microsoft.com/en-us/windows/win32/perfctrs/specifying-a-counter-path


        CPdhQuery pdhQuery(
            // https://learn.microsoft.com/en-us/previous-versions/windows/it-pro/windows-server-2003/cc780836(v=ws.10)
            //std::tstring(_T("\\Process(firefox*)\\ID Process"))
            GetMetricsList(settings.Get()->processMetrics),

            // https://learn.microsoft.com/en-us/dotnet/framework/debug-trace-profile/performance-counters
            //std::tstring(_T("\\Process(*)\\.NET CLR Memory\\Process ID"))
            //std::tstring(_T("\\Process(*)\\.NET CLR Memory\\# of current physical Threads"))
            //std::tstring(_T("\\Process(*)\\.NET CLR Memory\\# of current logical Threads"))
            
            
            //std::tstring(_T("\\Process(*)\\% Processor Time")) // 
            //std::tstring(L"\\Processor Information(_Total)\\% Processor Time")
            //std::tstring(_T("\\Thread(*)\\Context Switches/sec"))
            //std::tstring(_T("\\Thread(firefox/0)\\Context Switches/sec"))
            //tstring(L"\\Processor(*)\\% Processor Time")
            //tstring(_T("\\Processor(*)\\Interrupts/sec"))
            //tstring(L"\\Processor(_Total)\\Interrupts/sec")
            parentProcessPID,
            settings.Get()->processName
        );
        for (int i = 0; i < 100; ++i)
        {
            Sleep(1000);
            DumpMap(pdhQuery.CollectQueryData());
        }
    }
    catch (CPdhQuery::CException const& e)
    {
        tcout << e.What() << std::endl;
    }

    return 0;
}