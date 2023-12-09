#include <string>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <tree-sitter>

std::string * get_file_order() {
    std::string ret[];
    // find FROM tables, JOINed tables

    // build adjacency matrix
    /*     Pulls from
          A B C D E F G
        ------------------
       A|   - -   -      |
       B|-    -          |
       C|         - -    |
       D|-    -          |
       E|     - - -      |
       F|-        -      |
       G|-          -    |
        ------------------
    */

    return ret;
}

std::string open_sqls() {
    std::string ret;
    // open all SQL files into a buffer. Hideously inefficient, but we'll small atm
    DIR* cwd = opendir(".");
    while(struct dirent* e = readdir(cwd)) {
        std::string a = std::string(e->d_name);
        if(a.find(".sql") != std::string::npos) {
            printf("Looking in 'sql' file %s\n", e->d_name);
            std::ifstream fd;
            fd.open(e->d_name);
            std::string new_ret( (std::istreambuf_iterator<char>(fd) ),
                                 (std::istreambuf_iterator<char>()    ) );
            ret.append(new_ret);
            fd.close();
        }
    }
    return ret;
}

int main() {
    std::string all_sqls = open_sqls();
    std::cout << all_sqls;
}
