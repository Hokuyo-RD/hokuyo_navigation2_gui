#include <ros/ros.h>

#include <nav_msgs/GetMap.h>
#include <nav_msgs/OccupancyGrid.h>

#include <pcl/io/pcd_io.h>
#include <pcl_conversions/pcl_conversions.h>
#include <sensor_msgs/PointCloud2.h>

#include <pcl/filters/conditional_removal.h>         //Conditional filter header file
#include <pcl/filters/passthrough.h>                 //Pass filter header file
#include <pcl/filters/radius_outlier_removal.h>      //Radius filter header file
#include <pcl/filters/statistical_outlier_removal.h> //Statistics filter header file
#include <pcl/filters/voxel_grid.h>                  //Voxel filter header file
#include <pcl/point_types.h>

std::string g_fileDirectory;
std::string g_fileName;
std::string g_pcdFile;
std::string g_mapTopicName;

const std::string g_pcdFileFormat = ".pcd";

nav_msgs::OccupancyGrid g_mapTopicMsg;
//Minimum and maximum height
double g_threshold_Zmin = 0.5;
double g_threshold_Zmax = 5.0;
double g_threshold_radius = 0.5;
double g_mapResolution = 0.05;
//Point count threshold for radius filtering
int g_threshold_pointCount = 10;
int g_passThroughFlag = 0;

//Pointer to data after direct filtering
pcl::PointCloud<pcl::PointXYZ>::Ptr g_passFilterPointCloud;
//Radius filtered data pointer
pcl::PointCloud<pcl::PointXYZ>::Ptr g_radiusFilterPointCloud;
pcl::PointCloud<pcl::PointXYZ>::Ptr g_pcdPointCloud;

//Pass-through filtering
void PassThroughFilter(const double &thre_low, const double &thre_high, const bool &flag_in);
//Radius Filter
void RadiusOutlierFilter(const pcl::PointCloud<pcl::PointXYZ>::Ptr &pcd_cloud0, const double &radius, const int &thre_count);
//Convert to raster map data and publish
void SetMapTopicMsg(const pcl::PointCloud<pcl::PointXYZ>::Ptr cloud, nav_msgs::OccupancyGrid &msg);

int main(int argc, char **argv) {
  ros::init(argc, argv, "pcl_filters");
  ros::NodeHandle nh;                         //Automatic Startup and Shutdown of node
  ros::NodeHandle private_nh("~");            //namespace to the node constructor

  ros::Rate loop_rate(1.0);

  private_nh.param("file_directory", g_fileDirectory, std::string("/home/"));
  private_nh.param("file_name", g_fileName, std::string("map"));
  g_pcdFile = g_fileDirectory + g_fileName + g_pcdFileFormat;

  private_nh.param("threshold_z_min", g_threshold_Zmin, 0.5);
  private_nh.param("threshold_z_max", g_threshold_Zmax, 5.0);
  private_nh.param("pass_through_flag", g_passThroughFlag, 0);
  private_nh.param("threshold_radius", g_threshold_radius, 0.5);
  private_nh.param("threshold_point_Count", g_threshold_pointCount, 10);
  private_nh.param("map_resolution", g_mapResolution, 0.05);
  private_nh.param("map_topic_name", g_mapTopicName, std::string("map"));

  ros::Publisher map_topic_pub = nh.advertise<nav_msgs::OccupancyGrid>(g_mapTopicName, 1);

  pcl::PointCloud<pcl::PointXYZ>::Ptr pointCloud(new pcl::PointCloud<pcl::PointXYZ>);
  g_pcdPointCloud = pointCloud;

  // Download pcd file
  if (pcl::io::loadPCDFile<pcl::PointXYZ>(g_pcdFile, *g_pcdPointCloud) == -1) {
    PCL_ERROR("Couldn't read file: %s \n", g_pcdFile.c_str());
    return (-1);
  }

  ROS_INFO("Initial point cloud data points = %d\n", g_pcdPointCloud->points.size());
  //Pass-through filtering of data
  PassThroughFilter(g_threshold_Zmin, g_threshold_Zmax, bool(g_passThroughFlag));
  //Apply radius filtering to the data
  RadiusOutlierFilter(g_passFilterPointCloud, g_threshold_radius, g_threshold_pointCount);
  //Convert to raster map data and publish
  SetMapTopicMsg(g_radiusFilterPointCloud, g_mapTopicMsg);
  // SetMapTopicMsg(g_passFilterPointCloud, g_mapTopicMsg);

  while (ros::ok()) {
    map_topic_pub.publish(g_mapTopicMsg);

    loop_rate.sleep();

    ros::spinOnce();
  }

  return 0;
}

//The pass filter filters the point cloud to obtain data within the set height range
void PassThroughFilter(const double &thre_low, const double &thre_high, const bool &flag_in) 
{                 
  pcl::PointCloud<pcl::PointXYZ>::Ptr passFilter(new pcl::PointCloud<pcl::PointXYZ>);
  g_passFilterPointCloud = passFilter;
  
  // Creating a filter object
  pcl::PassThrough<pcl::PointXYZ> passthrough;
  //Input point cloud
  passthrough.setInputCloud(g_pcdPointCloud);
  //Set the operation on the z-axis
  passthrough.setFilterFieldName("z");
  //Set the filter range
  passthrough.setFilterLimits(thre_low, thre_high);
  // True: keep the filter outside the range, false: keep the filter within the range
  passthrough.setFilterLimitsNegative(flag_in);
  //Perform filtering and store
  passthrough.filter(*g_passFilterPointCloud);
  // test Save the filtered point cloud to a file
  // pcl::io::savePCDFile<pcl::PointXYZ>(g_fileDirectory + "map_filter.pcd", *g_passFilterPointCloud);
  ROS_INFO("After pass through filtering, point cloud size  = %d\n", g_passFilterPointCloud->points.size());
}

//Radius Filter
void RadiusOutlierFilter(const pcl::PointCloud<pcl::PointXYZ>::Ptr &pcd_cloud0, const double &radius, const int &thre_count) 
{
  pcl::PointCloud<pcl::PointXYZ>::Ptr radiusFilter(new pcl::PointCloud<pcl::PointXYZ>);
  g_radiusFilterPointCloud = radiusFilter;
  
  //Creating a filter
  pcl::RadiusOutlierRemoval<pcl::PointXYZ> radiusoutlier;
  //Set up the input point cloud
  radiusoutlier.setInputCloud(pcd_cloud0);
  //Set the radius and find nearby points within this range
  radiusoutlier.setRadiusSearch(radius);
  //Set the number of neighboring points of the query point. Points smaller than the threshold will be deleted.
  radiusoutlier.setMinNeighborsInRadius(thre_count);
  radiusoutlier.filter(*g_radiusFilterPointCloud);
  //test Save the filtered point cloud to a file
  // pcl::io::savePCDFile<pcl::PointXYZ>(g_fileDirectory + "map_radius_filter.pcd", *g_radiusFilterPointCloud);
  ROS_INFO("After radius outlier filtering, point cloud size = %d\n", g_radiusFilterPointCloud->points.size());
}

//Convert to raster map data and publish
void SetMapTopicMsg(const pcl::PointCloud<pcl::PointXYZ>::Ptr cloud,
                    nav_msgs::OccupancyGrid &msg) {
  msg.header.seq = 0;
  msg.header.stamp = ros::Time::now();
  msg.header.frame_id = "map";

  msg.info.map_load_time = ros::Time::now();
  msg.info.resolution = g_mapResolution;

  double x_min = std::numeric_limits<double>::max();
  double x_max = std::numeric_limits<double>::lowest();
  double y_min = std::numeric_limits<double>::max();
  double y_max = std::numeric_limits<double>::lowest();
  double z_max_grey_rate = 0.05;
  double z_min_grey_rate = 0.95;
  //? ? ??
  double k_line =
      (z_max_grey_rate - z_min_grey_rate) / (g_threshold_Zmax - g_threshold_Zmin);
  double b_line =
      (g_threshold_Zmax * z_min_grey_rate - g_threshold_Zmin * z_max_grey_rate) /
      (g_threshold_Zmax - g_threshold_Zmin);

  if (cloud->points.empty()) {
    ROS_WARN("pcd is empty!\n");
    return;
  }

  for (const auto & point : cloud-> points) {
    x_min = std::min(x_min, (double)point.x);
    x_max = std::max(x_max, (double)point.x);
    y_min = std::min(y_min, (double)point.y);
    y_max = std::max(y_max, (double)point.y);
  }
  //Determination of origin
  msg.info.origin.position.x = x_min;
  msg.info.origin.position.y = y_min;
  msg.info.origin.position.z = 0.0;
  msg.info.origin.orientation.x = 0.0;
  msg.info.origin.orientation.y = 0.0;
  msg.info.origin.orientation.z = 0.0;
  msg.info.origin.orientation.w = 1.0;
  //Set the grid map size
  msg.info.width = std::ceil((x_max - x_min) / g_mapResolution);
  msg.info.height = std::ceil((y_max - y_min) / g_mapResolution);
  //The coordinates of a point in the actual map are (x, y), and the corresponding coordinates in the grid map are [x*map.info.width+y]
  msg.data.resize(msg.info.width * msg.info.height);
  msg.data.assign(msg.info.width * msg.info.height, 0);

  for (const auto & point : cloud-> points) {
    int i = std::floor((point.x - x_min) / g_mapResolution);
    int j = std::floor((point.y - y_min) / g_mapResolution);
    if ((i >= 0 && i < msg.info.width) && (j >= 0 && j < msg.info.height)) {
        // The occupancy probability of the grid map is [0,100], which is set to occupy
       msg.data[i + j * msg.info.width] = 100;
      //    msg.data[i + j * msg.info.width] = int(255 * (cloud->points[iter].z *
      //    k_line + b_line)) % 255;
    }
  }
  
  ROS_INFO("Map data size = %d\n", msg.data.size());
}
