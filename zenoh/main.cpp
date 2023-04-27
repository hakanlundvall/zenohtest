#include <spdlog/spdlog.h>

#include <csignal>
#include <thread>
#include <zenoh.hxx>

#include "src/test.pb.h"

namespace {
volatile std::sig_atomic_t gSignalStatus;
}

void signal_handler(int s)
{
   spdlog::info("Received signal {}", s);
   gSignalStatus = s;
}

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
   ZenohPublisher(const std::string& topic, ZenohSession& session);

   bool publish(std::string_view data)
   {
      return m_publisher.put(data);
   }

private:
   std::string m_topic;
   zenoh::Publisher m_publisher;
};

class ZenohSubscriber
{
public:
   ZenohSubscriber(const std::string& topic, ZenohSession& session);

private:
   std::string m_topic;
   zenoh::Subscriber m_subscriber;
};

class ZenohSession
{
public:
   ZenohSession()
   : session(std::get<zenoh::Session>(
      zenoh::open(zenoh::Config(::zc_config_from_file("./zenoh.json5")))))
   {
      spdlog::info("Zenoh session created");
   }
   ~ZenohSession()
   {
      spdlog::info("Zenoh session destroyed");
   }

   auto make_publisher(const char* topic)
   {
      zenoh::KeyExprView expr(topic);
      return std::get<zenoh::Publisher>(
         session.declare_publisher(expr, pub_options()));
   }

   auto make_subscriber(const char* topic)
   {
      using namespace std::chrono;
      zenoh::KeyExprView expr(topic);
      auto f = [](const zenoh::Sample* sample) {
         auto t = high_resolution_clock::now().time_since_epoch();
         Data d;
         d.ParseFromArray(sample->payload.start, sample->payload.len);
         auto t2 = nanoseconds(d.ts().seconds() * 1000000000 + d.ts().nanos());

         if (d.id() == 0 || d.id() == 374)
            spdlog::info(
               "Got {}, ts = {}.{}, send jitter = {} us, delay = {} us",
               sample->get_keyexpr().as_string_view(),
               d.ts().seconds(),
               d.ts().nanos() / 1000000,
               d.jitter(),
               duration_cast<microseconds>(t - t2).count());
      };
      return std::get<zenoh::Subscriber>(session.declare_subscriber(expr, f));
   }

   std::unique_ptr<ZenohPublisher> makePublisher(const char* topic)
   {
      return std::make_unique<ZenohPublisher>(std::string(topic), *this);
   }

   std::unique_ptr<ZenohSubscriber> makeSubscriber(const char* topic)
   {
      spdlog::info("Making subscriber {}", topic);
      return std::make_unique<ZenohSubscriber>(std::string(topic), *this);
   }

private:
   zenoh::Session session;
};

std::unique_ptr<ZenohSession> create_zenoh_session()
{
   return std::make_unique<ZenohSession>();
}

ZenohPublisher::ZenohPublisher(const std::string& topic, ZenohSession& session)
: m_topic("prefix/db/" + topic)
, m_publisher(session.make_publisher(m_topic.c_str()))
{
}

ZenohSubscriber::ZenohSubscriber(const std::string& topic,
                                 ZenohSession& session)
: m_topic("prefix/db/" + topic)
, m_subscriber(session.make_subscriber(m_topic.c_str()))
{
}

int main(int argc, char** argv)
{
   {
      std::signal(SIGINT, signal_handler);
      bool publish = false;
      bool wildcard = false;
      bool subscribe = false;

      for (int i = 1; i < argc; ++i)
      {
         std::string arg(argv[i]);
         if (arg == "-p")
            publish = true;
         else if (arg == "-w")
            wildcard = true;
         else if (arg == "-s")
            subscribe = true;
      }
      spdlog::info("Start");
      auto session = create_zenoh_session();
      std::vector<std::unique_ptr<ZenohPublisher>> publishers;
      std::vector<std::unique_ptr<ZenohSubscriber>> subscibers;

      if (publish)
      {
         for (int i = 0; i < 5000; ++i)
         {
            publishers.emplace_back(
               session->makePublisher(fmt::format("DBI_{}", i).c_str()));
         }
      }
      if (subscribe)
      {
         if (wildcard)
         {
            subscibers.emplace_back(session->makeSubscriber("**"));
         }
         else
         {
            for (int i = 0; i < 375; ++i)
            {
               subscibers.emplace_back(
                  session->makeSubscriber(fmt::format("DBI_{}", i).c_str()));
            }
         }
      }

      auto ts =
         std::chrono::steady_clock::now() + std::chrono::milliseconds(80);

      if (publish)
      {
         spdlog::info("Start publishing");
         while (gSignalStatus == 0)
         {
            std::this_thread::sleep_until(ts);
            for (int i = 0; i < 375; ++i)
            {
               auto jitter = std::chrono::steady_clock::now() - ts;
               auto t = std::chrono::high_resolution_clock::now();
               auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
                                 t.time_since_epoch())
                                 .count();
               auto nanos =
                  std::chrono::duration_cast<std::chrono::nanoseconds>(
                     t.time_since_epoch() - std::chrono::seconds(seconds))
                     .count();
               Data data;
               data.set_id(i);
               data.mutable_ts()->set_seconds(seconds);
               data.mutable_ts()->set_nanos(nanos);
               data.set_jitter(
                  std::chrono::duration_cast<std::chrono::microseconds>(jitter)
                     .count());

               auto before = std::chrono::high_resolution_clock::now();
               publishers[i]->publish(data.SerializeAsString());
               auto after = std::chrono::high_resolution_clock::now();
               auto duration =
                  std::chrono::duration_cast<std::chrono::milliseconds>(
                     after - before);
               if (duration > std::chrono::microseconds(10))
               {
                  spdlog::info("Blocktime: {} ms", duration.count());
               }
            }
            ts += std::chrono::milliseconds(80);
         }
      }
      else
      {
         spdlog::info("Subscribe only");
         while (gSignalStatus == 0)
         {
            std::this_thread::sleep_for(std::chrono::seconds(1));
         }
      }
      spdlog::info("Exiting");
   }
   std::this_thread::sleep_for(std::chrono::seconds(1));
}