//! [Request definition]
#include <thread>
#include <condition_variable>
#include <fastcgi++/request.hpp>

class Timer: public Fastcgipp::Request<char>
{
public:
	Timer():
        m_time(0),
        m_startTime(std::chrono::steady_clock::now())
    {}
    //! [Request definition]

    //! [Stopwatch]
    static void startStopwatch()
    {
        s_stopwatch.start();
    }

    static void stopStopwatch()
    {
        s_stopwatch.stop();
    }

private:
    class Stopwatch
    {
    private:
        std::thread m_thread;
        std::mutex m_mutex;
        std::condition_variable m_cv;
        bool m_kill;

        struct Item
        {
            std::function<void(Fastcgipp::Message)> callback;
            mutable Fastcgipp::Message message;
            std::chrono::time_point<std::chrono::steady_clock> wakeup;

            Item(
                    const std::function<void(Fastcgipp::Message)>& callback_,
                    Fastcgipp::Message&& message_,
                    const std::chrono::time_point<std::chrono::steady_clock>&
                        wakeup_):
                callback(callback_),
                message(std::move(message_)),
                wakeup(wakeup_)
            {}

            bool operator<(const Item& item) const
            {
                return wakeup < item.wakeup;
            }

            bool operator==(const Item& item) const
            {
                return wakeup == item.wakeup;
            }
        };
        std::set<Item> m_queue;

        void handler()
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            while(!m_kill)
            {
                if(m_queue.empty())
                    m_cv.wait(lock);
                else
                {
                    const Item& item = *m_queue.begin();

                    if(item.wakeup <= std::chrono::steady_clock::now())
                    {
                        item.callback(std::move(item.message));
                        m_queue.erase(m_queue.begin());
                    }
                    else
                        m_cv.wait_until(lock, item.wakeup);
                }
            }
        }

    public:
        void push(
                const std::function<void(Fastcgipp::Message)>& callback,
                Fastcgipp::Message&& message,
                std::chrono::time_point<std::chrono::steady_clock> wakeup)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queue.emplace_hint(
                    m_queue.end(),
                    callback,
                    std::move(message),
                    wakeup);
            m_cv.notify_one();
        }

        void start()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if(!m_thread.joinable())
            {
                m_kill = false;
                std::thread thread(std::bind(&Stopwatch::handler, this));
                m_thread.swap(thread);
            }
        }

        void stop()
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if(m_thread.joinable())
            {
                m_kill = true;
                m_cv.notify_one();
                lock.unlock();
                m_thread.join();
            }
        }
    };

    static Stopwatch s_stopwatch;
    //! [Stopwatch]

    //! [Variables]
    unsigned m_time;

    const std::chrono::time_point<std::chrono::steady_clock> m_startTime;
    //! [Variables]

    //! [Response]
	bool response()
	{
        if(m_time < 5)
        {
            if(m_time == 0)
                out <<
"Content-Type: text/html; charset=iso-8859-1\r\n\r\n"
"<!DOCTYPE html>\n"
"<html lang='en'>"
    "<head>"
        "<meta charset='iso-8859-1' />"
        "<title>fastcgi++: Timer</title>"
    "</head>"
    "<body>"
        "<p>";
            out << m_time++ << "...";
            out.flush();

            Fastcgipp::Message message;
            message.type = 1;

            static const char messageText[] = "I was passed between threads!!";
            message.data.assign(messageText, sizeof(messageText)-1);

            s_stopwatch.push(
                    callback(),
                    std::move(message),
                    m_startTime + std::chrono::seconds(m_time));

            return false;
        }
        else
        {
            out << "5</p>"
    "</body>"
"</html>";
            return true;
        }
	}
};

Timer::Stopwatch Timer::s_stopwatch;

#include <fastcgi++/manager.hpp>

int main()
{
    Timer::startStopwatch();
    //! [Response]

    //! [Finish]
    Fastcgipp::Manager<Timer> manager(
            std::max(1u, unsigned(std::thread::hardware_concurrency()/2)));
    manager.setupSignals();
    manager.listen();
    manager.start();
    manager.join();

    Timer::stopStopwatch();

    return 0;
}
//! [Finish]
