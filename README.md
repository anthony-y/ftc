# ftc
Shows basic system info on Linux systems.

## Metrics

ftc can fetch the following stats about your system:

* Kernel name
* Current memory usage
* Number of installed packages \*
* Uptime
* Installation date \*
* CPU temperature (**WIP**)

\* only for Arch/Manjaro/derivatives (requires pacman, basically)

## Customization

You can change the colours of the prompts.
Just change the defines and recompile.

```C++
#define ANSI_COLOR_MAGENTA "\x1b[1;35m"
#define ANSI_COLOR_CYAN    "\x1b[1;36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define TITLE_COLOR    ANSI_COLOR_CYAN
#define INFO_COLOR     ANSI_COLOR_RESET // COLOR_RESET just means "default colour"
#define USERNAME_COLOR ANSI_COLOR_MAGENTA
#define HOSTNAME_COLOR ANSI_COLOR_RESET
```

Then recompile, instructions below :)

## How to install

ftc has no dependencies (except the C standard library) so it's extremely easy to compile:

```
gcc ftc.c -o ftc
```

Then symlink it to somewhere in your path:

```
sudo ln -s /full/path/to/ftc /bin/ftc
```
