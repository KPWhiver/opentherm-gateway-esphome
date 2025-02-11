#include "button.h"

namespace esphome {
namespace otgw {

void OpenthermGatewayButton::set_callback(decltype(OpenthermGatewayButton::_callback) &&callback) {
  _callback = callback;
}

void OpenthermGatewayButton::press_action() {
  _callback();
}

}  // namespace otgw
}  // namespace esphome
