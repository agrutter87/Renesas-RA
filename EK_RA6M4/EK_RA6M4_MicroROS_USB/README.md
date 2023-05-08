# Instructions to build Renesas Micro-ROS e2studio projects from Windows:
NOTE: See https://github.com/micro-ROS/micro_ros_renesas_demos for initial e2studio projects that mine is based off of.

## Update to Latest FSP
1. Import project
2. Re-select latest toolchain version in project settings
3. Regenerate code based on latest FSP.

## Build the Micro-ROS library
1. Update e2studio project to replace pre-build command:

    cd ../micro_ros_renesas2estudio_component/library_generation && ./library_generation.sh "${cross_toolchain_flags}"

	to
	
	echo "${cross_toolchain_flags}"

2. Attempt to build e2studio project. It will fail, but record the output of the flags.
3. Install toolchain matching e2studio project.

Example for FSP 4.4.0:
tar -xf gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2
export PATH="~/gcc-arm-none-eabi-10.3-2021.10/bin/:$PATH"
arm-none-eabi-g++ --version

4. Setup WSL as shown in the Linux sections by installing required software.

Example:
sudo apt update && sudo apt upgrade -y
sudo apt install colcon
sudo apt install python-is-python3 # Might be optional?
sudo sh -c 'echo "deb [arch=amd64,arm64] http://repo.ros2.org/ubuntu/main `lsb_release -cs` main" > /etc/apt/sources.list.d/ros2-latest.list'
curl -s https://raw.githubusercontent.com/ros/rosdistro/master/ros.asc | sudo apt-key add -
sudo apt update
sudo apt install python3-colcon-common-extensions
sudo apt install python3-pip
pip3 install lark
git clone https://github.com/micro-ROS/micro_ros_renesas2estudio_component.git
cd micro_ros_renesas2estudio_component/library_generation

5. Pull sources and build project using command that was replaced in the pre-build command.

Example for FSP 4.4.0:
< install any additional software that is discovered when attempting to build >
./library_generation.sh "-mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O2 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wunused -Wuninitialized -Wall -Wextra -Wmissing-declarations -Wconversion -Wpointer-arith -Wshadow -Wlogical-op -Waggregate-return -Wfloat-equal  -g"

6. Copy the Micro-ROS "libmicroros" directory from the WSL instance to the e2studio project

Example:

NOTE: This is wayyyy more than is necessary to compile e2studio project.
cp -r ../libmicroros <insert e2studio project path here>

Instead do this:
cp -r ../libmicroros/include <insert e2studio project path here>/libmicroros
cp -r ../libmicroros/libmicroros.a <insert e2studio project path here>/libmicroros


## General Setup of ROS Host on Linux
Installing ROS:
https://docs.ros.org/en/rolling/Installation.html
https://docs.ros.org/en/rolling/Installation/Ubuntu-Install-Debians.html

Setting up Micro ROS Agent to run with ROS:
https://micro.ros.org/docs/tutorials/core/first_application_linux/
 
## General Reminders for how to start ROS agent on Linux PC
First terminal:
source /opt/ros/rolling/setup.bash
source ~/microros_ws/install/local_setup.bash
USB Serial Example:
ros2 run micro_ros_agent micro_ros_agent serial --dev /dev/ttyACM0 -v6

Second terminal:
source /opt/ros/rolling/setup.bash
ros2 topic echo /int_publisher

## Using foxglove-bridge for remote debugging
https://console.foxglove.dev/dashboard

https://foxglove.dev/docs/studio/connection/ros2
https://index.ros.org/p/foxglove_bridge/#rolling

## Other links for information
https://micro.ros.org/
https://github.com/micro-ROS