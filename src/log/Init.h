#pragma once
#include <string>
#include <log/Logger.h>
#include <log/TxtFormatter.h>
#include <log/RollingFileAppender.h>

namespace kvdb
{
	template<int instance>
	inline Logger<instance>& init(Severity maxSeverity = none, IAppender* appender =NULL)
		{
			static Logger<instance> logger(maxSeverity);
			return appender ? logger.addAppender(appender):logger;
		}

	inline Logger<LOG_DEFAULT_INSTANCE>& init(Severity maxSeverity = none, IAppender* appender = NULL)
    {
        return init<LOG_DEFAULT_INSTANCE>(maxSeverity, appender);
    }

	
    // RollingFileAppender with any Formatter

    template<class Formatter, int instance>
    inline Logger<instance>& init(Severity maxSeverity, const util::nchar* fileName, size_t maxFileSize = 0, int maxFiles = 0)
    {
        static RollingFileAppender<Formatter> rollingFileAppender(fileName, maxFileSize, maxFiles);
        return init<instance>(maxSeverity, &rollingFileAppender);
    }

    template<class Formatter>
    inline Logger<LOG_DEFAULT_INSTANCE>& init(Severity maxSeverity, const util::nchar* fileName, size_t maxFileSize = 0, int maxFiles = 0)
    {
        return init<Formatter, LOG_DEFAULT_INSTANCE>(maxSeverity, fileName, maxFileSize, maxFiles);
    }

	// RollingFileAppender with TXT/CSV chosen by file extension

    template<int instance>
    inline Logger<instance>& init(Severity maxSeverity, const util::nchar* fileName, size_t maxFileSize = 0, int maxFiles = 0)
    {
        return init<TxtFormatter, instance>(maxSeverity, fileName, maxFileSize, maxFiles);
    }

    inline Logger<LOG_DEFAULT_INSTANCE>& init(Severity maxSeverity, const util::nchar* fileName, size_t maxFileSize = 0, int maxFiles = 0)
    {
        return init<LOG_DEFAULT_INSTANCE>(maxSeverity, fileName, maxFileSize, maxFiles);
    }

   
	
}

