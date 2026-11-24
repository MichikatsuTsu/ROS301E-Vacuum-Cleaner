#include <memory>
#include <string>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "smart_vacuum_interfaces/srv/set_floor_level.hpp"

using SetFloorLevel = smart_vacuum_interfaces::srv::SetFloorLevel;

class MapLoader : public rclcpp::Node
{
public:
  MapLoader() : Node("map_loader")
  {
    auto qos = rclcpp::QoS(1).transient_local();
    floor_publisher_ = create_publisher<std_msgs::msg::String>(
      "/vacuum/active_floor", qos);

    service_ = create_service<SetFloorLevel>(
      "/vacuum/set_floor_level",
      [this](const std::shared_ptr<SetFloorLevel::Request> request,
             std::shared_ptr<SetFloorLevel::Response> response) {
        set_floor(request, response);
      });

    RCLCPP_INFO(get_logger(), "Map loader ready");
  }

private:
  struct Pose
  {
    double x;
    double y;
    double yaw;
  };

  void set_floor(const std::shared_ptr<SetFloorLevel::Request> request,
                 std::shared_ptr<SetFloorLevel::Response> response)
  {
    
    if (request->floor_id == "Floor_1") {
      grid_map_ = {
        {1, 1, 1, 1, 1},
        {1, 0, 0, 0, 1},
        {1, 0, 1, 0, 1},
        {1, 0, 0, 0, 1},
        {1, 1, 1, 1, 1}
      };
      initial_pose_ = {1.0, 1.0, 0.0};
    } else if (request->floor_id == "Floor_2") {
      grid_map_ = {
        {1, 1, 1, 1, 1},
        {1, 0, 0, 0, 1},
        {1, 0, 0, 1, 1},
        {1, 0, 0, 0, 1},
        {1, 1, 1, 1, 1}
      };
      initial_pose_ = {2.0, 1.0, 1.57};
    } else {
      response->success = false;
      response->message = "Unknown floor: " + request->floor_id;
      RCLCPP_WARN(get_logger(), "%s", response->message.c_str());
      return;
    }

    current_floor_ = request->floor_id;
    response->success = true;
    response->message = "Loaded " + current_floor_ + " grid (" +
      std::to_string(grid_map_.size()) + "x" +
      std::to_string(grid_map_.front().size()) + ") and initial pose";

    std_msgs::msg::String floor_message;
    floor_message.data = current_floor_;
    floor_publisher_->publish(floor_message);

    RCLCPP_INFO(get_logger(), "%s, pose=(%.2f, %.2f, %.2f)",
      response->message.c_str(), initial_pose_.x,
      initial_pose_.y, initial_pose_.yaw);
  }

  std::string current_floor_;
  std::vector<std::vector<int>> grid_map_;
  Pose initial_pose_{0.0, 0.0, 0.0};
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr floor_publisher_;
  rclcpp::Service<SetFloorLevel>::SharedPtr service_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<MapLoader>());
  rclcpp::shutdown();
  return 0;
}
