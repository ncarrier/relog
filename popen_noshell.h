/*
 * popen_noshell.h
 *
 *  Created on: Oct 6, 2015
 *      Author: ncarrier
 */

#ifndef POPEN_NOSHELL_H_
#define POPEN_NOSHELL_H_

FILE *popen_noshell(const char *command, const char *type);
int pclose_noshell(FILE *file);

#endif /* POPEN_NOSHELL_H_ */
