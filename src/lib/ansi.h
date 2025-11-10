#ifndef ANSI_H
#define ANSI_H
#define CSI "\x1b["
#define C_RESET   CSI "0m"
#define C_BOLD    CSI "1m"
#define C_DIM     CSI "2m"
#define C_RED     CSI "31m"
#define C_GREEN   CSI "32m"
#define C_YELLOW  CSI "33m"
#define C_BLUE    CSI "34m"
#define C_MAGENTA CSI "35m"
#define C_CYAN    CSI "36m"
#define C_ACCENT  CSI "1;36m"
#endif 
