#ifndef LATLON_UTM_TRANS
#define LATLON_UTM_TRANS


#ifdef DEBUG_MODE
#define DEBUG_PRINT(A) std::cout << (#A) << " : " << (A) << std::endl;
#else
#define DEBUG_PRINT(A) {;}
#endif

#include <string>
#include <proj.h>
#include <limits>
#include <iostream>
#include <Eigen/Dense>

namespace fix_xyz_trans{

    // 緯度経度を表す構造体.
    struct LatLonAlt {
        double latitude;
        double longitude;
        double altitude;
        LatLonAlt(){
            latitude = 0;
            longitude = 0;
            altitude = 0;
        }
    };
    // 緯度経度に角度を足した構造体.
    struct LLAWithOrientation {
        LatLonAlt lla;
        Eigen::Vector4d orientation;
        LLAWithOrientation(){
            orientation << 0,0,0,1;
        }
    };

    struct Pose {
        Eigen::Vector3d position;
        Eigen::Vector4d orientation;
        Pose(){
            position = Eigen::Vector3d::Zero();
            orientation << 0,0,0,1;
        }
    };

    class LLAXYZTrans {
    private:

        bool set_origin_flg;
        bool set_epsg_flg;
        bool set_origin_vector_flg;

        std::string epsg_code;
        LatLonAlt orig_pose;
        Eigen::Matrix3d orig_R;    // 緯度経度の基準姿勢.
        Eigen::Matrix4d orig_Q;
        Eigen::Vector3d orig_vec;

        // 経度からUTMゾーンを取得する関数.
        int judge_utm_zone(double longitude);

        // UTMゾーンをEPSGコードに変換する関数.
        std::string utm_zone_to_epsg(int utm_zone);
        
    protected:

    public:

        // 緯度経度からUTMゾーンとUTM座標を求める関数.
        Eigen::Vector3d get_xyz_from_latlonalt(LatLonAlt latlonalt);
        Pose get_xyz_from_latlonalt(LLAWithOrientation latlonalt);

        // UTMゾーンとUTM座標から緯度経度を求める関数.
        LatLonAlt get_latlonalt_from_xyz(Eigen::Vector3d xyz);
        LLAWithOrientation get_latlonalt_from_xyz(Pose xyz);

        void set_epsg_code(int epsg_code_num);
        void set_origin( LatLonAlt orig_pose_, Eigen::Vector4d orig_quat_ );

        LLAXYZTrans();
        ~LLAXYZTrans();

    };
}


#endif
