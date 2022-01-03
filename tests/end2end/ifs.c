int main() {
    int x = 5;
    int ret = 0;
    if (x - 1 == 4) {
        ret = ret + 1;
        x = ret + 3;
    }
    if (x == 4 ) {
        ret = ret + 1;
    }

    return ret;
}