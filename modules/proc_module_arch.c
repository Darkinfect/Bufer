/*
 * proc_module.c - Модуль с /proc файлом для Arch Linux
 *
 * Создаёт файл /proc/student_info с информацией о студенте
 * и счётчиком обращений.
 *
 * Компиляция: make
 * Использование:
 *   sudo insmod proc_module.ko
 *   cat /proc/student_info
 *   sudo rmmod proc_module
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>
#include <linux/time.h>

#define PROC_NAME "student_info"
#define MAX_SIZE 1024

// Глобальные переменные
static struct proc_dir_entry *proc_file = NULL;
static int read_count = 0;
static unsigned long load_time = 0;

// Параметры модуля (можно задать при загрузке)
static char *student_name = "Kuharev Kirill";
static int group = 9;
static int subgroup = 2;

module_param(student_name, charp, 0644);
MODULE_PARM_DESC(student_name, "Student name");

module_param(group, int, 0644);
MODULE_PARM_DESC(group, "Group number");

module_param(subgroup, int, 0644);
MODULE_PARM_DESC(subgroup, "Subgroup number");

/**
 * proc_read - Функция чтения из /proc файла
 * 
 * Вызывается когда пользователь читает /proc/student_info
 * Например: cat /proc/student_info
 */
static ssize_t proc_read(struct file *file, char __user *ubuf,
                         size_t count, loff_t *ppos)
{
    char buf[MAX_SIZE];
    int len;
    unsigned long uptime_jiffies;
    unsigned long uptime_seconds;

    // Если уже читали (повторный вызов), возвращаем 0 (EOF)
    if (*ppos > 0)
        return 0;

    // Увеличиваем счётчик обращений
    read_count++;

    // Вычисляем uptime
    uptime_jiffies = jiffies - load_time;
    uptime_seconds = uptime_jiffies / HZ;

    // Заполняем буфер информацией
    len = snprintf(buf, sizeof(buf),
        "╔══════════════════════════════════════════════════╗\n"
        "║         Student Information                      ║\n"
        "╠══════════════════════════════════════════════════╣\n"
        "  Name:              %s\n"
        "  Group:             %d\n"
        "  Subgroup:          %d\n"
        "  Module loaded at:  %lu jiffies\n"
        "  Module uptime:     %lu seconds\n"
        "  Read count:        %d\n"
        "  Current jiffies:   %lu\n"
        "╚══════════════════════════════════════════════════╝\n",
        student_name, group, subgroup, load_time, 
        uptime_seconds, read_count, jiffies);

    // Копируем данные из kernel space в user space
    if (copy_to_user(ubuf, buf, len)) {
        printk(KERN_ERR "proc_module: Failed to copy data to user space\n");
        return -EFAULT;
    }

    // Обновляем позицию чтения
    *ppos = len;

    printk(KERN_INFO "proc_module: /proc/%s read (count: %d)\n", 
           PROC_NAME, read_count);

    return len;
}

// Структура операций для proc файла
static const struct proc_ops proc_file_ops = {
    .proc_read = proc_read,
};

/**
 * proc_module_init - Функция инициализации модуля
 * 
 * Вызывается при загрузке модуля (insmod)
 */
static int __init proc_module_init(void)
{
    printk(KERN_INFO "proc_module: Initializing for Arch Linux\n");

    // Сохраняем текущее время загрузки (в jiffies)
    load_time = jiffies;

    // Создаём proc файл
    // Параметры: имя, права доступа (0444 = r--r--r--), родитель, операции
    proc_file = proc_create(PROC_NAME, 0444, NULL, &proc_file_ops);

    if (!proc_file) {
        printk(KERN_ERR "proc_module: Failed to create /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }

    printk(KERN_INFO "proc_module: Successfully created /proc/%s\n", PROC_NAME);
    printk(KERN_INFO "proc_module: Student: %s, Group: %d, Subgroup: %d\n",
           student_name, group, subgroup);
    printk(KERN_INFO "proc_module: Load time: %lu jiffies\n", load_time);

    return 0;
}

/**
 * proc_module_exit - Функция выгрузки модуля
 * 
 * Вызывается при выгрузке модуля (rmmod)
 */
static void __exit proc_module_exit(void)
{
    // Удаляем proc файл
    if (proc_file) {
        proc_remove(proc_file);
        printk(KERN_INFO "proc_module: Removed /proc/%s\n", PROC_NAME);
    }

    printk(KERN_INFO "proc_module: Module unloaded. Total reads: %d\n", read_count);
}

module_init(proc_module_init);
module_exit(proc_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Student (Arch Linux)");
MODULE_DESCRIPTION("Proc filesystem example for Arch Linux");
MODULE_VERSION("1.0");

/*
 * ТЕСТИРОВАНИЕ НА ARCH LINUX:
 *
 * 1. Сборка:
 *    $ make
 *
 * 2. Загрузка модуля:
 *    $ sudo insmod proc_module.ko
 *    
 *    Или с параметрами:
 *    $ sudo insmod proc_module.ko student_name="Petrov Petr" group=7 subgroup=2
 *
 * 3. Проверка создания файла:
 *    $ ls -la /proc/student_info
 *
 * 4. Чтение файла:
 *    $ cat /proc/student_info
 *    $ cat /proc/student_info
 *    $ cat /proc/student_info
 *
 * 5. Просмотр логов (Arch специфично):
 *    $ dmesg | tail -10
 *    или
 *    $ journalctl -k -n 20
 *
 * 6. Выгрузка модуля:
 *    $ sudo rmmod proc_module
 *
 * 7. Проверка логов выгрузки:
 *    $ dmesg | tail -5
 *
 *
 * ОЖИДАЕМЫЙ ВЫВОД:
 *
 * $ cat /proc/student_info
 * ╔══════════════════════════════════════════════════╗
 * ║         Student Information                      ║
 * ╠══════════════════════════════════════════════════╣
 *   Name:              Kuharev Kirill
 *   Group:             9
 *   Subgroup:          2
 *   Module loaded at:  4295123456 jiffies
 *   Module uptime:     120 seconds
 *   Read count:        1
 *   Current jiffies:   4295243456
 * ╚══════════════════════════════════════════════════╝
 *
 *
 * ОТЛАДКА НА ARCH:
 *
 * 1. Проверка наличия заголовков ядра:
 *    $ pacman -Q linux-headers
 *    или
 *    $ pacman -Q linux-lts-headers
 *
 * 2. Если модуль не загружается:
 *    $ dmesg | grep proc_module
 *    $ journalctl -k | grep proc_module
 *
 * 3. Проверка прав доступа:
 *    $ ls -la /proc/student_info
 *    Должно быть: r--r--r-- (444)
 *
 * 4. Просмотр информации о модуле:
 *    $ modinfo proc_module.ko
 *
 * 5. Проверка загруженных модулей:
 *    $ lsmod | grep proc_module
 *
 *
 * ARCH LINUX СПЕЦИФИЧНЫЕ ОСОБЕННОСТИ:
 *
 * - Используйте journalctl -k для просмотра kernel логов
 * - Если kernel signing включен, может потребоваться подпись модуля
 * - Для разработки модулей установите: sudo pacman -S base-devel linux-headers
 *
 *
 * ДОПОЛНИТЕЛЬНЫЕ ВОЗМОЖНОСТИ:
 *
 * 1. Изменение параметров во время работы (если права > 0444):
 *    $ echo "New Name" | sudo tee /sys/module/proc_module/parameters/student_name
 *
 * 2. Просмотр параметров модуля:
 *    $ cat /sys/module/proc_module/parameters/student_name
 *    $ cat /sys/module/proc_module/parameters/group
 *
 * 3. Мониторинг в реальном времени (Arch специфично):
 *    Terminal 1: $ journalctl -k -f
 *    Terminal 2: $ watch -n 1 cat /proc/student_info
 */
