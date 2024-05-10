#pragma once
#ifndef __CExample_h__
#define __CExample_h__

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

class CExample
{
private:
    std::tstring m_counterPath;

public:
    //! Constructor
    explicit CExample(std::tstring const& counterPath)
    {
    }

    //! Destructor.
    ~CExample()
    {
    }

    std::map<std::tstring, double> CollectQueryData()
    {
        std::map<std::tstring, double> collectedData;

        return collectedData;
    }

private:
};

#endif __CExample_h__