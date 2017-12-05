/*!
 * @file       manager.cpp
 * @brief      Defines the Manager class
 * @author     Eddie Carle &lt;eddie@isatec.ca&gt;
 * @date       May 4, 2017
 * @copyright  Copyright &copy; 2017 Eddie Carle. This project is released under
 *             the GNU Lesser General Public License Version 3.
 */

/*******************************************************************************
* Copyright (C) 2017 Eddie Carle [eddie@isatec.ca]                             *
*                                                                              *
* This file is part of fastcgi++.                                              *
*                                                                              *
* fastcgi++ is free software: you can redistribute it and/or modify it under   *
* the terms of the GNU Lesser General Public License as  published by the Free *
* Software Foundation, either version 3 of the License, or (at your option)    *
* any later version.                                                           *
*                                                                              *
* fastcgi++ is distributed in the hope that it will be useful, but WITHOUT ANY *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS    *
* FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for     *
* more details.                                                                *
*                                                                              *
* You should have received a copy of the GNU Lesser General Public License     *
* along with fastcgi++.  If not, see <http://www.gnu.org/licenses/>.           *
*******************************************************************************/

#include "fastcgi++/log.hpp"
#include "fastcgi++/manager.hpp"

Fastcgipp::Manager_base* Fastcgipp::Manager_base::instance=nullptr;

Fastcgipp::Manager_base::Manager_base(unsigned threads):
    m_transceiver(std::bind(
                &Fastcgipp::Manager_base::push,
                this,
                std::placeholders::_1,
                std::placeholders::_2)),
    m_terminate(true),
    m_stop(true),
    m_threads(threads)
#if FASTCGIPP_LOG_LEVEL > 3
    ,m_requestCount(0),
    m_maxRequests(0),
    m_managementRecordCount(0),
    m_badSocketMessageCount(0),
    m_badSocketKillCount(0),
    m_messageCount(0),
    m_activeThreads(threads),
    m_maxActiveThreads(0)
#endif
{
    if(instance != nullptr)
        FAIL_LOG("You're not allowed to have multiple manager instances")
    instance = this;
    DIAG_LOG("Manager_base::Manager_base(): Initialized")
}

void Fastcgipp::Manager_base::terminate()
{
    std::lock_guard<std::mutex> lock(m_tasksMutex);
    m_terminate=true;
    m_transceiver.terminate();
    m_wake.notify_all();
}

void Fastcgipp::Manager_base::stop()
{
    std::lock_guard<std::mutex> lock(m_tasksMutex);
    m_stop=true;
    m_transceiver.stop();
    m_wake.notify_all();
}

void Fastcgipp::Manager_base::start()
{
    std::lock_guard<std::mutex> lock(m_tasksMutex);
    DIAG_LOG("Starting fastcgi++ manager")
    m_stop=false;
    m_terminate=false;
    m_transceiver.start();
    for(auto& thread: m_threads)
        if(!thread.joinable())
        {
            std::thread newThread(&Fastcgipp::Manager_base::handler, this);
            thread.swap(newThread);
        }
}

void Fastcgipp::Manager_base::join()
{
    for(auto& thread: m_threads)
        if(thread.joinable())
            thread.join();
    m_transceiver.join();
}

#include <signal.h>
void Fastcgipp::Manager_base::setupSignals()
{
    struct sigaction sigAction;
    sigAction.sa_handler=Fastcgipp::Manager_base::signalHandler;
    sigemptyset(&sigAction.sa_mask);
    sigAction.sa_flags=0;

    sigaction(SIGPIPE, &sigAction, NULL);
    sigaction(SIGUSR1, &sigAction, NULL);
    sigaction(SIGTERM, &sigAction, NULL);
}

void Fastcgipp::Manager_base::signalHandler(int signum)
{
    switch(signum)
    {
        case SIGUSR1:
        {
            if(instance)
            {
                DIAG_LOG("Received SIGUSR1. Stopping fastcgi++ manager.")
                instance->stop();
            }
            else
                WARNING_LOG("Received SIGUSR1 but fastcgi++ manager isn't "\
                        "running")
            break;
        }
        case SIGTERM:
        {
            if(instance)
            {
                DIAG_LOG("Received SIGTERM. Terminating fastcgi++ manager.")
                instance->terminate();
            }
            else
                WARNING_LOG("Received SIGTERM but fastcgi++ manager isn't "\
                        "running")
            break;
        }
    }
}

void Fastcgipp::Manager_base::localHandler()
{
    Message message;
    Socket socket;
    {
        std::lock_guard<std::mutex> lock(m_messagesMutex);
        message = std::move(m_messages.front().first);
        socket = m_messages.front().second;
        m_messages.pop();
    }

    if(message.type == 0)
    {
        const Protocol::Header& header
            = *reinterpret_cast<Protocol::Header*>(message.data.begin());
        switch(header.type)
        {
            case Protocol::RecordType::GET_VALUES:
            {
                const char* name;
                const char* value;
                const char* end;

                while(Protocol::processParamHeader(
                        message.data.begin()+sizeof(header),
                        message.data.end(),
                        name,
                        value,
                        end))
                {
                    switch(value-name)
                    {
                        case 14:
                        {
                            if(std::equal(name, value, "FCGI_MAX_CONNS"))
                            {
                                Block record(
                                        reinterpret_cast<const char*>(
                                            &Protocol::maxConnsReply),
                                        sizeof(Protocol::maxConnsReply));
                                m_transceiver.send(
                                        socket,
                                        std::move(record),
                                        false);
                            }
                            break;
                        }
                        case 13:
                        {
                            if(std::equal(name, value, "FCGI_MAX_REQS"))
                            {
                                Block record(
                                        reinterpret_cast<const char*>(
                                            &Protocol::maxReqsReply),
                                        sizeof(Protocol::maxReqsReply));
                                m_transceiver.send(
                                        socket,
                                        std::move(record),
                                        false);
                            }
                            break;
                        }
                        case 15:
                        {
                            if(std::equal(name, value, "FCGI_MPXS_CONNS"))
                            {
                                Block record(
                                        reinterpret_cast<const char*>(
                                            &Protocol::mpxsConnsReply),
                                        sizeof(Protocol::mpxsConnsReply));
                                m_transceiver.send(
                                        socket,
                                        std::move(record),
                                        false);
                            }
                            break;
                        }
                    }
                }
                break;
            }

            default:
            {
                Block record(
                        sizeof(Protocol::Header)
                        +sizeof(Protocol::UnknownType));

                Protocol::Header& sendHeader
                    = *reinterpret_cast<Protocol::Header*>(record.begin());
                sendHeader.version = Protocol::version;
                sendHeader.type = Protocol::RecordType::UNKNOWN_TYPE;
                sendHeader.fcgiId = 0;
                sendHeader.contentLength = sizeof(Protocol::UnknownType);
                sendHeader.paddingLength = 0;

                Protocol::UnknownType& sendBody
                    = *reinterpret_cast<Protocol::UnknownType*>(
                            record.begin()+sizeof(header));
                sendBody.type = header.type;

                m_transceiver.send(socket, std::move(record), false);

                break;
            }
        }
    }
    else
        ERROR_LOG("Got a non-FastCGI record destined for the manager")
}

void Fastcgipp::Manager_base::handler()
{
    std::unique_lock<std::shared_timed_mutex> requestsWriteLock(
            m_requestsMutex,
            std::defer_lock);
    std::unique_lock<std::mutex> tasksLock(m_tasksMutex);
    std::shared_lock<std::shared_timed_mutex> requestsReadLock(m_requestsMutex);

    while(!m_terminate && !(m_stop && m_requests.empty()))
    {
        requestsReadLock.unlock();
        while(!m_tasks.empty())
        {
            auto id = m_tasks.front();
            m_tasks.pop();
            tasksLock.unlock();

            if(id.m_id == 0)
                localHandler();
            else
            {
                requestsReadLock.lock();
                auto request = m_requests.find(id);
                if(request != m_requests.end())
                {
                    std::unique_lock<std::mutex> requestLock(
                            request->second->mutex,
                            std::try_to_lock);
                    requestsReadLock.unlock();

                    if(requestLock)
                    {
                        auto lock = request->second->handler();
                        if(!lock || !id.m_socket.valid())
                        {
#if FASTCGIPP_LOG_LEVEL > 3
                            if(!id.m_socket.valid())
                                ++m_badSocketKillCount;
#endif
                            if(lock)
                                lock.unlock();
                            requestsWriteLock.lock();
                            requestLock.unlock();
                            m_requests.erase(request);
                            requestsWriteLock.unlock();
                        }
                        else
                        {
                            requestLock.unlock();
                            lock.unlock();
                        }
                    }
                }
                else
                    requestsReadLock.unlock();
            }
            tasksLock.lock();
        }

        requestsReadLock.lock();
        if(m_terminate || (m_stop && m_requests.empty()))
            break;
        requestsReadLock.unlock();
#if FASTCGIPP_LOG_LEVEL > 3
        --m_activeThreads;
#endif
        m_wake.wait(tasksLock);
#if FASTCGIPP_LOG_LEVEL > 3
        if(!m_stop && !m_terminate)
        {
            ++m_activeThreads;
            m_maxActiveThreads = std::max(m_activeThreads, m_maxActiveThreads);
        }
#endif
        requestsReadLock.lock();
    }
}

void Fastcgipp::Manager_base::push(Protocol::RequestId id, Message&& message)
{
    if(id.m_id == 0)
    {
#if FASTCGIPP_LOG_LEVEL > 3
        ++m_managementRecordCount;
#endif
        std::lock_guard<std::mutex> lock(m_messagesMutex);
        m_messages.push(std::make_pair(std::move(message), id.m_socket));
    }
    else if(id.m_id == Protocol::badFcgiId)
    {
#if FASTCGIPP_LOG_LEVEL > 3
        ++m_badSocketMessageCount;
#endif
        std::lock_guard<std::shared_timed_mutex> lock(m_requestsMutex);
        const auto range = m_requests.equal_range(id.m_socket);
        auto request = range.first;
        while(request != range.second)
        {
            std::unique_lock<std::mutex> lock(
                    request->second->mutex,
                    std::try_to_lock);
            if(lock)
            {
                lock.unlock();
                request = m_requests.erase(request);
#if FASTCGIPP_LOG_LEVEL > 3
                ++m_badSocketKillCount;
#endif
            }
            else
                ++request;
        }
        return;
    }
    else
    {
#if FASTCGIPP_LOG_LEVEL > 3
        ++m_messageCount;
#endif
        std::unique_lock<std::shared_timed_mutex> lock(m_requestsMutex);
        auto request = m_requests.find(id);
        if(request == m_requests.end())
        {
            if(message.type == 0)
            {
                const Protocol::Header& header=
                    *reinterpret_cast<Protocol::Header*>(message.data.begin());
                if(header.type == Protocol::RecordType::BEGIN_REQUEST)
                {
                    const Protocol::BeginRequest& body
                        = *reinterpret_cast<Protocol::BeginRequest*>(
                                message.data.begin()
                                +sizeof(header));

                    request = m_requests.emplace(
                            std::piecewise_construct,
                            std::forward_as_tuple(id),
                            std::forward_as_tuple()).first;

                    request->second = makeRequest(
                            id,
                            body.role,
                            body.kill());
                    lock.unlock();
#if FASTCGIPP_LOG_LEVEL > 3
                    ++m_requestCount;
                    m_maxRequests = std::max(m_maxRequests, m_requests.size());
#endif
                }
                else
                    WARNING_LOG("Got a non BEGIN_REQUEST record for a request"\
                            " that doesn't exist")
            }
            return;
        }
        else
            request->second->push(std::move(message));
    }
    std::lock_guard<std::mutex> lock(m_tasksMutex);
    m_tasks.push(id);
    m_wake.notify_one();
}

Fastcgipp::Manager_base::~Manager_base()
{
    instance=nullptr;
    terminate();
    DIAG_LOG("Manager_base::~Manager_base(): New requests ============== " \
            << m_requestCount)
    DIAG_LOG("Manager_base::~Manager_base(): Max concurrent requests === " \
            << m_maxRequests)
    DIAG_LOG("Manager_base::~Manager_base(): Management records ======== " \
            << m_managementRecordCount)
    DIAG_LOG("Manager_base::~Manager_base(): Bad socket messages ======= " \
            << m_badSocketMessageCount)
    DIAG_LOG("Manager_base::~Manager_base(): Bad socket request kills == " \
            << m_badSocketKillCount)
    DIAG_LOG("Manager_base::~Manager_base(): Request messages received = " \
            << m_messageCount)
    DIAG_LOG("Manager_base::~Manager_base(): Maximum active threads ==== " \
            << m_maxActiveThreads)
    DIAG_LOG("Manager_base::~Manager_base(): Remaining requests ======== " \
            << m_requests.size())
    DIAG_LOG("Manager_base::~Manager_base(): Remaining tasks =========== " \
            << m_tasks.size())
    DIAG_LOG("Manager_base::~Manager_base(): Remaining local messages == " \
            << m_messages.size())
}
