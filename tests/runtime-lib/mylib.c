void __builtin_fill_zero(void* dest, int count) {
  for (int i = 0; i < count; ++i) {
    *((int*)dest + i) = 0;
  }
}