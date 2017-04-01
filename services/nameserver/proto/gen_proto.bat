
cd ..
cd src
for /R %%s in (*.pb.*) do (
echo %%s
rm %%s
) 

cd ..
cd proto

for /R %%s in (*.proto) do (
echo %%s
..\..\..\deps\protobuf\cmake\build\solution\Debug\protoc.exe --proto_path="%cd%" --cpp_out="%cd%" "%%s"
) 


for /R %%s in (*.pb.*) do (
echo %%s
copy %%s ..\src\
) 


for /R %%s in (*.pb.*) do (
echo %%s
rm %%s
) 