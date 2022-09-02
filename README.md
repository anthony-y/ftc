# ftc
Shows basic system info on Linux systems.

## Metrics

ftc can fetch the following stats about your system:

* Kernel name
* Current memory usage
* Uptime
* CPU temperature
* Date of system installation \*
* Number of installed packages \*

\* only for Arch Linux & derivatives, i.e Manjaro, Artix (requires pacman, basically)

## Customization

You can change which stats appear and in which order, see _main()_ in ftc.c.

You can change the colours of the prompts, just change the "COLOR" defines and recompile (instructions below).

```C++
#define ANSI_COLOR_MAGENTA "\x1b[1;35m"
#define ANSI_COLOR_CYAN    "\x1b[1;36m"
#define ANSI_COLOR_DEFAULT "\x1b[0m" // ANSI_COLOR_DEFAULT just means "default colour"

#define TITLE_COLOR    ANSI_COLOR_CYAN
#define INFO_COLOR     ANSI_COLOR_DEFAULT
#define USERNAME_COLOR ANSI_COLOR_MAGENTA
#define HOSTNAME_COLOR ANSI_COLOR_DEFAULT
```

## How to install

ftc has no dependencies (except the C standard library) so it's extremely easy to compile, all you need is a C compiler:

```
gcc ftc.c -o ftc
```

Then symlink it to somewhere in your path:

```
sudo ln -s /full/path/to/ftc /bin/ftc
```
