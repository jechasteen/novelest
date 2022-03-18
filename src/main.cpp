#include <gtkmm/application.h>

#include "window.hpp"

int main(int argc, char* argv[])
{
    auto app = Gtk::Application::create(argc, argv, "org.novelest.Novelest");
    Novelest window(app);
    return app->run(window);
}