/*
 * This file is part of the superbitrf project.
 *
 * Copyright (C) 2015 Freek van Tienen <freek.v.tienen@gmail.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "console.h"
#include "modules/ring.h"
#include "modules/cdcacm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define CONSOLE_SIZE 128
char console_buf[CONSOLE_SIZE];
uint16_t console_idx = 0;

typedef struct {
    char *name;
    char *params;
    void (*cmdFunc)(char *cmdLine);
} console_cmd_t;

static int console_cmd_cmp(const void *c1, const void *c2);
static console_cmd_t *console_get_cmd(char *name);
static void console_print_help(char *cmdLine);

#define CONSOLE_CMDS_SIZE 50
static console_cmd_t console_cmds[CONSOLE_CMDS_SIZE];
static int console_cmds_idx = 0;

/**
 * Initialize the console
 */
void console_init(void) {
  console_cmd_add("help", "", console_print_help);
}

/**
 * Console main loop
 */
void console_run(void) {
  uint8_t c;

  // Parse characters
  while(!RING_EMPTY(&cdcacm_console_rx)) {
    ring_read_ch(&cdcacm_console_rx, &c);

    switch(c) {
      // If it is EOL
      case '\n':
      case '\r':
        ring_write_ch(&cdcacm_console_tx, c);

        // Parse the command
        console_buf[console_idx++] = 0;
        console_cmd_t* cmd = console_get_cmd(console_buf);
        if(cmd && cmd->cmdFunc)
          cmd->cmdFunc(console_buf + strlen(cmd->name));
        else
          console_print("\r\nCommand not found!");

        // Wait for new command
        console_idx = 0;
        console_print("\r\n> ");
        break;

      // If it is a backspace
      case 127:
        if(console_idx > 0) {
          ring_write_ch(&cdcacm_console_tx, c);
          console_idx--;
        }
        break;

      // Else default
      default:
        // Check if valid ascii character
        if(c < 32 || c > 126)
          c = 7;
        else
          console_buf[console_idx++] = c;

        ring_write_ch(&cdcacm_console_tx, c);
    }
  }
}

/**
 * Add a console command
 */
void console_cmd_add(char *name, char *params, void (*cmdFunc)(char *cmdLine)) {
  // Check if there is free space
  if(console_cmds_idx >= CONSOLE_CMDS_SIZE)
    return;

  console_cmds[console_cmds_idx].name = name;
  console_cmds[console_cmds_idx].params = params;
  console_cmds[console_cmds_idx++].cmdFunc = cmdFunc;
  qsort(&console_cmds, console_cmds_idx, sizeof console_cmds[0], console_cmd_cmp);
}

/**
 * Add a console command
 */
void console_cmd_rm(char *name) {
  console_cmd_t* cmd = console_get_cmd(name);
  if(cmd) {
    cmd->name = NULL;
    qsort(&console_cmds, console_cmds_idx, sizeof console_cmds[0], console_cmd_cmp);
    console_cmds_idx--;
  }
}

/**
 * Print a text on the console
 */
void console_print(const char *format, ...) {
  char buf[128];
  va_list argptr;
  va_start(argptr, format);
  vsprintf(buf, format, argptr);
  va_end(argptr);

  ring_write(&cdcacm_console_tx, (uint8_t*)buf, strlen(buf));
}

/**
 * Compare two console commands
 */
static int console_cmd_cmp(const void *c1, const void *c2) {
  const console_cmd_t *cmd1 = c1, *cmd2 = c2;
  if(cmd1->name == NULL)
    return INT32_MAX;
  else if(cmd2->name == NULL)
    return INT32_MIN;
  else
    return strncasecmp(cmd1->name, cmd2->name, strlen(cmd2->name));
}

/**
 * Get a console command by name
 */
static console_cmd_t *console_get_cmd(char *name) {
  console_cmd_t target = {name, NULL, NULL};
  return bsearch(&target, console_cmds, console_cmds_idx, sizeof console_cmds[0], console_cmd_cmp);
}

/**
 * Print the help of the console
 */
static void console_print_help(char *cmdLine __attribute((unused))) {
  int i;

  console_print("\r\nAvailable commands:");
  for(i = 0; i < console_cmds_idx; i++) {
    console_print("\r\n\t%s %s", console_cmds[i].name, console_cmds[i].params);
  }
}
