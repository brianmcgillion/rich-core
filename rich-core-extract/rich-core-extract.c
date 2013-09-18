/*
 * This file is part of sp-rich-core
 *
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies). 
 *
 * Contact: Eero Tamminen <eero.tamminen@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License 
 * version 2 as published by the Free Software Foundation. 
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */  

#define _GNU_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#define RICHCORE_HEADER "[---rich-core: "
#define RICHCORE_HEADER_END "---]\n"

const char *usage = "%s <input filename> [<output directory>]\n";

/*!
  * \brief Returns position of reject in mem, used to search rich-core header data from data
  * \param mem Data to search reject from
  * \param memlen Size of mem
  * \param reject Data to be searched from mem
  * \param rejectlen Size of reject
  * \return Position of reject in mem
  */
size_t memcspn(const void *mem, size_t memlen, const void *reject, size_t rejectlen);

/* Buffer variables and functions */
#define BUFFER_SIZE 4096 + 128
/* Copy num bytes from data to buffer */
void buffer_data(char* data, size_t num);
/* Remove num bytes from buffer */
void unbuffer_data(size_t num);
/* Buffer */
char buffer[BUFFER_SIZE];
/* Buffer size */
int size = 0;

int main(int argc, char *argv[])
{
    char *input_fn;
    char *output_dir;
    struct stat stat_s;
    FILE *input_file;
    FILE *output_file;
    char buf[4096];

    if (argc < 2)
    {
        fprintf(stderr, usage, argv[0]);
        exit(1);
    }

    input_fn = argv[1];
    if (argc < 3)
    {
        /* check if filename ends with .rcore or .rcore.lzo */
        char *suffix = strstr(input_fn, ".rcore");

        if (!suffix || (strcmp(suffix, ".rcore.lzo") && strcmp(suffix, ".rcore")))
        {
            fprintf(stderr, "please specify output directory\n");
            exit(1);
        }

        if (!strcmp(suffix, ".rcore"))
            output_dir = strndup( input_fn, strlen(input_fn)-6 );
        else
            output_dir = strndup( input_fn, strlen(input_fn)-10 );
    }
    else
    {
        output_dir = argv[2];
    }

#ifdef DEBUG
    fprintf(stderr, "input: '%s'\n", input_fn);
    fprintf(stderr, "output: '%s'\n", output_dir);
#endif

    if (stat(input_fn, &stat_s))
    {
        fprintf(stderr, "input file error: %s\n", strerror(errno));
        exit(1);
    }

    if (S_ISDIR(stat_s.st_mode))
    {
        fprintf(stderr, "%s is a directory\n", input_fn);
        exit(1);
    }

    if (stat(output_dir, &stat_s))
    {
        if (errno != ENOENT)
        {
            fprintf(stderr, "error testing output: %s\n", strerror(errno));
            exit(1);
        }
    }
    else
    {
        fprintf(stderr, "%s exists, aborting\n", output_dir);
        exit(1);
    }

    if (mkdir(output_dir, 0777))
    {
        fprintf(stderr, "error creating %s: %s\n", output_dir, strerror(errno));
        exit(1);
    }

    snprintf(buf, 255, "lzop -d -c %s", input_fn);
    input_file = popen(buf, "r");
    output_file = fopen("/dev/null", "w");
    if (!input_file)
    {
        fprintf(stderr, "error forking lzop: %s\n", strerror(errno));
        exit(1);
    }

    while (!feof(input_file) && !ferror(input_file))
    {
        /* Read 4096 bytes from input file */
        int remaining = fread(buf, 1, sizeof(buf), input_file);
        char extra[128];
        int extra_size = 0;

        /* if buffer had less than 128bytes extra over remaining try to read more */
        if(size < 128 && !feof(input_file) && !ferror(input_file)) 
        {
            extra_size = fread(extra, 1, 128 - size, input_file);
        }

        /* Write remaining bytes to buffer */
        buffer_data(buf, remaining);
        /* Write extra bytes to buffer */
        buffer_data(extra, extra_size);

        /* Loop ------- */
        while(1)
        {
            /* Check for start of richcore header and header end in buffer's remaining+extra bytes */
            int start_of_header = memcspn(buffer, size, RICHCORE_HEADER, sizeof(RICHCORE_HEADER));
            //printf("Start of header %s\n", &buffer[start_of_header]);
            int end_of_header = memcspn(buffer, size, RICHCORE_HEADER_END, sizeof(RICHCORE_HEADER_END));

            if(end_of_header != size) {
                end_of_header += sizeof(RICHCORE_HEADER_END) -1;
            }

            /* If no start was found or start was after remaining bytes write remaining bytes to output_file, exit loop */
            if(start_of_header == size || start_of_header > remaining) 
            {
                fwrite(buffer, 1, remaining, output_file);
                unbuffer_data(remaining);

                /* If feof is true, process also extra bytes */
                if(feof(input_file) && start_of_header > remaining
                    && (start_of_header - remaining) != size)
                {
                    start_of_header -= remaining;
                    end_of_header -= remaining;
                    remaining = size;
                }
                else
                {
                    break;
                }
            }

            /* If start was found in remaining bytes write data before start to output_file */
            if(start_of_header <= remaining)
            {
                /* To not break binaries, don't write the \n before header */
                fwrite(buffer, 1, start_of_header -1, output_file);
                remaining -= start_of_header;

                /* If no end was found change output_file to /dev/null and write remaining bytes, exit loop */
                if(end_of_header == size)
                {
                    fprintf(stderr, "skipping invalid rich core header\n");
                    output_file = fopen("/dev/null", "w");
                    fwrite(buffer, 1, remaining, output_file);
                    unbuffer_data(remaining);
                    break;
                }

                unbuffer_data(start_of_header);
            }

            /* End was found so parse new filename and unbuffer the header */
            char *start_of_filename = &buffer[sizeof(RICHCORE_HEADER)-1];
            char *end_of_filename = strstr(start_of_filename, RICHCORE_HEADER_END);
            char *c;
            char fn[128];

            *end_of_filename = '\0';

            c = basename(start_of_filename);

            snprintf(fn, sizeof(fn), "%s/%s", output_dir, c);
    #ifdef DEBUG
            puts(fn);
    #endif
            fclose(output_file);
            output_file = fopen(fn, "w");
            remaining -= (end_of_header - start_of_header);
            unbuffer_data(end_of_header - start_of_header);

            /* If end was in remaining bytes + extra, exit loop */
            if(remaining <= 0)
            {
                break;
            }
            /* If end was in remaining bytes, loop again */
        }
        /* End loop ------- */
    }

    /* Empty out the buffer */
    fwrite(buffer, 1, size, output_file);
    fclose(output_file);
    pclose(input_file);
    exit(0);
}

void buffer_data(char* data, size_t num)
{
    memcpy(&buffer[size], data, num);
    size += num;
}

void unbuffer_data(size_t num)
{
    size -= num;
    char* move = malloc(sizeof(char) * size);
    memcpy(move, &buffer[num], size);
    memcpy(buffer, move, size);
    free (move);
}

size_t memcspn(const void *mem, size_t memlen,
	       const void *reject, size_t rejectlen)
{
  size_t i;

  const char *m = mem, *r = reject;

  if (rejectlen == 0 || reject == 0)
    return memlen;

  if (mem == NULL || memlen == 0)
    return 0;

  for (i = 0; i < memlen; i++)
  {
        if (strncmp(r, m+i, rejectlen-1) == 0)
        {
          break;
        }
  }
  return i;
}
