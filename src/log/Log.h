#pragma once
#include <log/Record.h>
#include <log/Logger.h>
#include <log/Init.h>

//////////////////////////////////////////////////////////////////////////
// Helper macros that get context info

#if _MSC_VER >= 1600 && !defined(__INTELLISENSE__) // >= Visual Studio 2010 and skip IntelliSense
#   define PLOG_GET_THIS()      __if_exists(this) { this } __if_not_exists(this) { 0 }
#else
#   define PLOG_GET_THIS()      0
#endif

#ifdef _MSC_VER
#   define PLOG_GET_FUNC()      __FUNCTION__
#elif defined(__BORLANDC__)
#   define PLOG_GET_FUNC()      __FUNC__
#else
#   define PLOG_GET_FUNC()      __PRETTY_FUNCTION__
#endif

#if PLOG_CAPTURE_FILE
#   define PLOG_GET_FILE()      __FILE__
#else
#   define PLOG_GET_FILE()      ""
#endif

//////////////////////////////////////////////////////////////////////////
// Log severity level checker

#define IF_LOG_(instance, severity)     !(kvdb::get<instance>() && kvdb::get<instance>()->checkSeverity(severity)) ? (void)0 :
#define IF_LOG(severity)                IF_LOG_(LOG_DEFAULT_INSTANCE, severity)

//////////////////////////////////////////////////////////////////////////
// Main logging macros

#define LOG_(instance, severity)        IF_LOG_(instance, severity) (*kvdb::get<instance>()) += kvdb::Record(severity, PLOG_GET_FUNC(), __LINE__, PLOG_GET_FILE(), PLOG_GET_THIS())
#define LOG(severity)                   LOG_(LOG_DEFAULT_INSTANCE, severity)

#define LOG_VERBOSE                     LOG(kvdb::verbose)
#define LOG_DEBUG                       LOG(kvdb::debug)
#define LOG_INFO                        LOG(kvdb::info)
#define LOG_WARNING                     LOG(kvdb::warning)
#define LOG_ERROR                       LOG(kvdb::error)
#define LOG_FATAL                       LOG(kvdb::fatal)

#define LOG_VERBOSE_(instance)          LOG_(instance, kvdb::verbose)
#define LOG_DEBUG_(instance)            LOG_(instance, kvdb::debug)
#define LOG_INFO_(instance)             LOG_(instance, kvdb::info)
#define LOG_WARNING_(instance)          LOG_(instance, kvdb::warning)
#define LOG_ERROR_(instance)            LOG_(instance, kvdb::error)
#define LOG_FATAL_(instance)            LOG_(instance, kvdb::fatal)

#define LOGV                            LOG_VERBOSE
#define LOGD                            LOG_DEBUG
#define LOGI                            LOG_INFO
#define LOGW                            LOG_WARNING
#define LOGE                            LOG_ERROR
#define LOGF                            LOG_FATAL

#define LOGV_(instance)                 LOG_VERBOSE_(instance)
#define LOGD_(instance)                 LOG_DEBUG_(instance)
#define LOGI_(instance)                 LOG_INFO_(instance)
#define LOGW_(instance)                 LOG_WARNING_(instance)
#define LOGE_(instance)                 LOG_ERROR_(instance)
#define LOGF_(instance)                 LOG_FATAL_(instance)

//////////////////////////////////////////////////////////////////////////
// Conditional logging macros

#define LOG_IF_(instance, severity, condition)  !(condition) ? void(0) : LOG_(instance, severity)
#define LOG_IF(severity, condition)             LOG_IF_(LOG_DEFAULT_INSTANCE, severity, condition)

#define LOG_VERBOSE_IF(condition)               LOG_IF(kvdb::verbose, condition)
#define LOG_DEBUG_IF(condition)                 LOG_IF(kvdb::debug, condition)
#define LOG_INFO_IF(condition)                  LOG_IF(kvdb::info, condition)
#define LOG_WARNING_IF(condition)               LOG_IF(kvdb::warning, condition)
#define LOG_ERROR_IF(condition)                 LOG_IF(kvdb::error, condition)
#define LOG_FATAL_IF(condition)                 LOG_IF(kvdb::fatal, condition)

#define LOG_VERBOSE_IF_(instance, condition)    LOG_IF_(instance, kvdb::verbose, condition)
#define LOG_DEBUG_IF_(instance, condition)      LOG_IF_(instance, kvdb::debug, condition)
#define LOG_INFO_IF_(instance, condition)       LOG_IF_(instance, kvdb::info, condition)
#define LOG_WARNING_IF_(instance, condition)    LOG_IF_(instance, kvdb::warning, condition)
#define LOG_ERROR_IF_(instance, condition)      LOG_IF_(instance, kvdb::error, condition)
#define LOG_FATAL_IF_(instance, condition)      LOG_IF_(instance, kvdb::fatal, condition)

#define LOGV_IF(condition)                      LOG_VERBOSE_IF(condition)
#define LOGD_IF(condition)                      LOG_DEBUG_IF(condition)
#define LOGI_IF(condition)                      LOG_INFO_IF(condition)
#define LOGW_IF(condition)                      LOG_WARNING_IF(condition)
#define LOGE_IF(condition)                      LOG_ERROR_IF(condition)
#define LOGF_IF(condition)                      LOG_FATAL_IF(condition)

#define LOGV_IF_(instance, condition)           LOG_VERBOSE_IF_(instance, condition)
#define LOGD_IF_(instance, condition)           LOG_DEBUG_IF_(instance, condition)
#define LOGI_IF_(instance, condition)           LOG_INFO_IF_(instance, condition)
#define LOGW_IF_(instance, condition)           LOG_WARNING_IF_(instance, condition)
#define LOGE_IF_(instance, condition)           LOG_ERROR_IF_(instance, condition)
#define LOGF_IF_(instance, condition)           LOG_FATAL_IF_(instance, condition)

