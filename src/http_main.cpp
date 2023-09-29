#include <http_server.h>

int main()
{
    HttpServer *server = HttpServer::GetInstance();
    server->Init("127.0.0.1", 1111, 5, 5);
    return 0;
}