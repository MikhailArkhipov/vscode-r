Create a folder ./bin under R-Host directory

Compiling:
g++ -std=c++14 -fexceptions -fpermissive -O0 -ggdb -I../src -I../lib/picojson -I/usr/share/R/include -c ../src/*.c* &> build.log

Linking:
g++ -g -o Microsoft.R.Host.out ./*.o -lpthread -L/usr/lib/x86_64-linux-gnu -lboost_filesystem -lboost_atomic -lboost_chrono -lboost_system -lboost_program_options -lzip -L/usr/lib/R/lib -lR
g++ -g -o Microsoft.R.Host.out ./*.o -lpthread -L/usr/lib/x86_64-linux-gnu -lboost_filesystem -lboost_atomic -lboost_chrono -lboost_system -lboost_program_options -lzip -L/usr/lib64/microsoft-r/3.3/lib64/R/lib -lR -lRblas