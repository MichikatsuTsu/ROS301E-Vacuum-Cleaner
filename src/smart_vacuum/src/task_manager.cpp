#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "std_msgs/msg/string.hpp"
#include "smart_vacuum_interfaces/msg/vacuum_status.hpp"
#include "smart_vacuum_interfaces/action/clean_session.hpp"

using CleanSession = smart_vacuum_interfaces::action::CleanSession;
using GoalHandle = rclcpp_action::ServerGoalHandle<CleanSession>;
using VacuumStatus = smart_vacuum_interfaces::msg::VacuumStatus;

class TaskManager : public rclcpp::Node
{
public:
  TaskManager() : Node("task_manager")
  {
    status_subscriber_ = create_subscription<VacuumStatus>(
      "/vacuum/status", 10,
      [this](VacuumStatus::SharedPtr msg) {
        battery_level_.store(msg->battery_level);
      });

    floor_subscriber_ = create_subscription<std_msgs::msg::String>(
      "/vacuum/active_floor", rclcpp::QoS(1).transient_local(),
      [this](std_msgs::msg::String::SharedPtr msg) {
        std::lock_guard<std::mutex> lock(floor_mutex_);
        active_floor_ = msg->data;
        RCLCPP_INFO(get_logger(), "Active map: %s", active_floor_.c_str());
      });

    action_server_ = rclcpp_action::create_server<CleanSession>(
      this, "/vacuum/clean",
      std::bind(&TaskManager::handle_goal, this,
        std::placeholders::_1, std::placeholders::_2),
      std::bind(&TaskManager::handle_cancel, this, std::placeholders::_1),
      std::bind(&TaskManager::handle_accepted, this, std::placeholders::_1));

    RCLCPP_INFO(get_logger(), "Task manager ready");
  }

  ~TaskManager() override
  {
    if (worker_.joinable()) {
      worker_.join();
    }
  }

private:
  rclcpp_action::GoalResponse handle_goal(
    const rclcpp_action::GoalUUID & uuid,
    std::shared_ptr<const CleanSession::Goal> goal)
  {
    (void)uuid;
    const bool valid_mode =
      goal->mode == "MAP_ONLY" ||
      goal->mode == "CLEAN_EXISTING_MAP" ||
      goal->mode == "SLAM_AND_CLEAN";
    const bool valid_power =
      goal->power_mode == "ECO" ||
      goal->power_mode == "STANDARD" ||
      goal->power_mode == "TURBO";

    if (!valid_mode || !valid_power) {
      RCLCPP_WARN(get_logger(), "Rejected unsupported mode or power mode");
      return rclcpp_action::GoalResponse::REJECT;
    }

    {
      std::lock_guard<std::mutex> lock(floor_mutex_);
      if (active_floor_.empty()) {
        RCLCPP_WARN(get_logger(), "Load a floor before starting a session");
        return rclcpp_action::GoalResponse::REJECT;
      }
    }

   
    if (busy_.exchange(true)) {
      RCLCPP_WARN(get_logger(), "A session is already running");
      return rclcpp_action::GoalResponse::REJECT;
    }
    RCLCPP_INFO(get_logger(), "Accepted %s / %s",
      goal->mode.c_str(), goal->power_mode.c_str());
    return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
  }

  rclcpp_action::CancelResponse handle_cancel(
    const std::shared_ptr<GoalHandle> goal_handle)
  {
    (void)goal_handle;
    RCLCPP_INFO(get_logger(), "Cancellation requested (return to dock)");
    return rclcpp_action::CancelResponse::ACCEPT;
  }

  void handle_accepted(const std::shared_ptr<GoalHandle> goal_handle)
  {
    if (worker_.joinable()) {
      worker_.join();
    }
    worker_ = std::thread(&TaskManager::execute, this, goal_handle);
  }

  void execute(const std::shared_ptr<GoalHandle> goal_handle)
  {
    const auto goal = goal_handle->get_goal();
    const auto started = std::chrono::steady_clock::now();
    const std::vector<std::string> rooms = {
      "Living Room", "Kitchen", "Bedroom", "Hallway"
    };
    const int total_steps = 20;
    float cleaned_area = 0.0f;
    rclcpp::Rate rate(1.0);

    auto make_result = [&]() {
      auto result = std::make_shared<CleanSession::Result>();
      result->total_area_cleaned_m2 = cleaned_area;
      result->elapsed_time_seconds =
        std::chrono::duration<float>(
          std::chrono::steady_clock::now() - started).count();
      return result;
    };

    for (int step = 1; step <= total_steps && rclcpp::ok(); ++step) {
      if (goal_handle->is_canceling()) {
        auto result = make_result();
        result->completion_code = "CANCELED";
        goal_handle->canceled(result);
        busy_.store(false);
        RCLCPP_INFO(get_logger(), "Session canceled: returning to dock");
        return;
      }

     
      if (goal->mode != "MAP_ONLY") {
        cleaned_area += goal->power_mode == "TURBO" ? 2.5f :
                        goal->power_mode == "ECO" ? 1.0f : 1.5f;
      }

      auto feedback = std::make_shared<CleanSession::Feedback>();
      feedback->current_room = rooms[(step - 1) / 5];
      feedback->completion_percentage = 100.0f * step / total_steps;
      feedback->battery_level = battery_level_.load();
      goal_handle->publish_feedback(feedback);
      RCLCPP_INFO(get_logger(), "Progress: %.0f%% (%s)",
        feedback->completion_percentage, feedback->current_room.c_str());

      if (step != total_steps) {
        rate.sleep();
      }
    }

    if (rclcpp::ok()) {
    
      auto result = make_result();
      if (goal_handle->is_canceling()) {
        result->completion_code = "CANCELED";
        goal_handle->canceled(result);
      } else {
        result->completion_code = "SUCCESS";
        goal_handle->succeed(result);
      }
    }
    busy_.store(false);
  }

  std::atomic<float> battery_level_{100.0f};
  std::atomic<bool> busy_{false};
  std::mutex floor_mutex_;
  std::string active_floor_;
  std::thread worker_;
  rclcpp::Subscription<VacuumStatus>::SharedPtr status_subscriber_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr floor_subscriber_;
  rclcpp_action::Server<CleanSession>::SharedPtr action_server_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<TaskManager>());
  rclcpp::shutdown();
  return 0;
}
