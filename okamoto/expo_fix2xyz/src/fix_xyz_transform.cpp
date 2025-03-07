// #define DEBUG_MODE
#include "expo_fix2xyz/fix_xyz_transform.h"



// クォータニオンから回転行列を計算する関数.
Eigen::Matrix3d RotMatFromQuat(const Eigen::Vector4d& quat){
    Eigen::Matrix3d Rot;
        Rot << 2*(quat(0)*quat(0) + quat(3)*quat(3))-1 , 2*(quat(0)*quat(1) - quat(2)*quat(3)), 2*(quat(0)*quat(2) + quat(1)*quat(3)) ,
        2*(quat(0)*quat(1) + quat(2)*quat(3)) , 2*(quat(1)*quat(1) + quat(3)*quat(3))-1 , 2*(quat(1)*quat(2) - quat(0)*quat(3)) ,
        2*(quat(0)*quat(2) - quat(1)*quat(3)) , 2*(quat(1)*quat(2) + quat(0)*quat(3)) , 2*(quat(2)*quat(2) + quat(3)*quat(3))-1 ;

    return Rot;
}

using namespace fix_xyz_trans;

int LatlonUtmTrans::judge_utm_zone(double longitude){
    //経度を6で割って切り上げ.
    int zone = (int)(longitude + 180.0 + 5 )/6;
    return zone;
}

std::string LatlonUtmTrans::utm_zone_to_epsg(int utm_zone){
    int epsg_num = utm_zone + 32600;
    return "EPSG:" + std::to_string(epsg_num);
}


void LatlonUtmTrans::set_origin(fix_xyz_trans::LatLonAlt orig_pose_, Eigen::Vector4d orig_quat_){
    set_origin_flg = true;
    orig_pose = orig_pose_;
    orig_R = RotMatFromQuat(orig_quat_);
    int utm_zone = judge_utm_zone(orig_pose_.longitude);
    if(utm_zone > 0 && utm_zone <=60){
        epsg_code = utm_zone_to_epsg(utm_zone);
        set_epsg_flg = true;
    }

    PJ_CONTEXT *C_orig;
    PJ *P_orig;
    PJ *norm_orig;
    PJ_COORD a_orig, b_orig;
    C_orig = proj_context_create();
    P_orig = proj_create_crs_to_crs( C_orig, "EPSG:4326", epsg_code.c_str() , NULL);
    a_orig = proj_coord(orig_pose.latitude, orig_pose.longitude, 0, 0);
    b_orig = proj_trans(P_orig, PJ_FWD, a_orig);

    orig_vec(0) = b_orig.xy.x;
    orig_vec(1) = b_orig.xy.y;
    orig_vec(2) = orig_pose.altitude;
    DEBUG_PRINT(orig_vec);
    
    proj_destroy(P_orig);
    proj_context_destroy(C_orig);
    set_origin_vector_flg = true;
}

// 基本つかわない.
void LatlonUtmTrans::set_epsg_code(int epsg_code_num){
    epsg_code = "EPSG:" + std::to_string(epsg_code_num);
    set_epsg_flg = true;
}

Eigen::Vector3d LatlonUtmTrans::get_xyz_from_latlonalt(LatLonAlt latlonalt){
    std::string fix_debug_msg = "get_fix";
    DEBUG_PRINT(fix_debug_msg);

    Eigen::Vector3d xyz = Eigen::Vector3d::Zero();
    
    int utm_zone;
    PJ_CONTEXT *C;
    PJ *P;
    PJ *norm;
    PJ_COORD a, b;

    if(!set_epsg_flg){
        utm_zone = judge_utm_zone(latlonalt.longitude);
        if(utm_zone > 0 && utm_zone <=60){
            epsg_code = utm_zone_to_epsg(utm_zone);
            set_epsg_flg = true;
        }
        else{
            return xyz;
        }
    }

    if(!set_origin_flg || !set_origin_vector_flg){
        return xyz;
    }

    DEBUG_PRINT(epsg_code);
    DEBUG_PRINT(latlonalt.latitude);
    DEBUG_PRINT(latlonalt.longitude);

    C = proj_context_create();
    P = proj_create_crs_to_crs( C, "EPSG:4326", epsg_code.c_str() , NULL);
    if (0 == P) {
        double inf = std::numeric_limits<double>::infinity();
        xyz << inf, inf, inf;
    }
    else{
        a = proj_coord(latlonalt.latitude, latlonalt.longitude, 0, 0);
        b = proj_trans(P, PJ_FWD, a);

        xyz(0) = b.xy.x;
        xyz(1) = b.xy.y;
        xyz(2) = latlonalt.altitude;
    }
    
    proj_destroy(P);
    proj_context_destroy(C);

    fix_debug_msg = "return xyz from fix";
    DEBUG_PRINT(fix_debug_msg);


    // ここで座標変換.
    xyz = xyz - orig_vec;
    xyz = orig_R * xyz;

    return xyz;
}

LatLonAlt LatlonUtmTrans::get_latlonalt_from_xyz(Eigen::Vector3d xyz){
    std::string fix_debug_msg = "get_xy";
    DEBUG_PRINT(fix_debug_msg);

    LatLonAlt latlonalt;

    PJ_CONTEXT *C;
    PJ *P;
    PJ *norm;
    PJ_COORD a, b;

    double inf = std::numeric_limits<double>::infinity();
    if(!set_origin_flg || !set_epsg_flg){
        latlonalt.latitude = inf;
        latlonalt.longitude = inf;
        latlonalt.altitude = inf;
        return latlonalt;
    }

    // ここで座標変換.
    xyz = orig_R.transpose() * xyz;
    xyz = xyz + orig_vec;

    C = proj_context_create();
    P = proj_create_crs_to_crs( C, epsg_code.c_str() , "EPSG:4326", NULL);

    DEBUG_PRINT(epsg_code);

    if (0 == P) {
        latlonalt.latitude = inf;
        latlonalt.longitude = inf;
        latlonalt.altitude = inf;
    }
    else{
        a = proj_coord(xyz(0), xyz(1), 0, 0);
        b = proj_trans(P, PJ_FWD, a);

        latlonalt.latitude = b.xyz.x;
        latlonalt.longitude = b.xyz.y;
        latlonalt.altitude = xyz(2);
    }

    proj_destroy(P);
    proj_context_destroy(C);

    fix_debug_msg = "return fix from xyz";
    DEBUG_PRINT(fix_debug_msg);

    return latlonalt;
}


// 初期化処理.
LatlonUtmTrans::LatlonUtmTrans() {
    set_origin_flg = false;
    set_epsg_flg = false ;
    set_origin_vector_flg = false;
}
LatlonUtmTrans::~LatlonUtmTrans() {
}
