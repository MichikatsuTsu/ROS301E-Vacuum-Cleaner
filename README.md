# ROS 301E - Simulated Autonomous Vacuum Cleaner

ROS 2 Humble workspace (C++ / `rclcpp`) with custom message, service and action interfaces.

## Software architecture

- `hardware_driver`: publishes `smart_vacuum_interfaces/msg/VacuumStatus` on `/vacuum/status` at 1 Hz.
- `map_loader`: service server at `/vacuum/set_floor_level`. It stores a 2D occupancy grid and initial pose for `Floor_1` or `Floor_2`. Publishes the selected floor on `/vacuum/active_floor` (transient local).
- `task_manager`: action server at `/vacuum/clean`, subscribes to vacuum status and active floor. Simulates one cleaning/mapping session at a time, publishes progress, and handles cancellation.

The grid, sensor readings, cleaned area, and task timing are **simulated**; no SLAM, navigation, or hardware control is implemented.

## Build (Ubuntu 22.04, ROS 2 Humble)

```bash
cd ~/vacum_ws
source /opt/ros/humble/setup.bash
colcon build --packages-up-to smart_vacuum
source install/setup.bash
```

## Run

Terminal 1:

```bash
source /opt/ros/humble/setup.bash
source ~/vacum_ws/install/setup.bash
ros2 launch smart_vacuum vacuum.launch.py
```

Terminal 2 (call the floor service **before** sending a cleaning goal):

```bash
source /opt/ros/humble/setup.bash
source ~/vacum_ws/install/setup.bash
ros2 service call /vacuum/set_floor_level smart_vacuum_interfaces/srv/SetFloorLevel "{floor_id: 'Floor_2'}"
```

Terminal 2 (action goal and live feedback):

```bash
ros2 action send_goal /vacuum/clean smart_vacuum_interfaces/action/CleanSession "{mode: 'CLEAN_EXISTING_MAP', power_mode: 'TURBO'}" --feedback
```

Terminal 3 (cancel the in-progress goal **before it finishes**):

```bash
source /opt/ros/humble/setup.bash
source ~/vacum_ws/install/setup.bash
ros2 service call /vacuum/clean/_action/cancel_goal action_msgs/srv/CancelGoal "{goal_info: {goal_id: {uuid: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]}, stamp: {sec: 0, nanosec: 0}}}"
```

A zero UUID and timestamp request cancellation of all active goals for this action server. It is intended here only for a single test goal.

## Additional verification

```bash
ros2 topic echo /vacuum/status
ros2 topic hz /vacuum/status
ros2 action list
ros2 node list
ros2 run rqt_graph rqt_graph
ros2 run rqt_topic rqt_topic
ros2 run rqt_plot rqt_plot
```

For `rqt_plot`, choose `/vacuum/status/battery_level`. For a negative service test:

```bash
ros2 service call /vacuum/set_floor_level smart_vacuum_interfaces/srv/SetFloorLevel "{floor_id: 'Floor_9'}"
```

Other action goal values: `MAP_ONLY`, `SLAM_AND_CLEAN` and power modes `ECO`, `STANDARD`, `TURBO`.

## Report and screenshots

The assignment requires a single PDF containing an architecture diagram, code explanation, V1-V4 screenshots and a GitHub repository link. See `REPORT_TEMPLATE.html` and save real screenshots in the `screenshots/` directory:

- `screenshots/V1_graph.png` - rqt_graph
- `screenshots/V2_topic.png` - rqt_topic or rqt_plot
- `screenshots/V3_service.png` - successful CLI service call
- `screenshots/V4_action.png` - action feedback and final status/cancel response, ideally showing both in one screenshot (or add more screenshots)

Open the HTML in a browser, fill in your details/GitHub URL, and use Print > Save as PDF after inserting screenshots.
