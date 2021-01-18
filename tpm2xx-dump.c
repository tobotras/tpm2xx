#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <modbus.h>
#include <iconv.h>

int verbose = 0;
unsigned int current_value;
struct {
  int number;
  char *name;
  char *comment;
  enum {
        GROUP,
        HEX,
        INT16,
        UINT16,
        CHAR8,
        FLOAT32
  } type;
} registers[] =
  {
   { 0, NULL, "Группа LvoP. Оперативные параметры" },  
   { 0x0000, "STAT", "Регистр статуса", HEX },
   { 0x0001, "PV1", "Измеренная величина на входе 1", INT16 },
   { 0x0002, "PV2", "Измеренная величина на входе 2", INT16 },
   { 0x0003, "LUPV", "Значение на выходе вычислителя", INT16 },
   { 0x0004, "SP", "Уставка регулятора", INT16 },
   { 0x0005, "SET.P", "Текущее значение уставки работающего регулятора",  INT16 },
   { 0x0006, "O", "Выходная мощность ПИД-регулятора (положение задвижки)", UINT16 },

   { 0, NULL, "Группа LvoP. Рабочие параметры прибора" },
   { 0x0007, "r-L", "Переход на внешнее управление", UINT16 },
   { 0x0008, "r.out", "Выходной сигнал регулятора", INT16 },
   { 0x0009, "R-S", "Запуск/остановка регулирования", UINT16 },
   { 0x000A, "AT", "Запуск/остановка процесса автонастройки", UINT16 },

   { 0, NULL, "Группа LvoP. Оперативные параметры прибора" },
   { 0x1000, "DEV", "Тип прибора", CHAR8 },
   { 0x1004, "VER", "Версия прибора", CHAR8 },
   { 0x1008, "STAT", "Регистр статуса", HEX },
   { 0x1009, "PV1", "Измеренная величина на входе 1", FLOAT32 },
   { 0x100B, "PV2", "Измеренная величина на входе 2", FLOAT32 },
   { 0x100D, "LUPV", "Значение на выходе вычислителя", FLOAT32 },
   { 0x100F, "SP", "Уставка регулятора", FLOAT32 },
   { 0x1011, "SET.P", "Текущее значение уставки работающего регулятора",  FLOAT32 },
   { 0x1013, "O", "Выходная мощность ПИД-регулятора (положение задвижки)", FLOAT32 },

   /* Группа Comm. Параметры обмена */
   /* если с ней что-то не так, то всё равно ничего не прочитается :) */

   { 0, NULL, "Группа init. Параметры входов" },
   { 0x0200, "in.t1", "Тип входного датчика или сигнала для входа 1", UINT16 },
   { 0x0201, "dPt1", "Точность вывода температуры на входе 1", UINT16 },
   { 0x0202, "dP1", "Положение десятичной точки для входа 1", UINT16 },
   { 0x0203, "in.L1", "Нижняя граница диапазона измерения для входа 1", INT16 },
   { 0x0204, "in.H1", "Верхняя граница диапазона измерения для входа 1", INT16 },
   { 0x0205, "SH1", "Сдвиг характеристики для входа 1", INT16 },
   { 0x0206, "KU1", "Наклон характеристики для входа 1", UINT16 },
   { 0x0207, "Fb1", "Полоса фильтра для входа 1", UINT16 },
   { 0x0208, "inF1", "Постоянная времени цифрового фильтра для входа 1", UINT16 },
   { 0x0209, "Sqr1", "Вычислитель квадратного корня для аналогового входа 1", UINT16 },
   { 0x020A, "in.t2", "Тип входного датчика или сигнала для входа 2", UINT16 },
   { 0x020B, "dPt2", "Точность вывода температуры на входе 2", UINT16 },
   { 0x020C, "dP2", "Положение десятичной точки для входа 2", UINT16 },
   { 0x020D, "in.L2", "Нижняя граница диапазона измерения для входа 2", INT16 },
   { 0x020E, "in.H2", "Верхняя граница диапазона измерения для входа 2", INT16 },
   { 0x020F, "SH2", "Сдвиг характеристики для входа 2", INT16 },
   { 0x0210, "KU2", "Наклон характеристики для входа 2", UINT16 },
   { 0x0211, "Fb2", "Полоса фильтра для входа 2", UINT16 },
   { 0x0212, "inF2", "Постоянная времени цифрового фильтра для входа 2", UINT16 },
   { 0x0213, "Sqr2", "Вычислитель квадратного корня для аналогового входа 2", UINT16 },

   { 0, NULL, "Группа Adv. Параметры регулирования" },
   { 0x0300, "inP2", "Функция на входе 2", UINT16 },
   { 0x0301, "CALC", "Формула вычислителя", UINT16 },
   { 0x0302, "kPV1", "Весовой коэффициент для PV1", INT16 },
   { 0x0303, "kPV2", "Весовой коэффициент для PV2", INT16 },
   { 0x0304, "SL-L", "Нижняя граница уставки", INT16 },
   { 0x0305, "SL-H", "Верхняя граница уставки", INT16 },
   { 0x0306, "orEU", "Тип управления при регулировании", UINT16 },
   { 0x0307, "PV0", "Поддерживаемая величина при мощности 0%", INT16 },
   { 0x0308, "ramP", "Режим «быстрого выхода на уставку»", UINT16 },
   { 0x0309, "P", "Полоса пропорциональности ПИД-регулятора", UINT16 },
   { 0x030A, "I", "Интегральная постоянная ПИД-регулятора", UINT16 },
   { 0x030B, "D", "Дифференциальная постоянная ПИД-регулятора", UINT16 },
   { 0x030C, "dB", "Зона нечувствительности ПИД-регулятора", UINT16 },
   { 0x030D, "vSP", "Скорость изменения уставки", UINT16 },
   { 0x030E, "OL-L", "Минимальная выходная мощность", UINT16 },
   { 0x030F, "OL-H", "Максимальная выходная мощность", UINT16 },
   { 0x030F, "OL-H", "Максимальная выходная мощность", UINT16 },
   { 0x0310, "LbA", "Время диагностики обрыва контура", UINT16 },
   { 0x0311, "LbAt", "Ширина зоны диагностики обрыва контура", UINT16 },
   { 0x0312, "MVEr", "Выходной сигнал в состоянии «ошибка»", UINT16 },
   { 0x0313, "MVSt", "Выходной сигнал в состоянии «остановка регулирования»", UINT16 },
   { 0x0314, "MdSt", "Состояние выхода  в состоянии «остановка регулирования»", UINT16 },
   { 0x0315, "Alt", "Тип логики работы компаратора", UINT16 },
   { 0x0316, "AL-d", "Порог срабатывания компаратора", UINT16 },
   { 0x0317, "AL-H", "Гистерезис компаратора", UINT16 },

   { 0, NULL, "Группа valv. Параметры задвижки" },
   { 0x0400, "v.Mot", "Полное время хода задвижки", UINT16 },
   { 0x0401, "V.db", "Зона нечувствительности задвижки", UINT16 },
   { 0x0402, "V.GAP", "Время выборки люфта задвижки", UINT16 },
   { 0x0403, "V.rEV", "Минимальное время реверса", UINT16 },
   { 0x0404, "V.tOF", "Пауза между импульсами доводки", UINT16 },

   { 0, NULL, "Группа DISP. Параметры индикации" },
   { 0x0500, "rEt", "Время выхода из режима программирования", UINT16 },
   { 0x0501, "DiS1", "Режим индикации 1", UINT16 },
   { 0x0502, "DiS2", "Режим индикации 2", UINT16 },
   { 0x0503, "DiS3", "Режим индикации 3", UINT16 },
   { 0x0504, "DiS4", "Режим индикации 4", UINT16 },
   { 0x0505, "DiS5", "Режим индикации 5", UINT16 },
   
   { 0, NULL, "Группа GraF. Параметры графика коррекции уставки" },
   { 0x0600, "Node", "Количество узловых точек графика", UINT16 },
   { 0x0601, "X1", "Значение внешнего параметра в точке 1", INT16 },
   { 0x0602, "Y1", "Корректирующее значение уставки в точке 1", INT16 },
   { 0x0603, "X2", "Значение внешнего параметра в точке 2", INT16 },
   { 0x0604, "Y2", "Корректирующее значение уставки в точке 2", INT16 },
   { 0x0605, "X3", "Значение внешнего параметра в точке 3", INT16 },
   { 0x0606, "Y3", "Корректирующее значение уставки в точке 3", INT16 },
   { 0x0607, "X4", "Значение внешнего параметра в точке 4", INT16 },
   { 0x0608, "Y4", "Корректирующее значение уставки в точке 4", INT16 },
   { 0x0609, "X5", "Значение внешнего параметра в точке 5", INT16 },
   { 0x060A, "Y5", "Корректирующее значение уставки в точке 5", INT16 },
   { 0x060B, "X6", "Значение внешнего параметра в точке 6", INT16 },
   { 0x060C, "Y6", "Корректирующее значение уставки в точке 6", INT16 },
   { 0x060D, "X7", "Значение внешнего параметра в точке 7", INT16 },
   { 0x060E, "Y7", "Корректирующее значение уставки в точке 7", INT16 },
   { 0x060F, "X8", "Значение внешнего параметра в точке 8", INT16 },
   { 0x0610, "Y8", "Корректирующее значение уставки в точке 8", INT16 },
   { 0x0611, "X9", "Значение внешнего параметра в точке 9", INT16 },
   { 0x0612, "Y9", "Корректирующее значение уставки в точке 9", INT16 },
   { 0x0613, "X10", "Значение внешнего параметра в точке 10", INT16 },
   { 0x0614, "Y10", "Корректирующее значение уставки в точке 10", INT16 },

   { 0, NULL, "Группа SECR. Параметры секретности" },
   { 0x0700, "oAPt", "Защита параметров от просмотра", UINT16 },
   { 0x0701, "wtPt", "Защита параметров от изменения", UINT16 },
   { 0x0702, "EdPt", "Защита отдельный параметров от просмотра и изменений", UINT16 },
#define MAX_REGISTER_NUMBER 0x0702
  };

uint16_t values[MAX_REGISTER_NUMBER * 2];

int data_size(int idx)
{
  if (registers[idx].name == NULL)
    return 0;
  switch (registers[idx].type) {
  case HEX:
  case INT16:
  case UINT16:
    return 1;
  case CHAR8:  
    return 4;
  case FLOAT32:
    return 2;
    break;
  default:
    printf("<UNKNOWN data type>");
    return -1;
  }
}

int read_register(modbus_t *ctx, int idx)
{
  if (registers[idx].name == NULL )
    return 0;
  
  int size = data_size(idx);
  int res;
  uint16_t data[8];
  
  res = modbus_read_registers(ctx, registers[idx].number, size, values + current_value);
  current_value += size;
  if ( res == -1 ) {
    fprintf(stderr, "Can't read register %d (0x%04x) (size %d): %s\n", idx, registers[idx].number, size, modbus_strerror(errno));
    return -1;
  }
  return 0;
}

int read_registers(modbus_t *ctx)
{
  current_value = 0;
  for (int i = 0; i < sizeof registers / sizeof registers[0]; ++i)
    read_register(ctx, i);
}

char *recode8(char *text)
{
  static char dest[30], *dst;

  bzero(dest, 30);
  dst = dest;
  size_t in = 8, out = 30;
  iconv_t cd = iconv_open("UTF-8", "CP1251");
  iconv(cd, &text, &in, &dst, &out);
  iconv_close(cd);
  return dest;
}

int dump_register(int idx)
{
  if (registers[idx].name == NULL) {
    if (verbose)
      printf( "------------ %s ------------\n", registers[idx].comment );
    return 0;
  }

  char buf[9];
  printf("%s ", registers[idx].name);
  if (verbose)
    printf("(%s)", registers[idx].comment);
  printf(": ");
  switch (registers[idx].type) {
  case HEX:
    printf("0x%x", values[current_value]);
    break;
  case INT16:
    printf("%d", (int16_t) values[current_value]);
    break;
  case UINT16:
    printf("%u", values[current_value]);
    break;
  case CHAR8:
    sprintf(buf, "%c%c%c%c%c%c%c%c",
            (char) ((values[current_value]>>8) & 0xFF), (char) (values[current_value] & 0xFF),
            (char) ((values[current_value+1]>>8) & 0xFF), (char) (values[current_value+1] & 0xFF),
            (char) ((values[current_value+2]>>8) & 0xFF), (char) (values[current_value+2] & 0xFF),
            (char) ((values[current_value+3]>>8) & 0xFF), (char) (values[current_value+3] & 0xFF));
    buf[8] = '\0';
    printf("%s", recode8(buf));
    break;
  case FLOAT32:
    printf("%.2f", (float) (((uint32_t) (values[current_value+1] << 16)) | ((uint32_t) values[current_value])));
    break;
  default:
    printf("<UNKNOWN register type>");
  }
  puts("");
  return 0;
}

int dump_registers(void)
{
  current_value = 0;
  for (int i = 0; i < sizeof registers / sizeof registers[0]; ++i) {
    dump_register(i);
    current_value += data_size(i);
  }
}

void hello(void)
{
  printf("tpm2xx-dump v0.1 (c) Boris Tobotras, 2021\n");
}

void usage(char *me)
{
  hello();
  fprintf(stderr, "Usage: %s -h hostname -p port [-t timeout] [-v] [-d]\n", me);
  exit(-1);
}

int main(int argc, char *argv[])
{
  char *host = NULL;
  char *port = NULL;
  int timeout = 10, option;
  int debug = 0;
  
  while ((option = getopt(argc, argv, "h:p:t:vd")) != -1) {
    switch (option) {
    case 't':
      timeout = atoi(optarg);
      break;
    case 'h':
      host = optarg;
      break;
    case 'p':
      port = optarg;
      break;
    case 'v':
      verbose = 1;
      break;
    case 'd':
      debug = 1;
      break;
    default:
      usage(argv[0]);
    }
  }

  if (!host || !port)
    usage(argv[0]);

  if (verbose)
    hello();
  
  modbus_t *ctx = modbus_new_tcp_pi( host, port);
  if ( ctx == NULL ) {
    fprintf(stderr, "Can't create MODBUS TCP context\n");
    return -1;
  }
  if ( modbus_connect(ctx) == -1 ) {
    fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
    return -1;
  }
  if ( modbus_set_debug(ctx, debug) == -1 ) {
    fprintf(stderr, "Can't set debug mode (huh?): %s\n", modbus_strerror(errno));
    return -1;
  }
  if ( modbus_set_response_timeout( ctx, timeout, 0 ) < 0 ) {
    fprintf(stderr, "Can't set response timeout: %s\n", modbus_strerror(errno));
    return -1;
  }

  if ( modbus_set_slave(ctx, 1) < 0 ) {
    fprintf(stderr, "Can't set slave id: %s\n", modbus_strerror(errno));
    return -1;
  }
    
  read_registers(ctx);
  dump_registers();    
  return 0;
}
