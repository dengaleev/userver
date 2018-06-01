#pragma once

#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <server/handlers/http_handler_base.hpp>
#include <storages/redis/sentinel.hpp>
#include <taxi_config/component.hpp>

#include "../taxi_config/taxi_config.hpp"

namespace driver_authorizer {
namespace handlers {

class DriverSession : public server::handlers::HttpHandlerBase {
 public:
  static constexpr const char* kName = "handler-driver-session";

  DriverSession(const components::ComponentConfig&,
                const components::ComponentContext&);

  const std::string& HandlerName() const override;
  std::string HandleRequestThrow(
      const server::http::HttpRequest&,
      server::request::RequestContext&) const override;

 private:
  const bool is_session_ttl_update_enabled_;

  std::shared_ptr<storages::redis::Sentinel> redis_ptr_;
  const components::TaxiConfig<TaxiConfig>* taxi_config_component_;
};

}  // namespace handlers
}  // namespace driver_authorizer
