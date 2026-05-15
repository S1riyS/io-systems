#set document(title: "ЛР1 Принципы организации ввода/вывода без ОС")
#set page(paper: "a4", margin: (top: 2cm, bottom: 2cm, left: 2cm, right: 2cm))

#set text(lang: "ru", font: "Liberation Serif", size: 14pt)
#set par(justify: true, leading: 0.8em, first-line-indent: 0cm)
#set heading(numbering: "1.")
#show heading: set par(first-line-indent: 0pt)

#show raw: set text(font: "Liberation Mono", size: 11pt)
#let source(path, lang) = block(
  fill: luma(240),
  stroke: 0.5pt + luma(180),
  inset: 10pt,
  radius: 4pt,
  below: 1.5em,
  raw(read(path), lang: lang, block: true),
)

#let console-shot(path, caption) = figure(
  image(path, width: 16cm),
  caption: caption,
)

// ─────────────────────────── Титульный лист ───────────────────────────

#align(center)[
  #set par(first-line-indent: 0pt)
  #text(size: 12pt)[
    Федеральное государственное автономное образовательное учреждение\
    высшего образования\
    *«Национальный исследовательский университет ИТМО»*
  ]

  #v(0.4cm)
  #text(size: 12pt)[Факультет программной инженерии и компьютерной техники]

  #v(3.5cm)

  #text(size: 16pt, weight: "bold")[
    Лабораторная работа №1\
    «Принципы организации ввода/вывода \ без операционной системы»
  ]

  #v(0.6cm)
  #text(size: 13pt)[по дисциплине «Системы ввода-вывода»]
  #v(0.4cm)
  #text(size: 13pt)[Вариант: 2]

  #v(5cm)

  #align(right)[
    #set par(first-line-indent: 0pt)
    #grid(
      columns: 10cm,
      row-gutter: 0.5em,
      align: (right, left),
      [*Студенты:*],
      [Анкудинов Кирилл (1.2)],
      [Есев Ярослав (1.2)],
      [*Преподаватель:*],
      [Табунщик Сергей Михайлович],
    )
  ]

  #v(1fr)
  #text(size: 12pt)[Санкт-Петербург, 2026]
]

#pagebreak()

#set page(numbering: "1")
#counter(page).update(1)

#outline(
  title: [Содержание],
  indent: auto,
)

#pagebreak()

= Введение

*Цель:* познакомиться с принципами организации ввода/вывода без операционной системы на примере компьютерной системы на базе процессора с архитектурой RISC-V и интерфейсом OpenSBI с использованием эмулятора QEMU.

*Задачи:*
+ Реализовать функцию `putchar` вывода данных в консоль.
+ Реализовать функцию `getchar` для получения данных из консоли.
+ На базе реализованных функций `putchar` и `getchar` написать программу, позволяющую вызывать определённые вариантом функции OpenSBI посредством взаимодействия пользователя через меню:
  a. 1 — Get SBI implementation version
  b. 2 — Hart get status
  c. 3 — Hart stop
  d. System Shutdown

#pagebreak()

= Ход выполнения

Для всех реализованных функций будем переиспользовать функцию `sbi_call`:

#block(
  fill: luma(240),
  stroke: 0.5pt + luma(180),
  inset: 10pt,
  radius: 4pt,
  below: 1.5em,
)[
  ```c
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
  ```
]

== putchar

#block(
  fill: luma(240),
  stroke: 0.5pt + luma(180),
  inset: 10pt,
  radius: 4pt,
  below: 1.5em,
)[
  ```c
  void putchar(int ch) { sbi_call(ch, 0, 0, 0, 0, 0, 0, 0x01); }
  ```
]

Функция осуществляет вывод одиночного символа в консоль. Реализована через обёртку `sbi_call`, которая вызывает расширение Console Putchar (EID `0x01`).

#pagebreak()

== getchar

#block(
  fill: luma(240),
  stroke: 0.5pt + luma(180),
  inset: 10pt,
  radius: 4pt,
  below: 1.5em,
)[
  ```c
  int getchar(void) {
    struct sbiret ret;
    do {
      ret = sbi_call(0, 0, 0, 0, 0, 0, 0, 0x02);
    } while (ret.error < 0);
    return (int)ret.error;
  }
  ```
]

Функция осуществляет чтение одиночного символа с устройства ввода. Функция работает в режиме блокирующего ожидания (цикл `do-while`), вызывая расширение Console Getchar (EID `0x02`) до тех пор, пока в регистре возврата не появится код символа (значение, не меньшее нуля). Возвращает считанный байт в формате `int`.

== Функции по варианту

=== Get SBI implementation version

#block(
  fill: luma(240),
  stroke: 0.5pt + luma(180),
  inset: 10pt,
  radius: 4pt,
  below: 1.5em,
)[
  ```c
  void cmd_get_impl_version(void) {
    struct sbiret ret = sbi_call(0, 0, 0, 0, 0, 0, 2, 0x10);
    puts("SBI implementation version: ");
    print_dec(ret.value);
    puts(" (");
    print_hex(ret.uvalue);
    puts(")\n");
  }
  ```
]

Запрашивает версию реализации SBI, поддерживаемую текущим окружением. Использует базовое расширение (EID `0x10`, FID `2`). Полученное значение из регистра `a1` выводится в десятичном и шестнадцатеричном виде.

#pagebreak()

=== Hart get status

#block(
  fill: luma(240),
  stroke: 0.5pt + luma(180),
  inset: 10pt,
  radius: 4pt,
  below: 1.5em,
)[
  ```c
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
    case 0: puts("STARTED"); break;
    case 1: puts("STOPPED"); break;
    /* ... */
    }
    putchar('\n');
  }
  ```
]

Запрашивает статус указанного аппаратного потока (hart). Для ввода идентификатора hart применяется вспомогательная функция `read_int`. Использует расширение HSM (EID `0x48534D`, FID `2`). Возможные значения статуса: `STARTED`, `STOPPED`, `START_PENDING`, `STOP_PENDING`, `SUSPENDED`, `SUSPEND_PENDING`, `RESUME_PENDING`.

=== Hart stop

#block(
  fill: luma(240),
  stroke: 0.5pt + luma(180),
  inset: 10pt,
  radius: 4pt,
  below: 1.5em,
)[
  ```c
  void cmd_hart_stop(void) {
    puts("Stopping current hart...\n");
    struct sbiret ret = sbi_call(0, 0, 0, 0, 0, 0, 1, 0x48534D);
    puts("Error: ");
    puts(sbi_err_str(ret.error));
    putchar('\n');
  }
  ```
]

Останавливает текущий hart. Использует расширение HSM (EID `0x48534D`, FID `1`). При успешном вызове управление не возвращается в программу; в противном случае выводится код ошибки SBI.

=== System Shutdown

#block(
  fill: luma(240),
  stroke: 0.5pt + luma(180),
  inset: 10pt,
  radius: 4pt,
  below: 1.5em,
)[
  ```c
  void cmd_shutdown(void) {
    puts("System shutdown...\n");
    sbi_call(0, 0, 0, 0, 0, 0, 0, 0x53525354);
    puts("Shutdown failed!\n");
  }
  ```
]

Завершает работу виртуальной машины при помощи системного вызова расширения System Reset (EID `0x53525354`, FID `0`, тип сброса — shutdown). Управление после успешного вызова не возвращается; при неудаче выводится сообщение об ошибке.

#pagebreak()

= Результаты работы программы

Ниже приведены скриншоты вывода в консоль при вызове каждого пункта меню.

== Get SBI implementation version

#console-shot(
  "assets/option_1.png",
  [Пункт меню 1 — Get SBI implementation version],
)

== Hart get status

#console-shot(
  "assets/option_2_valid.png",
  [Пункт меню 2 — Hart get status (hart id = 0)],
)

#console-shot(
  "assets/option_2_invalid.png",
  [Пункт меню 2 — Hart get status (некорректный hart id)],
)

== Hart stop

#console-shot(
  "assets/option_3.png",
  [Пункт меню 3 — Hart stop],
)

== System Shutdown

#console-shot(
  "assets/option_4.png",
  [Пункт меню 4 — System Shutdown],
)
