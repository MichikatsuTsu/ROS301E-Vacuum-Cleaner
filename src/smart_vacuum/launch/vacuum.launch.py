from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        Node(package='smart_vacuum', executable='hardware_driver', output='screen'),
        Node(package='smart_vacuum', executable='map_loader', output='screen'),
        Node(package='smart_vacuum', executable='task_manager', output='screen'),
    ])
