#include <fastcgi++/mailer.hpp>
#include <fastcgi++/request.hpp>

class EmailSender: public Fastcgipp::Request<wchar_t>
{
    static Fastcgipp::Mail::Mailer s_mailer;

    inline bool send();

    virtual bool response()
    {
        out <<
L"Content-Type: text/html; charset=utf-8\r\n\r\n"
L"<!DOCTYPE html>\n"
L"<html>"
    L"<head>"
        L"<meta charset='utf-8' />"
        L"<title>fastcgi++: Email Sender</title>"
    L"</head>"
    L"<body>";

        if(environment().requestMethod == Fastcgipp::Http::RequestMethod::POST)
        {
            if(send())
                out << L"<h2>Email Queued!</h2>";
            else
                out << L"<h2>Unable to send email</h2>";
        }
        else
        {
            out <<
        L"<h2>Compose Email</h2>"
        L"<form method='post' enctype='application/x-www-form-urlencoded' "
            L"accept-charset='utf-8'>"
            L"To: <input type='text' name='to' /><br />"
            L"From: <input type='text' name='from' /><br />"
            L"Subject: <input type='text' name='subject' /><br />"
            L"<textarea name='message' wrap='soft' cols='50' rows='20'>"
                L"Put you message here with whatever dangerous characters you want!\r\n\r\nUse a double space for new paragraphs."
            L"</textarea><br />"
            L"<input type='submit' name='Send' value='Send' />"
        L"</form>";
        }

        out <<
    L"</body>"
L"</html>";

        return true;
    }

    static void removeLines(std::wstring& string);

public:
    EmailSender():
        Fastcgipp::Request<wchar_t>(5*1024)
    {}

    static void start()
    {
        s_mailer.init(
                "localhost",
                "isatec.ca");
        s_mailer.start();
    }

    static void terminate()
    {
        s_mailer.terminate();
        s_mailer.join();
    }
};

void EmailSender::removeLines(std::wstring& string)
{
    std::wstring::size_type n = 0;
    while(true)
    {
        n = string.find(wchar_t('\n'), n);
        if(n == std::wstring::npos)
            break;
        string[n] = wchar_t(' ');
    }

    n = 0;
    while(true)
    {
        n = string.find(wchar_t('\r'), n);
        if(n == std::wstring::npos)
            break;
        string[n] = wchar_t(' ');
    }
}

#include <fastcgi++/email.hpp>
#include <fastcgi++/log.hpp>
#include <iomanip>
bool EmailSender::send()
{
    auto to_it = environment().posts.find(L"to");
    if(to_it == environment().posts.end())
    {
        ERROR_LOG("No \"to\" field received from form.")
        return false;
    }
    removeLines(to_it->second);
    const auto& to = to_it->second;

    auto from_it = environment().posts.find(L"from");
    if(from_it == environment().posts.end())
    {
        ERROR_LOG("No \"from\" field received from form.")
        return false;
    }
    removeLines(from_it->second);
    const auto& from = from_it->second;

    auto subject_it = environment().posts.find(L"subject");
    if(subject_it == environment().posts.end())
    {
        ERROR_LOG("No \"subject\" field received from form.")
        return false;
    }
    removeLines(subject_it->second);
    const auto& subject = subject_it->second;

    auto message_it = environment().posts.find(L"message");
    if(subject_it == environment().posts.end())
    {
        ERROR_LOG("No \"message\" field received from form.")
        return false;
    }
    const auto& message = message_it->second;

    auto now = std::time(nullptr);
    
    Fastcgipp::Mail::Email<wchar_t> email;
    email.to(to);
    email.from(from);

    using Fastcgipp::Encoding;
    email <<
L"From: " << from << L"\n"
L"To: " << to << L"\n"
L"Subject: " << subject << L"\n"
L"Date: " << std::put_time(
        std::gmtime(&now),
        L"%a, %d %b %Y %H:%M:%S GMT") << L"\n"
L"Content-Type: text/html; charset=utf-8;\r\n\r\n"
L"<!DOCTYPE html>\n"
L"<html>"
    L"<head>"
        L"<title>" << Encoding::HTML << subject << Encoding::NONE << "</title>"
    L"</head>"
    L"<body>";

    std::wstring::size_type start=0;
    while(true)
    {
        email << L"<p>" << Encoding::HTML;
        auto end = message.find(L"\r\n\r\n", start);
        if(end == std::wstring::npos)
        {
            email << message.substr(start) << Encoding::NONE << L"</p>";
            break;
        }
        else
            email << message.substr(start, end-start)
                  << Encoding::NONE << L"</p>";

        start = end+4;
    }

    email <<
    L"</body>"
L"</html>";

    s_mailer.queue(email);
    return true;
}

Fastcgipp::Mail::Mailer EmailSender::s_mailer;

#include <fastcgi++/manager.hpp>

int main()
{
    Fastcgipp::Manager<EmailSender> manager;
    manager.setupSignals();
    manager.listen();
    EmailSender::start();
    manager.start();
    manager.join();
    EmailSender::terminate();

    return 0;
}
