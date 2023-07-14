int main ()
{
    int v = 1;
    int count = 0;

    v = __atomic_load_n (&v, __ATOMIC_RELAXED);
    if (v != ++count)
       return 1;
    __atomic_store_n (&v, count + 1, __ATOMIC_RELAXED);
    if (v != ++count)
        return 2;

    __atomic_store_n (&v, count + 1, __ATOMIC_RELEASE);
    if (v != ++count)
        return 3;

    __atomic_store_n (&v, count + 1, __ATOMIC_SEQ_CST);
    if (v != ++count)
        return 4;
    return 0;
}

