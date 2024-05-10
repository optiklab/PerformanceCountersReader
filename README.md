# PerformanceCountersReader
Using Windows PDH API to track performance metrics on certain processes

# Simple example of what we need

Code to measure CPU load in Windows ...

```C
	#include <stdio.h>
	#include <pdh.h>

	double cpuLoad[10];
	double avgCpuLoad = 0;
	int i;

	void DispTime()
	{
		HQUERY hQuery;
		HCOUNTER hCounter;

		PDH_FMT_COUNTERVALUE FmtValue;

		PdhOpenQuery(NULL, 0, &hQuery);

		PdhAddCounter(hQuery, "\\Processor(_Total)\\% Processor Time", 0, &hCounter);

		printf("Starting the process...\n");

		PdhCollectQueryData(hQuery);


		PdhGetFormattedCounterValue(hCounter, PDH_FMT_DOUBLE, NULL, &FmtValue);
		printf("The cpu usage is : %f%%\n", FmtValue.doubleValue);


		PdhCloseQuery(hQuery);
	}

	void main()
	{
		DispTime();
		getchar();
	} 
```

All you need to do is include the pdh.lib in your project.