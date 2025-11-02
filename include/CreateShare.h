//
// Created by Yu Xin on 2025/11/2.
//

#ifndef FRAME_CREATESHARE_H
#define FRAME_CREATESHARE_H

#include <string>
#include <Logger.h>
#include <Tool.h>
#include <sys/mman.h>
#include <sys/shm.h>

using namespace std;
// 程序控制的共享内存
struct CtrlShared {
    bool running;
    char config[1024];
    CtrlShared():running(false), config("") {}
};

/* 权限位
* 0666：所有用户可读写
* 0644：所有者可读写，其他用户只读
* 0600：仅所有者可读写
 */
enum SharedAuthority {
    OWN_RW_OTH_RW = 0666,
    OWN_RW_OTH_R = 0644,
    OWN_RW_OTH_N = 0600,
};

template<typename T>
class CreateShare {
public:
    explicit CreateShare(
            int share_data_id,
            const SharedAuthority share_data_mode = OWN_RW_OTH_RW,
            string share_path = "/tmp"):
        share_path_(std::move(share_path)),
        share_data_id_(share_data_id),
        share_data_mode_(share_data_mode),
        data_(new T){
        if (ensureFileExists(share_path_)) {
            LOG(ERROR) << "Failed to create file [" << share_path_ << "]: " << strerror(errno);
            return;
        }
        // 1. 生成唯一key
        key_ = ftok(share_path_.c_str(), static_cast<long long>(share_data_id_));
        if (key_ == -1) {
            LOG(ERROR) << "ftok ["<< share_path_ << ":" << share_data_id_ << "] failed"<< strerror(errno);
            return;
        }
        shmid_ = shmget(key_, sizeof(T), IPC_CREAT | share_data_mode_);
        if (shmid_ == -1) {
            LOG(ERROR) << "shmget ["<< share_path_ << ":" << share_data_id_ << "] failed"<< strerror(errno);
        }
        // 附加到进程空间
        data_ = static_cast<T *>(shmat(shmid_, nullptr, 0));
        if (data_ ==  (T*)-1) {
            LOG(ERROR) << "shmat ["<< share_path_ << ":" << share_data_id_ << "] failed"<< strerror(errno);
        }
    }

    ~CreateShare() {
        if (data_ != (T*)-1) {
            shmdt(data_);
        }
    }

    T* get() { return data_; }
    void remove() const { shmctl(shmid_, IPC_RMID, nullptr); }
    [[nodiscard]] int getId() const { return shmid_; }
    [[nodiscard]] key_t getKey() const { return key_; }

private:
    string share_path_;
    size_t share_data_id_;
    SharedAuthority share_data_mode_;
    key_t key_;
    int shmid_;
    T* data_;
};

#endif //FRAME_CREATESHARE_H