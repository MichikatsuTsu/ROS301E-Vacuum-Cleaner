#include <chrono>
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "smart_vacuum_interfaces/msg/vacuum_status.hpp"

using VacuumStatus = smart_vacuum_interfaces::msg::VacuumStatus;

class HardwareDriver : public rclcpp::Node
{
public:
  HardwareDriver() : Node("hardware_driver")
  {
    publisher_ = create_publisher<VacuumStatus>("/vacuum/status", 10);
    timer_ = create_wall_timer(
      std::chrono::seconds(1),
      [this]() { publish_status(); });
    RCLCPP_INFO(get_logger(), "Hardware driver started");
  }

private:
  void publish_status()
  {
    VacuumStatus status;
    status.battery_level = battery_level_;
    status.dustbin_status = "OK";
    status.cliff_detected = false;
    status.bumper_pressed = false;
    publisher_->publish(status);

    
    if (battery_level_ > 5.0f) {
      battery_level_ -= 0.2f;
    }
  }

  float battery_level_ = 100.0f;
  rclcpp::Publisher<VacuumStatus>::SharedPtr publisher_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<HardwareDriver>());
  rclcpp::shutdown();
  return 0;
}
