typedef unsigned int uint32_t;
typedef int int32_t;

extern char __bss[], __bss_end[], __stack_top[];

__attribute__((section(".text.boot"))) __attribute__((naked)) void boot(void) {
  __asm__ __volatile__("la sp, __stack_top\n"
                       "j main\n");
}

enum sbi_error {
  SBI_SUCCESS = 0,
  SBI_ERR_FAILED = -1,
  SBI_ERR_NOT_SUPPORTED = -2,
  SBI_ERR_INVALID_PARAM = -3,
  SBI_ERR_DENIED = -4,
  SBI_ERR_INVALID_ADDRESS = -5,
  SBI_ERR_ALREADY_AVAILABLE = -6,
  SBI_ERR_ALREADY_STARTED = -7,
  SBI_ERR_ALREADY_STOPPED = -8,
  SBI_ERR_NO_SHMEM = -9,
  SBI_ERR_INVALID_STATE = -10,
  SBI_ERR_BAD_RANGE = -11,
  SBI_ERR_TIMEOUT = -12,
  SBI_ERR_IO = -13,
  SBI_ERR_DENIED_LOCKED = -14,
};

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

void print_hex(unsigned long val) {
  puts("0x");
  int started = 0;
  for (int i = 28; i >= 0; i -= 4) {
    int digit = (val >> i) & 0xF;
    if (digit || started || i == 0) {
      putchar(digit < 10 ? '0' + digit : 'a' + digit - 10);
      started = 1;
    }
  }
}

void print_dec(long val) {
  if (val < 0) {
    putchar('-');
    val = -val;
  }
  char buf[12];
  int i = 0;
  do {
    buf[i++] = '0' + (val % 10);
    val /= 10;
  } while (val > 0);
  while (i > 0)
    putchar(buf[--i]);
}

int read_int(void) {
  int result = 0;
  int got_digit = 0;
  for (;;) {
    int ch = getchar();
    if (ch >= '0' && ch <= '9') {
      putchar(ch);
      result = result * 10 + (ch - '0');
      got_digit = 1;
    } else if ((ch == '\r' || ch == '\n') && got_digit) {
      putchar('\r');
      putchar('\n');
      return result;
    }
  }
}

static const char *sbi_err_str(long err) {
  switch (err) {
  case SBI_SUCCESS:
    return "SBI_SUCCESS";
  case SBI_ERR_FAILED:
    return "SBI_ERR_FAILED";
  case SBI_ERR_NOT_SUPPORTED:
    return "SBI_ERR_NOT_SUPPORTED";
  case SBI_ERR_INVALID_PARAM:
    return "SBI_ERR_INVALID_PARAM";
  case SBI_ERR_DENIED:
    return "SBI_ERR_DENIED";
  case SBI_ERR_INVALID_ADDRESS:
    return "SBI_ERR_INVALID_ADDRESS";
  case SBI_ERR_ALREADY_AVAILABLE:
    return "SBI_ERR_ALREADY_AVAILABLE";
  case SBI_ERR_ALREADY_STARTED:
    return "SBI_ERR_ALREADY_STARTED";
  case SBI_ERR_ALREADY_STOPPED:
    return "SBI_ERR_ALREADY_STOPPED";
  case SBI_ERR_NO_SHMEM:
    return "SBI_ERR_NO_SHMEM";
  case SBI_ERR_INVALID_STATE:
    return "SBI_ERR_INVALID_STATE";
  case SBI_ERR_BAD_RANGE:
    return "SBI_ERR_BAD_RANGE";
  case SBI_ERR_TIMEOUT:
    return "SBI_ERR_TIMEOUT";
  case SBI_ERR_IO:
    return "SBI_ERR_IO";
  case SBI_ERR_DENIED_LOCKED:
    return "SBI_ERR_DENIED_LOCKED";
  default:
    return "UNKNOWN";
  }
}

/* EID=0x10 Base Extension, FID=2 sbi_get_impl_version */
void cmd_get_impl_version(void) {
  struct sbiret ret = sbi_call(0, 0, 0, 0, 0, 0, 2, 0x10);
  puts("SBI implementation version: ");
  print_dec(ret.value);
  puts(" (");
  print_hex(ret.uvalue);
  puts(")\n");
}

/* EID=0x48534D (HSM), FID=2 sbi_hart_get_status */
void cmd_hart_get_status(void) {
  puts("Enter hart id: ");
  int hartid = read_int();
  struct sbiret ret = sbi_call(hartid, 0, 0, 0, 0, 0, 2, 0x48534D);
  if (ret.error != SBI_SUCCESS) {
    puts("Error: ");
    puts(sbi_err_str(ret.error));
    putchar('\n');
    return;
  }
  puts("Hart ");
  print_dec(hartid);
  puts(" status: ");
  switch (ret.value) {
  case 0:
    puts("STARTED");
    break;
  case 1:
    puts("STOPPED");
    break;
  case 2:
    puts("START_PENDING");
    break;
  case 3:
    puts("STOP_PENDING");
    break;
  case 4:
    puts("SUSPENDED");
    break;
  case 5:
    puts("SUSPEND_PENDING");
    break;
  case 6:
    puts("RESUME_PENDING");
    break;
  default:
    puts("UNKNOWN (");
    print_dec(ret.value);
    putchar(')');
  }
  putchar('\n');
}

/* EID=0x48534D (HSM), FID=1 sbi_hart_stop */
void cmd_hart_stop(void) {
  puts("Stopping current hart...\n");
  struct sbiret ret = sbi_call(0, 0, 0, 0, 0, 0, 1, 0x48534D);
  puts("Error: ");
  puts(sbi_err_str(ret.error));
  putchar('\n');
}

/* EID=0x53525354 (SRST), FID=0 sbi_system_reset, type=0 (shutdown) */
void cmd_shutdown(void) {
  puts("System shutdown...\n");
  sbi_call(0, 0, 0, 0, 0, 0, 0, 0x53525354);
  puts("Shutdown failed!\n");
}

void print_menu(void) {
  puts("\nMenu:\n");
  puts("1. Get SBI implementation version\n");
  puts("2. Hart get status\n");
  puts("3. Hart stop\n");
  puts("4. System Shutdown\n");
  puts("Select option: ");
}

void main(void) {
  puts("\nRISC-V Bare-metal IO Lab 1\n");

  for (;;) {
    print_menu();
    int ch = getchar();
    putchar(ch);
    putchar('\r');
    putchar('\n');

    switch (ch) {
    case '1':
      cmd_get_impl_version();
      break;
    case '2':
      cmd_hart_get_status();
      break;
    case '3':
      cmd_hart_stop();
      break;
    case '4':
      cmd_shutdown();
      break;
    default:
      puts("Unknown option\n");
    }
  }
}
