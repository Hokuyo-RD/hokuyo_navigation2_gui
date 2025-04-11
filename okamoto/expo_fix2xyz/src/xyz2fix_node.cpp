#include "expo_fix2xyz/xyz2fix_node_lib.h"

int main(int argc, char **argv) {

    ros::init(argc, argv, "xyz2fix");
    XYZLLATransNode transformer;

    ros::spin();
    return 0;
}