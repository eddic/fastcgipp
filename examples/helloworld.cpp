//! [Request definition]
#include <fastcgi++/request.hpp>

class HelloWorld: public Fastcgipp::Request<wchar_t>
{
    //! [Request definition]
    //! [Response definition]
    bool response()
    {
        //! [Response definition]
        //! [HTTP header]
        out << L"Content-Type: text/html; charset=utf-8\r\n\r\n";
        //! [HTTP header]

        //! [Output]
        out <<
L"<!DOCTYPE html>\n"
L"<html>"
    L"<head>"
        L"<meta charset='utf-8' />"
        L"<title>fastcgi++: Hello World</title>"
    L"</head>"
    L"<body>"
        L"<p>"
            L"English: Hello World<br>"
            L"Russian: Привет мир<br>"
            L"Greek: Γεια σας κόσμο<br>"
            L"Chinese: 世界您好<br>"
            L"Japanese: 今日は世界<br>"
            L"Runic English?: ᚺᛖᛚᛟ ᚹᛟᛉᛚᛞ<br>"
        L"</p>"
    L"</body>"
L"</html>";
        //! [Output]

        //! [Return]
        return true;
    }
};
//! [Return]

//! [Manager]
#include <fastcgi++/manager.hpp>

int main()
{
    Fastcgipp::Manager<HelloWorld> manager;
    //! [Manager]
    //! [Signals]
    manager.setupSignals();
    //! [Signals]
    //! [Listen]
    //manager.listen("127.0.0.1", "9000"); // if you are using mac os try this instead, because was the only way i can make it work
    manager.listen();


    //! [Listen]
    //! [Start]
    manager.start();
    //! [Start]
    //! [Join]
    manager.join();

    return 0;
}
//! [Join]
