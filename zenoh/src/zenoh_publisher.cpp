#include "zenoh_publisher.h"

std::unique_ptr<rcs::data::IMiddlewareSession> create_zenoh_session()
{
    return std::make_unique<ZenohSession>();
}

ZenohPublisher::ZenohPublisher(const std::string &topic, ZenohSession &session)
    : m_topic("epiroc/db/" + topic),
      m_publisher(session.make_publisher(m_topic.c_str()))
{
}
