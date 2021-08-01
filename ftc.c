#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef signed   char s8;
typedef unsigned char u8;
typedef signed   short s16;
typedef unsigned short u16;
typedef signed   int s32;
typedef unsigned int u32;
typedef signed   long int s64;
typedef unsigned long int u64;
typedef float f32;
typedef double f64;

#define IO_BUFFER_SIZE 10240 // 10kb

// My CPU shows as a PCI adapter in `sensors` for some reason??
#define CPU_SENSOR_NAME "k10temp-pci-00c3"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[1;34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[1;36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

static char *do_command(const char *the_command);
static char *read_file(const char *path);

static void fetch_user_and_host_name();
static void fetch_memory_info();
static void fetch_cpu_info();

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

//
// TODO: maybe rewrite this to parse from the `free` command, instead of reading /proc/meminfo,
//       idk, this approach is more extensible.
// 
// Print format:
//   Memory: used / available MiB
//
static void fetch_memory_info()
{
	char *meminfo = read_file("/proc/meminfo");

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

	printf(ANSI_COLOR_BLUE "Memory" ANSI_COLOR_RESET ": %ld MiB / %ld MiB\n", used_memory, total_memory);
}

static void fetch_user_and_host_name()
{
	char *username = do_command("whoami");
	char *hostname = read_file("/proc/sys/kernel/hostname");

	printf(ANSI_COLOR_CYAN "%s" ANSI_COLOR_RESET "@%s", username, hostname);

	// Print vanity separator.
	int separator_length = strlen(username) + strlen(hostname) + 1;
	for (int i = 0; i < separator_length; i++) printf("-");
	printf("\n");

	free(hostname);
	free(username);
}

static void fetch_pacman_package_count()
{
	char *packages = do_command("pacman -Q | wc -l"); // TODO: improve, we have to run pacman -Q which takes a while
	printf(ANSI_COLOR_BLUE "Packages" ANSI_COLOR_RESET ": ");
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

	printf(ANSI_COLOR_BLUE "Kernel" ANSI_COLOR_RESET ": ");
	printf("linux %.*s\n", (int)(end_of_version - start_of_version), start_of_version);

	free(version);
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
	if (bytes_read < 1)	// TODO: this might be wrong, but right now I don't think any /proc/ files are 0 size.
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

static void fetch_cpu_info()
{
    return;
	char *sensors = do_command("sensors");
	char *cpu_temp = NULL;

	char *cursor = sensors;
	while (*cursor)
	{
		char *name_start = cursor;
		while (*cursor && *cursor++ != '\n');
		char *name_end = cursor;

		printf("%.*s\n", (int)(name_end - name_start), name_start);

		if (strncmp(name_start, CPU_SENSOR_NAME, (int)(name_end - name_start)) == 0)
		{
			while (1)
			{
				if (*cursor == '\n') break;
				cursor++;

				char *info_line_start = cursor, *info_line_end = 0;
				while (*cursor && *cursor++ != '\n');
				info_line_end = cursor;
				
				if (strncmp(info_line_start, "Adapter", strlen("Adapter")) == 0) continue;

				if (strncmp(info_line_start, "Tdie:", strlen("Tdie:")) == 0)
				{
					cursor += strlen("Tdie:");
					while ((*cursor == '+' || *cursor == '-') && *cursor++ == ' ');
					char *temp_start = cursor;
					while (is_numerical(*cursor) || *cursor++ == '.');
					u64 temp_length = (u64)(cursor - temp_start); 
					cpu_temp = malloc(temp_length);
					strncpy(cpu_temp, temp_start, temp_length);
				}
			}
		}
		else
		{
			continue;
		}

		cursor++;
	}

	if (!cpu_temp) return;
	printf(ANSI_COLOR_BLUE "Processor temp " ANSI_COLOR_RESET ": %s°C", cpu_temp);
	free(cpu_temp);
	free(sensors);
}

int main(int arg_count, char **args)
{
	printf("\n");

	fetch_user_and_host_name();
	fetch_kernel_version();
	fetch_memory_info();
	fetch_pacman_package_count();
	fetch_cpu_info();

	printf("\n");

	return 0;
}