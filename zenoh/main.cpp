#include <zenohcpp.h>
#include <spdlog/spdlog.h>
#include <thread>

auto put_options()
{
    zenoh::PublisherPutOptions options;
    options.set_encoding(Z_ENCODING_PREFIX_APP_OCTET_STREAM);
    return options;
}

auto pub_options()
{
    zenoh::PublisherOptions options;
    return options;
}

class ZenohSession;

class ZenohPublisher
{
public:
    ZenohPublisher(const std::string &topic, ZenohSession &session);

    bool publish(std::string_view data)
    {
        return m_publisher.put(data);
    }

private:
    std::string m_topic;
    zenoh::Publisher m_publisher;
};

class ZenohSession
{
public:
    ZenohSession()
        : session(std::get<zenoh::Session>(zenoh::open(zenoh::Config(::zc_config_from_file("./zenoh.json5")))))
    {
        spdlog::info("Zenoh session created");
    }
    ~ZenohSession()
    {
        spdlog::info("Zenoh session destroyed");
    }

    auto make_publisher(const char *topic)
    {
        zenoh::KeyExprView expr(topic);
        return std::get<zenoh::Publisher>(session.declare_publisher(expr, pub_options()));
    }

    std::unique_ptr<ZenohPublisher> makePublisher(const char *topic)
    {
        return std::make_unique<ZenohPublisher>(std::string(topic), *this);
    }

private:
    zenoh::Session session;
};

std::unique_ptr<ZenohSession> create_zenoh_session()
{
    return std::make_unique<ZenohSession>();
}

ZenohPublisher::ZenohPublisher(const std::string &topic, ZenohSession &session)
    : m_topic("prefix/db/" + topic),
      m_publisher(session.make_publisher(m_topic.c_str()))
{
}

int main()
{
    spdlog::info("Start");
    auto session = create_zenoh_session();
    std::vector<std::unique_ptr<ZenohPublisher>> publishers;

    for (int i = 0; i < 5000; ++i)
    {
        publishers.emplace_back(session->makePublisher(fmt::format("DBI_{}", i).c_str()));
    }

    auto ts = std::chrono::steady_clock::now() + std::chrono::milliseconds(80);

    while (true)
    {
        for (int i = 0; i < 375; ++i)
        {
            auto data = fmt::format("{} {}", i, std::chrono::steady_clock::now().time_since_epoch().count());
            auto before = std::chrono::high_resolution_clock::now();
            publishers[i]->publish(data);
            auto after = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(after-before);
            if (duration > std::chrono::microseconds(10))
            {
                spdlog::info("Blocktime: {} ms", duration.count());
            }

        }
        std::this_thread::sleep_until(ts);
        ts += std::chrono::milliseconds(80);
    }
}