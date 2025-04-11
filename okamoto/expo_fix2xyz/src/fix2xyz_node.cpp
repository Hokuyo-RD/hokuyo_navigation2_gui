#include "expo_fix2xyz/fix2xyz_node_lib.h"

int main(int argc, char **argv) {

    ros::init(argc, argv, "fix2xyz");
    LLAXYZTransNode transformer;

    ros::spin();
    return 0;
}