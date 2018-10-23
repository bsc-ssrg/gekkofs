/* Test fs functionality involving links */

#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cassert>
#include <sys/stat.h>

int main(int argc, char* argv[]) {

    const std::string mountdir = "/tmp/mountdir";
    const std::string dir_int = mountdir + "/dir";
    const std::string source_int = dir_int + "/source";
    const std::string target_ext = "/tmp/target";
    const std::string target_int = dir_int + "/target";

    char buffIn[] = "oops.";
    char *buffOut = new char[strlen(buffIn) + 1];

    struct stat st;
    int ret;
    int fd;


    // Clean internal dir
    ret = rmdir(dir_int.c_str());
    if (ret != 0) {
        if (errno != ENOENT) {
            std::cerr << "ERROR: cannot remove internal dir: " << strerror(errno) << std::endl;
            return -1;
        }
    }
    ret = mkdir(dir_int.c_str(), 0770);
    if (ret != 0) {
        std::cerr << "ERROR: cannot create internal dir: " << strerror(errno) << std::endl;
        return -1;
    }


    /* Create target */
    fd = open(source_int.c_str(), O_WRONLY | O_CREAT, 0770);
    if(fd < 0){
        std::cerr << "ERROR: opening target for write" << strerror(errno) << std::endl;
        return -1;
    }
    auto nw = write(fd, buffIn, strlen(buffIn));
    if(nw != strlen(buffIn)){
        std::cerr << "ERROR: writing target" << strerror(errno) << std::endl;
        return -1;
    }
    if(close(fd) != 0){
        std::cerr << "ERROR: closing target" << strerror(errno) << std::endl;
        return -1;
    }


    // Move outside namespace: NOT SUPPORTED
    ret = rename(source_int.c_str(), target_ext.c_str());
    if (ret == 0) {
        std::cerr << "ERROR: Succeeded on moving file outside directory" << std::endl;
        return -1;
    }
    if(errno != ENOTSUP){
        std::cerr << "ERROR: wrong error number on not supported move: " << errno << std::endl;
        return -1;
    }

    // make sure no target has been created
    if (stat(target_ext.c_str(), &st) == 0) {
        std::cerr << "ERROR: unexpected external target found" << std::endl;
        return -1;
    }


    // Move source to target
    ret = rename(source_int.c_str(), target_int.c_str());
    if(ret != 0){
        std::cerr << "ERROR: moving source to target: " << strerror(errno) << std::endl;
        return -1;
    }
    // source needs not to exists after move
    assert(stat(source_int.c_str(), &st) != 0 && errno == ENOENT);

    // Check target stat
    ret = stat(target_int.c_str(), &st);
    if (ret != -0) {
        std::cerr << "ERROR: stating target after move: " << strerror(errno) << std::endl;
        return -1;
    }
    if (st.st_size != strlen(buffIn)) {
        std::cerr << "Wrong file size after creation: " << st.st_size << std::endl;
        return -1;
    }


    /* Try to reuse source */
    fd = open(source_int.c_str(), O_WRONLY | O_CREAT | O_EXCL);
    if(fd < 0 ){
        std::cerr << "ERROR: Succeeded on overwriting moved source: " << strerror(errno) << std::endl;
        return -1;
    }
    /* Remove source */
    ret = unlink(source_int.c_str());
    if (ret != 0) {
        std::cerr << "Error removing source: " << strerror(errno) << std::endl;
        return -1;
    };


    /* Read target back */
    fd = open(target_int.c_str(), O_RDONLY);
    if (fd < 0) {
        std::cerr << "ERROR: opening renamed file (read): " << strerror(errno) << std::endl;
        return -1;
    }
    auto nr = read(fd, buffOut, strlen(buffIn) + 1);
    if (nr != strlen(buffIn)) {
        std::cerr << "ERROR: reading renamed file: " << strerror(errno) << std::endl;
        return -1;
    }
    if (strncmp(buffIn, buffOut, strlen(buffIn)) != 0) {
        std::cerr << "ERROR: File content mismatch" << std::endl;
        return -1;
    }
    ret = close(fd);
    if (ret != 0) {
        std::cerr << "ERROR: Error closing renamed file: " << strerror(errno) << std::endl;
        return -1;
    };


    /* Remove target */
    ret = unlink(target_int.c_str());
    if (ret != 0) {
        std::cerr << "Error removing target: " << strerror(errno) << std::endl;
        return -1;
    };
    assert((stat(target_int.c_str(), &st) == -1) && (errno == ENOENT));


    // Clean test working directories
    ret = rmdir(dir_int.c_str());
    if (ret != 0) {
        std::cerr << "ERROR: cannot remove internal dir: " << strerror(errno) << std::endl;
        return -1;
    }
}
