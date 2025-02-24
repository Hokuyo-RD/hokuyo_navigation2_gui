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

namespace latlon_utm_trans{

    // 緯度経度を表す構造体.
    struct LatLon {
        double latitude;
        double longitude;
    };

    // UTMゾーンとUTM座標を表す構造体.
    struct UTM {
        int zone;
        double x;
        double y;
    };


    class LatlonUtmTrans {
    private:

        bool use_first_zone;

        // 経度からUTMゾーンを取得する関数.
        int judge_utm_zone(double longitude);

        // UTMゾーンをEPSGコードに変換する関数.
        std::string utm_zone_to_epsg(int utm_zone);
        
    protected:
    
        int first_zone;
    

    public:

        // 緯度経度からUTMゾーンとUTM座標を求める関数.
        UTM get_utm_from_latlon(LatLon latlon);

        // UTMゾーンとUTM座標から緯度経度を求める関数.
        LatLon get_latlon_from_utm(UTM utm);

        LatlonUtmTrans(bool zone_is_static);
        ~LatlonUtmTrans();

    };
}


#endif
