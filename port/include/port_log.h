#ifndef PORT_LOG_H
#define PORT_LOG_H

/* SD card logging (task 02b): printf does not reach Dolphin's log, so the
 * port logs lines to sd:/drmario/log.txt via libfat. Safe no-op if the
 * SD card is missing (port_log_init and PORT_LOG just do nothing). */
int port_log_init(void);
void PORT_LOG(const char *fmt, ...);
void port_log_flush(void);

#endif
