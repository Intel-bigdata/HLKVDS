#pragma once
#include <iostream>
#include <log/Util.h>

namespace kvdb
{
    template<class Formatter>
    class ConsoleAppender : public IAppender
    {
    public:
        ConsoleAppender()
        {

        }

        virtual void write(const Record& record)
        {
            util::nstring str = Formatter::format(record);
            util::MutexLock lock(m_mutex);

            writestr(str);
        }

    protected:
        void writestr(const util::nstring& str)
        {

            std::cout << str << std::flush;
        }

    protected:
        util::Mutex m_mutex;
    };
}

