FROM ubuntu:22.04

ARG DEBIAN_FRONTEND=noninteractive

# set time zone
ENV TZ=Europe/Warsaw
ENV LANG=en_US.UTF-8
ENV XDG_RUNTIME_DIR=/tmp

RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# nvidia-container-runtime
ENV NVIDIA_VISIBLE_DEVICES \
    ${NVIDIA_VISIBLE_DEVICES:-all}
ENV NVIDIA_DRIVER_CAPABILITIES \
    ${NVIDIA_DRIVER_CAPABILITIES:+$NVIDIA_DRIVER_CAPABILITIES,}graphics

# INSTALL SYSTEM DEPENDENCIES

RUN apt clean
RUN apt update && apt install -y --no-install-recommends \
        ca-certificates \
        devilspie \
        gnupg2 \
        mesa-utils \
        sudo \
        unzip \
        wget \
        xfce4-terminal \
        unzip \
        curl \
        wget \
        libssl-dev \
        zlib1g-dev \
        libreadline-dev

# INSTALL GPU DEPENDENCIES
RUN apt install nvidia-driver-525 -y
RUN apt-get install -y libglu1 xvfb libxcursor1 libvulkan1 mesa-vulkan-drivers vulkan-tools -y

# INSTALL ROS2-HUMBLE
RUN curl -sSL https://raw.githubusercontent.com/ros/rosdistro/master/ros.key -o /usr/share/keyrings/ros-archive-keyring.gpg
RUN echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/ros-archive-keyring.gpg] http://packages.ros.org/ros2/ubuntu $(. /etc/os-release && echo $UBUNTU_CODENAME) main" | sudo tee /etc/apt/sources.list.d/ros2.list > /dev/null

RUN apt update -y && apt install -y ros-humble-desktop

RUN touch /root/.bashrc \
 && echo "echo '|======================================================|'" >> /root/.bashrc \
 && echo "echo '| DO NOT SOURCE ROS IN TERMINAL WHERE YOU RUN LUNARSIM |'" >> /root/.bashrc \
 && echo "echo '|======================================================|'" >> /root/.bashrc

# CONFIGURE APPLICATION
RUN mkdir -p /root/lunarsim

RUN wget "https://github.com/PUTvision/LunarSim/releases/latest/download/LunarSim.tar.xz" \
 && cd /root/lunarsim \
 && tar xf /LunarSim.tar.xz \
 && rm -f /LunarSim.tar.xz \
 && chmod +x /root/lunarsim/LunarSim.x86_64

WORKDIR /root/lunarsim

# TODO: ADD ENTRYPOINT SCRIPT TO RUN SIMULATION
