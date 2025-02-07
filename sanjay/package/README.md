## pcd2pgm_package
Convert point cloud pcd file to 2D raster map

## Build command
Place the source code in catkin workspace
$ catkin_make 

## Launch command
roslaunch pcd2pgm run.launch


## Map-server 
## Open terminal where you want to generate pgm file
## execution command
rosrun map_server map_saver -f mymap
or
rosrun map_server map_saver --occ 90 --free 10 -f mymap
