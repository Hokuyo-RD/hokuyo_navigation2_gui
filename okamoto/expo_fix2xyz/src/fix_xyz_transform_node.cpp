#include "expo_fix2xyz/fix_xyz_transform_node_lib.h"

int main(int argc, char **argv) {

    ros::init(argc, argv, "fix_xyz_transform");
    LatlonUtmTransNode transformer;

    ros::spin();
    return 0;
}