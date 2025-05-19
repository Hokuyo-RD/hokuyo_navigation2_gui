#include "safety_urg_node/safety_urg_node_lib.h"

int main(int argc, char **argv) {

    ros::init(argc, argv, "safety_urg");
    SafetyUrgNode sensor;

    ros::spin();
    return 0;
}
