#include <iostream>
#include <string>
#include <ConfLoad.h>

using namespace std;

ConfLoad* ConfLoad::conf_load_= nullptr;;
string ConfLoad::conf_dir_ = "/Volumes/WD_BLACK/Coding/Frame/conf/framejk.conf";

int main() {
    ConfLoad* conf_ = ConfLoad::GetInstance();

    return 0;
}