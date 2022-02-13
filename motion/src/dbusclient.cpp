#include "dbusclient.h"

#include "common.h"
#include "config.h"
#include "constants.h"
#include "ofMain.h"

Glib::RefPtr<Glib::MainLoop> loop;
string Dbusclient::m_date;
string Dbusclient::m_ticket;

bool Dbusclient::on_main_loop_idle()
{
    loop->quit();
    return false;
}

bool Dbusclient::server_alive()
{
    return getDate().empty() == false;
}

const string& Dbusclient::getDate()
{
    m_date = "";
    std::locale::global(std::locale(""));
    Gio::init();

    loop = Glib::MainLoop::create();

    auto connection = Gio::DBus::Connection::get_sync(Gio::DBus::BUS_TYPE_SESSION);

    if (!connection) {
        std::cerr << "The user's session bus is not available." << std::endl;
        return m_date;
    }

    // Create the proxy to the bus asynchronously.
    // clang-format off
    Gio::DBus::Proxy::create(connection,
        "org.yoo.recorder",
        "/org/yoo/recorder",
        "org.yoo.recorder", sigc::ptr_fun(&on_dbus_proxy_GetDate));

    // clang-format on

    loop->run();
    return m_date;
}

const string& Dbusclient::start_recording()
{
    m_ticket = "";
    std::locale::global(std::locale(""));

    Gio::init();

    loop = Glib::MainLoop::create();

    auto connection = Gio::DBus::Connection::get_sync(Gio::DBus::BUS_TYPE_SESSION);

    if (!connection) {
        std::cerr << "The user's session bus is not available." << std::endl;
        return m_ticket;
    }

    // Create the proxy to the bus asynchronously.
    // clang-format off
    Gio::DBus::Proxy::create(connection,
        "org.yoo.recorder",
        "/org/yoo/recorder",
        "org.yoo.recorder", sigc::ptr_fun(&on_dbus_proxy_start_recording));

    // clang-format on

    loop->run();
    return m_ticket;
}

void Dbusclient::stop_recording(const string& ticket)
{
    m_ticket = ticket;
    std::locale::global(std::locale(""));

    Gio::init();

    loop = Glib::MainLoop::create();

    auto connection = Gio::DBus::Connection::get_sync(Gio::DBus::BUS_TYPE_SESSION);

    if (!connection) {
        std::cerr << "The user's session bus is not available." << std::endl;
        return;
    }

    // Create the proxy to the bus asynchronously.
    // clang-format off
    Gio::DBus::Proxy::create(connection,
        "org.yoo.recorder",
        "/org/yoo/recorder",
        "org.yoo.recorder", sigc::ptr_fun(&on_dbus_proxy_stop_recording));

    // clang-format on

    loop->run();
}

void Dbusclient::on_dbus_proxy_GetDate(Glib::RefPtr<Gio::AsyncResult>& result)
{
    auto proxy = Gio::DBus::Proxy::create_finish(result);

    if (!proxy) {
        std::cerr << "The proxy to the user's session bus was not successfully "
                     "created."
                  << std::endl;
        loop->quit();
        return;
    }

    try {
        const auto call_result = proxy->call_sync("GetTime");

        Glib::Variant<Glib::ustring> time_variant;
        call_result.get_child(time_variant);

        m_date = time_variant.get();

    } catch (const Glib::Error& error) {
        std::cerr << "Got an error: '" << error.what() << "'." << std::endl;
    }

    // Connect an idle callback to the main loop to quit when the main loop is
    // idle now that the method call is finished.
    Glib::signal_idle().connect(sigc::ptr_fun(&on_main_loop_idle));
}

void Dbusclient::on_dbus_proxy_start_recording(Glib::RefPtr<Gio::AsyncResult>& result)
{
    auto proxy = Gio::DBus::Proxy::create_finish(result);

    if (!proxy) {
        std::cerr << "The proxy to the user's session bus was not successfully "
                     "created."
                  << std::endl;
        loop->quit();
        return;
    }

    try {
        Config& m_config = m_config.getInstance();

        vector<Glib::VariantBase> args;
        args.push_back(Glib::Variant<Glib::ustring>::create(m_config.settings.uri));
        args.push_back(Glib::Variant<Glib::ustring>::create(string(TMPDIR)));
        args.push_back(Glib::Variant<Glib::ustring>::create(m_config.settings.storage));
        args.push_back(
            Glib::Variant<Glib::ustring>::create("motion_" + m_config.parameters.camname));

        const auto init_result =
            proxy->call_sync("start", Glib::VariantContainerBase::create_tuple(args));

        Glib::Variant<Glib::ustring> ticket_variant;
        init_result.get_child(ticket_variant);

        m_ticket = ticket_variant.get();

    } catch (const Glib::Error& error) {
        std::cerr << "Got an error: '" << error.what() << "'." << std::endl;
    }

    // Connect an idle callback to the main loop to quit when the main loop is
    // idle now that the method call is finished.
    Glib::signal_idle().connect(sigc::ptr_fun(&on_main_loop_idle));
}

void Dbusclient::on_dbus_proxy_stop_recording(Glib::RefPtr<Gio::AsyncResult>& result)
{
    auto proxy = Gio::DBus::Proxy::create_finish(result);

    if (!proxy) {
        std::cerr << "The proxy to the user's session bus was not successfully "
                     "created."
                  << std::endl;
        loop->quit();
        return;
    }

    try {
        std::vector<Glib::VariantBase> args;
        args.push_back(Glib::Variant<Glib::ustring>::create(m_ticket));

        proxy->call("stop", Glib::VariantContainerBase::create_tuple(args));

    } catch (const Glib::Error& error) {
        std::cerr << "Got an error: '" << error.what() << "'." << std::endl;
    }

    // Connect an idle callback to the main loop to quit when the main loop is
    // idle now that the method call is finished.
    Glib::signal_idle().connect(sigc::ptr_fun(&on_main_loop_idle));
}
