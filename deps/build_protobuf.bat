::�ο����� https://github.com/google/protobuf/blob/master/cmake/README.md
::Ĭ�ϵ�ǰ����ϵͳ�Ѱ�װ git �� cmake,�����ú��˻�������
echo off & color 0A

::��������Ҫ��Protobuf�汾,���°汾������github�ϲ鵽 https://github.com/google/protobuf
::���������صİ汾һ��
set PROTOBUF_VESION="3.2.0"
echo %PROTOBUF_VESION%
set PROTOBUF_PATH="protobuf"
echo %PROTOBUF_PATH%
cd %PROTOBUF_PATH%

::����VS���߼�,�൱��ָ��VS�汾,ȡ����VS�İ�װ·��
set VS_DEV_CMD="C:\Program Files (x86)\Microsoft Visual Studio 14.0\Common7\Tools\VsDevCmd.bat"
::���ù����ļ�������,�������ֲ�ͬ��VS�汾
set BUILD_PATH="build_VS2015"
::���ñ���汾 Debug Or Release
set MODE="Release"


cd cmake
if not exist %BUILD_PATH% md %BUILD_PATH%

cd %BUILD_PATH%
if not exist %MODE% md %MODE%
cd %MODE%

::��ʼ�����ͱ���
call %VS_DEV_CMD%
cmake ../../ -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=%MODE%
call extract_includes.bat
nmake /f Makefile

echo %cd%
pause