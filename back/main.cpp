#include "./httplib.h"
int main() {
  httplib::Server svr;

  svr.Get("/api/hello", [](const httplib::Request& req, httplib::Response& res) {
    res.set_content("Hello from C++ backend!", "text/plain");
  });

  svr.listen("localhost", 8081);
}
