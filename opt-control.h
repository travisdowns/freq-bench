
/*
 * "sinking" a value instructs the compiler to calculate it, i.e.,
 * makes the compiler believe that the value is necessary and hence
 * must the calculated. The actual sink implementation is empty and
 * so usually leaves no trace in the generated code except that the
 * value will be calculated.
 */
static inline void sink(int x) {
    __asm__ volatile ("" :: "r"(x) :);
}

/*
 * Similar to sink except that it sinks the content pointed to 
 * by the pointer, so the compiler will materialize in memory
 * anything pointed to by the pointer.
 */
static inline void sink_ptr(void *p) {
    __asm__ volatile ("" :: "r"(p) : "memory");
}
