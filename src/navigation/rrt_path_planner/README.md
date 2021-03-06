
## 安装

- ompl

使用现成的RRT，RRT*算法。这是一个运动规划库，有不同的路径规划算法可以使用

```bash
git clone https://github.com/ompl/ompl.git
cd ompl
mkdir -p build/Release
cd build/Release
cmake ../..
make -j 4
sudo make install
```

- fcl

碰撞检测，用于检测代表飞行器的几何形状和octomap表示的长方体是否发生碰撞，这是ompl要求的碰撞检测

```bash
git clone https://github.com/flexible-collision-library/fcl.git

sudo add-apt-repository --yes ppa:libccd-debs/ppa
sudo apt-get update

sudo apt-get install libeigen3-dev
sudo apt-get install libccd-dev

cd fcl
mkdir build
cd build
cmake ..
make -j 4
sudo  make install
```


- octomap

ros-kinetic-octomap
版本1.8以上的，这个路径规划我把所有版本更新到最新，对应的ros版本是kinetic,使用`sudo apt-get install ros-kinetic-octomap ros-kinetic-octomap-mapping`

## 使用

```bash
rosrun path_planning path_planning_node
```

使用已有的地图进行测试

```bash
roscd path_planning
rosrun octomap_server octomap_server_node data/grad.bt
python scripts/planClient.py
#因为地图是在map坐标系。路径是在world坐标系，你要把所有topic显示在rviz，做一个坐标变换
cd launch
roslaunch pubTf.launch
```

上面的`grad.bt`是我建好的一张地图，`planClient.py`是一个脚本文件，发布起点位置，终点位置进行路径规划，会输出可行的路径出来．

有问题直接看输出，一般是起始点或者是终点位置处有障碍物。

可以直接看终端会输出一系列的路径点，或者`rostopic echo /path_plan`，但是注意路径发布的是在`world`坐标系下，3D occupied map是在`map`坐标系下的,rviz不会同时显示两个topic，除非你自己提供一个从`map-->world`的坐标转换。

## SJ Revised

This package provide a service that recieve a octomap, start_pose and goal_pose, then using rrt* algorithm to plan a path and make sure that each waypoint's pose is heading.

### Subscribed topic 

| topic name | topic type | description |
|------------|------------|---------|
|"/octomap_binary" | geometry_msgs::PoseStamped | An octree map representing the enviroment around the uav 

### Provided service

| service name | service type | description |
|------------|------------|---------|
|"/rrt_planner_server" | nav_msgs::GetPlan | rrt* planning service

### Note
- All topic are represented in the map coordinate system 

- In order to reduce the time taken to plan a path, please make sure that the starting point and the ending point are at the same height. **Only plan in the starting heigth+-0.1m space**




