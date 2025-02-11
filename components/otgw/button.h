#pragma once

#include "esphome/components/button/button.h"

namespace esphome {
namespace otgw {

class OpenthermGatewayButton : public button::Button {
 protected:
  std::function<void()> _callback;

 public:
  void set_callback(decltype(_callback) &&callback);

  void press_action() override;
};

}  // namespace otgw
}  // namespace esphome
