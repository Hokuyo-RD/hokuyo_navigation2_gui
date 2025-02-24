#include <coordinate_transform/latlon_utm_transform_node_lib.h>

int main(int argc, char **argv) {

    ros::init(argc, argv, "latlon_utm_transform");
    LatlonUtmTransNode transformer;

    ros::spin();
    return 0;
}