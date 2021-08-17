#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char     u8;
typedef unsigned long int u64;

#define IO_BUFFER_SIZE 10240 // 10kb

// For any of these, you can change the "1;" to a "0;" to make it non-bold (these are all bold by default, except ANSI_COLOR_DEFAULT).
#define ANSI_COLOR_RED     "\x1b[1;31m" 
#define ANSI_COLOR_GREEN   "\x1b[1;32m"
#define ANSI_COLOR_YELLOW  "\x1b[1;33m"
#define ANSI_COLOR_BLUE    "\x1b[1;34m"
#define ANSI_COLOR_MAGENTA "\x1b[1;35m"
#define ANSI_COLOR_CYAN    "\x1b[1;36m"

#define ANSI_COLOR_DEFAULT "\x1b[0m"

#define TITLE_COLOR    ANSI_COLOR_YELLOW
#define INFO_COLOR     ANSI_COLOR_DEFAULT
#define USERNAME_COLOR ANSI_COLOR_MAGENTA
#define HOSTNAME_COLOR ANSI_COLOR_DEFAULT

// These are the core functions of the program.
// They do all the heavy-lifting of querying the system for information.
static char *do_command(const char *the_command);
static char *read_file(const char *path);

// These call do_command and read_file to get the right info.
// They are "pure", so not state, and ultimately they just invoke printf.
static void fetch_installation_date();
static void fetch_uptime();
static void fetch_memory_info();
static void fetch_user_and_host_name();
static void fetch_pacman_package_count();
static void fetch_kernel_version();
static void fetch_cpu_temperature();

int main(int arg_count, char **args)
{
    printf("\n");

    /*
     * This is the order they will appear in.
     * It can be any order you want.
     * You can also comment out any you don't want.
    */
    fetch_user_and_host_name();
    fetch_kernel_version();
    fetch_memory_info();
    fetch_pacman_package_count();
    fetch_uptime();
    fetch_cpu_temperature();
    fetch_installation_date();

    printf("\n");

    return 0;
}

//
// Util functions for parsing.
//
static inline int is_letter(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}
static inline int is_numerical(char c)
{
    return (c >= '0' && c <= '9');
}


static void fetch_installation_date()
{
    char *first_pacman_cmd = do_command("head -n1 /var/log/pacman.log");
    if (!first_pacman_cmd || *first_pacman_cmd != '[') return; // we'll exit here if the system doesn't use pacman.

    char *start = first_pacman_cmd+1;
    char *cursor = start;
    while (*cursor++ != 'T');
    cursor--;

    u64 length = (cursor - start);
    printf(TITLE_COLOR "System installed" INFO_COLOR ": ");
    printf("%.*s\n", length, start);

    free(first_pacman_cmd);
}

static void fetch_uptime()
{
    char *uptime = do_command("uptime");
    char *cursor = uptime;
    char *start  = uptime;

    int colon_index = 0;
    int index       = 0;

    if (!uptime) return;

    while (*cursor++ != 'p');
    while (*cursor == ' ' || *cursor == '\t') cursor++;
    start = cursor;

    int minutes_only = 0;
    while (1)
    {
        if (*cursor == 'm')
        {
            minutes_only = 1;
            break;
        }
        if (*cursor == ',' || index >= 1000) break;
        if (*cursor == ':') colon_index = index;
        index++;
        cursor++;
    }

    if (minutes_only == 1)
    {
        u64 length = (cursor - start) - 1;
        printf(TITLE_COLOR "Uptime" INFO_COLOR ": ");
        printf("%.*s minutes\n", length, start);
        free(uptime);
        return;
    }

    u64 minutes_length = (index - colon_index) - 1;

    char *hour_string    = malloc(colon_index);
    char *minutes_string = malloc(minutes_length);

    strncpy(hour_string, start, colon_index);
    hour_string[colon_index] = 0;

    strncpy(minutes_string, start+colon_index+1, minutes_length);
    minutes_string[minutes_length] = 0;

    u64 length = (cursor - start);
    printf(TITLE_COLOR "Uptime" INFO_COLOR ": ");
    printf("%s hours, %s minutes\n", hour_string, minutes_string);

    free(hour_string);
    free(minutes_string);
    free(uptime);
}

// Print format:
//   Memory: used / available MiB
static void fetch_memory_info()
{
    char *meminfo = read_file("/proc/meminfo");
    if (!meminfo) return;

    u64 total_memory = 0, free_memory = 0;

    char *cursor = meminfo;
    char *text_start = 0, *text_end = 0;
    char *number_start = 0, *number_end = 0;

    while (*cursor)
    {
        // Each line starts with the name of the variable.
        // Then, a colon.
        // Then, some whitespace.
        // Then, an integer value.
        // Then, the units (kB, MiB, etc.)
        // Finally, a new line character.
        if (is_letter(*cursor))
        {
            // Skip the text, storing a pointer to where it starts
            // and how long it is so we can use strncmp on it later.
            text_start = cursor;
            while (is_letter(*cursor) && *cursor++);
            text_end = cursor;
            u64 length = (u64)(text_end - text_start);

            // Skip the colon and the spaces.
            if (*cursor != ':') *(void *)0; // bootleg assert()
            cursor++; // :
            while (*++cursor == ' ');

            // Skip the integer value, storing a pointer to where it starts
            // and how it long it is so we can copy it and convert it to an int.
            number_start = cursor;
            while (is_numerical(*cursor) && *cursor++);
            number_end = cursor;
            u64 number_length = (u64)(number_end - number_start);
            char number_as_text[number_length+1];
            strncpy(number_as_text, number_start, length);

            // We're only interested in the total memory installed on the system...
            if (strncmp(text_start, "MemTotal", length) == 0)
                total_memory = atol(number_as_text);

            // ... and how much of it is available.
            else if (strncmp(text_start, "MemAvailable", length) == 0)
                free_memory = atol(number_as_text);

            // Skip the units.
            if (*cursor != 'k' && *cursor+1 != 'B') *(void *)0;
            cursor += 2; // kB

            // Skip the new line character.
            if (*cursor != '\n') *(void *)0;
            cursor++;
        }
        cursor++;
    }

    free(meminfo);

    // It's in kB originally but we want to display it in MiB.
    total_memory /= 1024;
    free_memory  /= 1024;
    u64 used_memory = total_memory - free_memory;

    printf(TITLE_COLOR "Memory" INFO_COLOR ": %ld MiB / %ld MiB\n", used_memory, total_memory);
}

static void fetch_user_and_host_name()
{
    char *username = do_command("whoami");
    char *hostname = read_file("/proc/sys/kernel/hostname");
    if (!username || !hostname) return;

    printf(USERNAME_COLOR "%s" ANSI_COLOR_DEFAULT "@" HOSTNAME_COLOR "%s", username, hostname);

    // Print vanity separator.
    int separator_length = strlen(username) + strlen(hostname) + 1;
    for (int i = 0; i < separator_length; i++) printf("-");
    printf("\n");

    free(hostname);
    free(username);
}

static void fetch_pacman_package_count()
{
    // TODO: improve, we have to run pacman -Q which takes a while.
    char *packages = do_command("pacman -Q | wc -l");
    if (!packages) return; // we'll exit here if the system doesn't use pacman.

    printf(TITLE_COLOR "Packages" INFO_COLOR ": ");
    printf("%s", packages);
    printf(" (pacman)\n");

    free(packages);
}

static void fetch_kernel_version()
{
    char *version = read_file("/proc/version");

    // Skip the fluff "Linux version..." blah blah
    char *cursor = version, *start_of_text = version;
    while ((is_letter(*cursor) || *cursor == ' ') && *cursor != '\n' && *cursor++);

    char *start_of_version = cursor;
    while (*cursor != ' ' && *cursor++ != '\n');
    char *end_of_version = cursor;

    printf(TITLE_COLOR "Kernel" INFO_COLOR ": ");
    printf("linux %.*s\n", (int)(end_of_version - start_of_version), start_of_version);

    free(version);
}

static void fetch_cpu_temperature()
{
    char *cpu_temp = read_file("/sys/class/hwmon/hwmon1/temp2_input");
    if (!cpu_temp)
    {
        printf(TITLE_COLOR "Processor temp" INFO_COLOR ": unavailable\n");
        return;
    }

    float as_float = atof(cpu_temp);
    as_float /= 1000;
    int as_int = (int)as_float;

    printf(TITLE_COLOR "Processor temp" INFO_COLOR ": %d°C\n", as_int);
    free(cpu_temp);
}

static char *do_command(const char *the_command)
{
    FILE *pipe = popen(the_command, "r");
    if (!pipe)
    {
        fprintf(stderr, "ftc: error: failed to open pipe from command '%s'.\n", the_command);
        exit(1);
    }

    char buffer[IO_BUFFER_SIZE];
    u64 bytes_read = fread(buffer, 1, IO_BUFFER_SIZE, pipe);
    if (bytes_read < 1)
    {
        fprintf(stderr, "ftc: error: failed to read pipe from command '%s'.\n", the_command);
        pclose(pipe);
        exit(1);
    }

    pclose(pipe);

    char *ret = malloc(bytes_read);
    strncpy(ret, buffer, bytes_read);
    
    // Remove trailing newline and null terminate.
    if (ret[bytes_read] == '\n') bytes_read--;
    ret[bytes_read-1] = 0;

    return ret;
}

// Read a /proc/ file into memory, the resulting pointer must be free()'d.
static char *read_file(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f)
    {
        fprintf(stderr, "ftc: error: failed to open file %s.\n", path);
        exit(1);
    }

    char buffer[IO_BUFFER_SIZE];

    u64 bytes_read = fread(buffer, 1, IO_BUFFER_SIZE, f);
    if (bytes_read < 1) // TODO: this might be wrong, but right now I don't think any /proc/ files are 0 size.
    {
        fprintf(stderr, "ftc: error: failed to read file \"%s\".\n", path);
        fclose(f);
        exit(1);
    }
    fclose(f);

    char *ret = malloc(bytes_read);
    strncpy(ret, buffer, bytes_read);
    ret[bytes_read] = 0;
    return ret;
}
