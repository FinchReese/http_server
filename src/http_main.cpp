#include <http_server.h>

int main()
{
    HttpServer server("127.0.0.1", 1111, 5, 5);
    server.Init();
    return 0;
}