#include <string>
#include <ConfLoad.h>
#include <InitConf.h>
#include <Logger.h>
#include <CreateShare.h>
#include <TransCtx.h>
#include "TaskQueue.h"
#include "ThreadPool.h"
#include <iostream>
#include <CommentCtrl.h>

using namespace std;

ConfLoad* ConfLoad::conf_load_= nullptr;;
string ConfLoad::conf_dir_ = "/Volumes/WD_BLACK/Coding/Frame/conf/framejk.conf";
SpdLogger* SpdLogger::instance = nullptr;
string tmp_path = "/Volumes/WD_BLACK/Coding/Frame/tmp/share_data";

int main(const int argc, char** argv) {
    //
    InitConf();
    // 创建共享内存实例
    CreateShare<CtrlShared> shm1(1, OWN_RW_OTH_RW, tmp_path);
    CreateShare<CtrlShared> shm2(2, OWN_RW_OTH_RW, tmp_path);

    CtrlShared* data1 = shm1.get();
    data1->running = true;
    cout << data1->running << endl;
    data1->running = false;
    cout << data1->running << endl;
    CtrlShared* data2 = shm1.get();
    data2 = shm2.get();
    data2->running = true;
    cout << data2->running << endl;
    data2->running = false;
    cout << data2->running << endl;

    // if (argc < 2) {
    //     std::cout << "Usage: " << argv[0] << " [-start|-stop|-set key value]" << std::endl;
    //     return 1;
    // }


    return 0;
}