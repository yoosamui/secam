#include <giomm.h>
#include <glibmm/variant.h>

#include "ofMain.h"
#include "ofxCv.h"
#include "ofxOpenCv.h"
#include "streamer.hpp"

using namespace ofxCv;
using namespace cv;

namespace
{
    Streamer m_writer;
    Glib::ustring m_uri;
    Glib::ustring m_source_dir;
    Glib::ustring m_destination_dir;

    std::map<Glib::ustring, shared_ptr<Streamer>> threads;
    guint registered_id = 0;

    // Stores the current alarm.
    static Glib::DateTime curr_alarm;

    static Glib::RefPtr<Gio::DBus::NodeInfo> introspection_data;
    static Glib::ustring introspection_xml =
        "<node name='/org/yoo/secam/server'>"
        "  <interface name='org.yoo.secam.server'>"
        "    <method name='start'>"
        "      <arg name='uri' type='s' direction='in'/>"
        "      <arg name='source' type='s' direction='in'/>"
        "      <arg name='destination' type='s' direction='in'/>"
        "      <arg name='title' type='s' direction='in'/>"
        "      <arg name='ticket' type='s' direction='out'/>"
        "    </method>"
        "    <method name='stop'>"
        "      <arg name='ticket' type='s' direction='in'/>"
        "    </method>"
        "    <method name='GetTime'>"
        "      <arg type='s' name='iso8601' direction='out'/>"
        "    </method>"
        "  </interface>"
        "</node>";

    void on_completed(Glib::ustring& ticket)
    {
        if (threads.count(ticket) == 0) return;

        auto streamer = threads[ticket];
        streamer->stop();

        ofRemoveListener(streamer->on_completed, &on_completed);

        cout << "stop thread: " << ticket << " size: " << threads.size() << endl;
    }

    static void on_method_call(const Glib::RefPtr<Gio::DBus::Connection>& /* connection */,
                               const Glib::ustring& /* sender */,
                               const Glib::ustring& /* object_path */,
                               const Glib::ustring& /* interface_name */,
                               const Glib::ustring& method_name,
                               const Glib::VariantContainerBase& parameters,
                               const Glib::RefPtr<Gio::DBus::MethodInvocation>& invocation)
    {
        std::cout << "request: " << method_name << std::endl;

        if (method_name == "start") {
            auto streamer = std::make_shared<Streamer>();
            ofAddListener(streamer->on_completed, &on_completed);

            Glib::Variant<Glib::ustring> param;
            parameters.get_child(param, 0);
            streamer->m_uri = param.get();

            parameters.get_child(param, 1);
            streamer->m_temp_dir = param.get();

            parameters.get_child(param, 2);
            streamer->m_storage = param.get();

            parameters.get_child(param, 3);
            streamer->m_title = param.get();

            const Glib::ustring ticket = to_string(getTickCount());
            const auto ticket_var = Glib::Variant<Glib::ustring>::create(ticket);

            threads[ticket] = streamer;

            streamer->m_ticket = ticket;
            streamer->start();

            Glib::VariantContainerBase response =
                Glib::VariantContainerBase::create_tuple(ticket_var);

            invocation->return_value(response);

        } else if (method_name == "stop") {
            Glib::Variant<Glib::ustring> param;
            parameters.get_child(param, 0);
            auto ticket = param.get();

            if (threads.count(ticket) != 0) {
                on_completed(ticket);

                // delete item & free streamer
                threads[ticket] = nullptr;
                threads.erase(ticket);

                cout << "remove thread: " << ticket << " size: " << threads.size() << endl;
            }

        } else if (method_name == "GetTime") {
            Glib::DateTime curr_time = Glib::DateTime::create_now_local();

            const Glib::ustring time_str = curr_time.format_iso8601();
            const auto time_var = Glib::Variant<Glib::ustring>::create(time_str);

            // Create the tuple.
            Glib::VariantContainerBase response =
                Glib::VariantContainerBase::create_tuple(time_var);

            // Return the tuple with the included time.
            invocation->return_value(response);
            ofLog() << "End Method GetTime" << endl;

        } else {
            // Non-existent method on the interface.
            Gio::DBus::Error error(Gio::DBus::Error::UNKNOWN_METHOD, "Method does not exist.");
            invocation->return_error(error);
        }
    }

    const Gio::DBus::InterfaceVTable interface_vtable(sigc::ptr_fun(&on_method_call));
    void on_bus_acquired(const Glib::RefPtr<Gio::DBus::Connection>& connection,
                         const Glib::ustring& /* name */)
    {
        try {
            // clang-format off
            registered_id = connection->register_object(
                "/org/yoo/secam/server",
                introspection_data->lookup_interface(),
                interface_vtable);
            // clang-format on

        } catch (const Glib::Error& ex) {
            std::cerr << "Registration of object failed." << std::endl;
        }

        std::cout << "ready." << endl;
        return;
    }

    void on_name_acquired(const Glib::RefPtr<Gio::DBus::Connection>& /* connection */,
                          const Glib::ustring& /* name */)
    {
    }

    void on_name_lost(const Glib::RefPtr<Gio::DBus::Connection>& connection,
                      const Glib::ustring& /* name */)
    {
        connection->unregister_object(registered_id);
    }

}  // namespace
int main()
{
    ofLogToFile("data/logs/server.log", false);

    std::locale::global(std::locale(""));
    Gio::init();

    try {
        introspection_data = Gio::DBus::NodeInfo::create_for_xml(introspection_xml);
    } catch (const Glib::Error& ex) {
        std::cerr << "Unable to create introspection data: " << ex.what() << "." << std::endl;
        return 1;
    }

    // clang-format off
    const guint id = Gio::DBus::own_name(
        Gio::DBus::BUS_TYPE_SESSION,"org.yoo.secam.server",
        sigc::ptr_fun(&on_bus_acquired),
        sigc::ptr_fun(&on_name_acquired),
        sigc::ptr_fun(&on_name_lost));
    // clang-format on

    auto loop = Glib::MainLoop::create();
    loop->run();

    Gio::DBus::unown_name(id);
    return EXIT_SUCCESS;
}
