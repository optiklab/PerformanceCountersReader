#pragma once

#ifndef __CAppSettings_h__
#define __CAppSettings_h__

#include "Common.h"
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Configuration structure
class CAppConfig
{
public:
	std::tstring serviceName;
	std::tstring processName;
	std::tstring processMetrics;
	std::tstring metrics;
};

class CAppSettings
{
private:
	bool m_inited = false;
    std::string m_filePath;
	CAppConfig* m_config;

public:
    //! Constructor
    explicit CAppSettings(std::string const& filePath) : m_filePath(filePath)
    {
    }

    //! Destructor.
    ~CAppSettings()
    {
		if (m_config != NULL)
			delete m_config;
    }

	CAppConfig* Get()
	{
		if (m_config == NULL)
			m_config = LoadConfiguration();

		return m_config;
	}

private:
	// Load configuration from a JSON file
	CAppConfig* LoadConfiguration()
	{
		std::ifstream file(m_filePath);
		if (!file.is_open()) {
			throw std::runtime_error("Failed to open the configuration file.");
		}

		json jsonData;
		file >> jsonData;
		file.close();

		if (m_config == NULL)
			m_config = new CAppConfig();

		auto processName = jsonData["processName"].get<std::string>();
		std::wstring wprocessName(processName.begin(), processName.end());
		m_config->processName = wprocessName;

		auto serviceName = jsonData["serviceName"].get<std::string>();
		std::wstring wserviceName(serviceName.begin(), serviceName.end());
		m_config->serviceName = wserviceName;

		auto processMetrics = jsonData["processMetrics"].get<std::string>();
		std::wstring wprocessMetrics(processMetrics.begin(), processMetrics.end());
		m_config->processMetrics = wprocessMetrics;

		return m_config;
	}
};


#endif __CAppSettings_h__