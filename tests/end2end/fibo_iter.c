int main() {
    int a = 0;
    int b = 1;

    int n = 100;
    for (int i = 0; i < n; i++)
    {
        int tmp = b;
        b = a + b;
        a = b;
    }

    return b;
}
