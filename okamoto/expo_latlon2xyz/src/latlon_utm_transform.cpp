#define DEBUG_MODE
#include <latlon_utm_transform.h>

using namespace latlon_utm_trans;

int LatlonUtmTrans::judge_utm_zone(double longitude){
    //経度を6で割って切り上げ.
    int zone = (int)(longitude + 180.0 + 5 )/6;
    return zone;
}

std::string LatlonUtmTrans::utm_zone_to_epsg(int utm_zone){
    int epsg_num = utm_zone + 32600;
    return "EPSG:" + std::to_string(epsg_num);
}

UTM LatlonUtmTrans::get_utm_from_latlon(LatLon latlon){
    std::string latlon_debug_msg = "get_latlon";
    DEBUG_PRINT(latlon_debug_msg);

    UTM utm;
    
    int utm_zone;
    std::string epsg_code;
    PJ_CONTEXT *C;
    PJ *P;
    PJ *norm;
    PJ_COORD a, b;
    if(use_first_zone && first_zone > 0 && first_zone <=60){
        utm_zone = first_zone;
    }
    else{
        utm_zone = judge_utm_zone(latlon.longitude);
        first_zone = utm_zone;
    }
    epsg_code = utm_zone_to_epsg(utm_zone);

    DEBUG_PRINT(utm_zone);
    DEBUG_PRINT(epsg_code);
    DEBUG_PRINT(latlon.latitude);
    DEBUG_PRINT(latlon.longitude);

    C = proj_context_create();
    P = proj_create_crs_to_crs( C, "EPSG:4326", epsg_code.c_str() , NULL);
    if (0 == P) {
        double inf = std::numeric_limits<double>::infinity();
        utm.x = inf;
        utm.y = inf;
        utm.zone = -1;
    }
    else{
        a = proj_coord(latlon.latitude, latlon.longitude, 0, 0);
        b = proj_trans(P, PJ_FWD, a);

        utm.x = b.xy.x;
        utm.y = b.xy.y;
        utm.zone = utm_zone;
    }
    
    proj_destroy(P);
    proj_context_destroy(C);

    latlon_debug_msg = "return utm from latlon";
    DEBUG_PRINT(latlon_debug_msg);

    return utm;
}

LatLon LatlonUtmTrans::get_latlon_from_utm(UTM utm){
    std::string latlon_debug_msg = "get_utm";
    DEBUG_PRINT(latlon_debug_msg);

    LatLon latlon;

    std::string epsg_code;
    PJ_CONTEXT *C;
    PJ *P;
    PJ *norm;
    PJ_COORD a, b;

    if( first_zone <= 0 || first_zone > 60){
        first_zone = utm.zone;
    }
    epsg_code = utm_zone_to_epsg(utm.zone);
    C = proj_context_create();
    P = proj_create_crs_to_crs( C, epsg_code.c_str() , "EPSG:4326", NULL);

    DEBUG_PRINT(utm.zone);
    DEBUG_PRINT(epsg_code);
    DEBUG_PRINT(utm.x);
    DEBUG_PRINT(utm.y);

    if (0 == P) {
        double inf = std::numeric_limits<double>::infinity();
        latlon.latitude = inf;
        latlon.longitude = inf;
    }
    else{
        a = proj_coord(utm.x, utm.y, 0, 0);
        b = proj_trans(P, PJ_FWD, a);

        latlon.latitude = b.xy.x;
        latlon.longitude = b.xy.y;
    }

    proj_destroy(P);
    proj_context_destroy(C);

    latlon_debug_msg = "return latlon from utm";
    DEBUG_PRINT(latlon_debug_msg);

    return latlon;
}


// 初期化処理.
LatlonUtmTrans::LatlonUtmTrans(bool zone_is_static) {
    use_first_zone = zone_is_static;
    first_zone = -1;
}

LatlonUtmTrans::~LatlonUtmTrans() {
}
