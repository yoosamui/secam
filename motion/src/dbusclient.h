#include <giomm.h>
//#include <giomm/asyncresult.h>
//#include <giomm/dbusproxy.h>
//#include <giomm/initable.h>
//#include <glibmm/main.h>

using namespace std;

class Dbusclient
{
  private:
    static string m_date;
    static string m_ticket;

    static bool on_main_loop_idle();
    static void on_dbus_proxy_GetDate(Glib::RefPtr<Gio::AsyncResult>& result);
    static void on_dbus_proxy_start_stream(Glib::RefPtr<Gio::AsyncResult>& result);
    static void on_dbus_proxy_stop_stream(Glib::RefPtr<Gio::AsyncResult>& result);

    // static void on_dbus_proxy_pushover(Glib::RefPtr<Gio::AsyncResult>& result);
    // static void on_dbus_proxy_camtimestamp(Glib::RefPtr<Gio::AsyncResult>& result);

  public:
    const string& getDate();
    const string& start_stream();

    void stop_stream(const string& ticket);

    bool server_alive();
};
