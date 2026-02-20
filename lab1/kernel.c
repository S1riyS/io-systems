typedef unsigned int uint32_t;
typedef int int32_t;

extern char __bss[], __bss_end[], __stack_top[];

__attribute__((section(".text.boot"))) __attribute__((naked)) void boot(void) {
  __asm__ __volatile__("la sp, __stack_top\n"
                       "j main\n");
}

struct sbiret {
  long error;
  union {
    long value;
    unsigned long uvalue;
  };
};

static struct sbiret sbi_call(long arg0, long arg1, long arg2, long arg3,
                               long arg4, long arg5, long fid, long eid) {
  register long a0 __asm__("a0") = arg0;
  register long a1 __asm__("a1") = arg1;
  register long a2 __asm__("a2") = arg2;
  register long a3 __asm__("a3") = arg3;
  register long a4 __asm__("a4") = arg4;
  register long a5 __asm__("a5") = arg5;
  register long a6 __asm__("a6") = fid;
  register long a7 __asm__("a7") = eid;

  __asm__ __volatile__("ecall"
                       : "+r"(a0), "+r"(a1)
                       : "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7)
                       : "memory");

  struct sbiret ret;
  ret.error = a0;
  ret.value = a1;
  return ret;
}

void putchar(int ch) { sbi_call(ch, 0, 0, 0, 0, 0, 0, 0x01); }

int getchar(void) {
  struct sbiret ret;
  do {
    ret = sbi_call(0, 0, 0, 0, 0, 0, 0, 0x02);
  } while (ret.error < 0);
  return (int)ret.error;
}

void puts(const char *s) {
  while (*s) {
    if (*s == '\n')
      putchar('\r');
    putchar(*s++);
  }
}

void main(void) {
  int ch = getchar();
  puts("Got char: ");
  putchar(ch);
  putchar('\r');
  putchar('\n');
}
