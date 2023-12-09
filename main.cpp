#include <string>
#include <stdio.h>

class Query {
    public:
        std::string db_conn_string;
        std::string body;
        void eval();
};

class Node {
    public:
        Node * child;
        Query inverse(int x, int y, std::string hexcode);
};

class Positional: public Node {
    public:
        Node first;
        Node second;
        int width; int height;
        enum {HORIZONTAL, VERTICAL} type;

        Query inverse(int x, int y, std::string hexcode) {
            if (this->type == HORIZONTAL) {
                if (x < this->width / 2)
                    return this->first.inverse(x, y, hexcode);
                else
                    return this->second.inverse(x, y, hexcode);
            } else if (this->type == VERTICAL) {
                if (y < this->height / 2)
                    return this->first.inverse(x, y, hexcode);
                else
                    return this->second.inverse(x, y, hexcode);
            }
            printf("failed proof by parts\n");
            exit(1); 
        }
};

class Plot: public Node {
    public:
        Query inverse(int x, int y, std::string hexcode);
};

class HConcat: public Plot {
    public:
        Query inverse(int x, int y, std::string hexcode);
};


Node root = Node();

int main() {
    bool done = false;
    while(!done) {
        if(click) {
            root.inverse(m.x, m.y);
        }
    }
    return 1;
}
