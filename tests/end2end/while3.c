int main() {
    int s = 0;
    int i = 0;
    while (i < 10) {
        int j = 0;
        while (j < 10) {
            s = s + 1;
            j = j + 1;
        }
        i = i + 1;
    }

    return s;
}
