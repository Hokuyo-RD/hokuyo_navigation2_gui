@echo off

if not exist vsvars32.bat goto end

CALL vsvars32.bat

REM Compile URG library

cl.exe -c -MD -I../include/c ../src/urg_sensor.c
cl.exe -c -MD -I../include/c ../src/urg_utils.c
cl.exe -c -MD -I../include/c ../src/urg_connection.c
cl.exe -c -MD -I../include/c ../src/urg_serial.c
cl.exe -c -MD -I../include/c ../src/urg_serial_utils.c
cl.exe -c -MD -I../include/c ../src/urg_tcpclient.c
cl.exe -c -MD -I../include/c ../src/urg_ring_buffer.c
cl.exe -c -MD -I../include/c ../src/urg_debug.c
cl.exe -c -MD -I../include/c ../src/safety_crc.c
cl.exe -c -MD -I../include/c ../src/safety_sensor_safetyCMD.c
lib.exe /OUT:urg.lib urg_sensor.obj urg_utils.obj urg_connection.obj urg_serial.obj urg_serial_utils.obj urg_tcpclient.obj urg_ring_buffer.obj urg_debug.obj safety_crc.obj safety_sensor_safetyCMD.obj

REM Compile samples linking with ws2_32.lib setupapi.lib with /MD option.

cl.exe /MD -I../include/c ../samples/c/get_area_safetyCMD.c ws2_32.lib setupapi.lib urg.lib

cl.exe /MD -I../include/c ../samples/c/get_status_safetyCMD.c ws2_32.lib setupapi.lib urg.lib

cl.exe /MD -I../include/c ../samples/c/get_distance_intensity_continuous_safetyCMD.c ws2_32.lib setupapi.lib urg.lib

cl.exe /MD -I../include/c ../samples/c/get_distance_continuous_safetyCMD.c ws2_32.lib setupapi.lib urg.lib

cl.exe /MD -I../include/c ../samples/c/get_distance_intensity_safetyCMD.c ws2_32.lib setupapi.lib urg.lib

cl.exe /MD -I../include/c ../samples/c/get_distance_safetyCMD.c ws2_32.lib setupapi.lib urg.lib

cl.exe /MD -I../include/c ../samples/c/get_full_step_distance_safetyCMD.c ws2_32.lib setupapi.lib urg.lib

cl.exe /MD -I../include/c ../samples/c/get_full_step_distance_continuous_safetyCMD.c ws2_32.lib setupapi.lib urg.lib

cl.exe /MD -I../include/c ../samples/c/sensor_parameter_safetyCMD.c ws2_32.lib setupapi.lib urg.lib

cl.exe /MD -I../include/c ../samples/c/get_detection_log_safetyCMD.c ws2_32.lib setupapi.lib urg.lib

cl.exe /MD -I../include/c ../samples/c/clear_detection_log_safetyCMD.c ws2_32.lib setupapi.lib urg.lib

echo ビルドが完了しました。終了します。
echo Built completed successfully.
set /p TMP=""
exit /b

:end
echo vsvars32.bat が見つかりません。終了します。
echo Error: vsvars32.bat could not be found
set /p TMP=""
exit /b
