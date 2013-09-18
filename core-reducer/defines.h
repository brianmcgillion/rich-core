/*
 * This file is part of sp-rich-core
 *
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 *
 * Contact: Brian McGillion <brian.mcgillion@symbio.com>, Denis Mingulov <denis.mingulov@symbio.com>
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

/*!
  * \file defines.h
  * \author Brian McGillion <brian.mcgillion@symbio.com>, Denis Mingulov <denis.mingulov@symbio.com>
  *
  * \brief Global definitions that are used throughout the application.
  */


#ifndef DEFINES_H
#define DEFINES_H

#include <stdint.h>
#include <elf.h>
#include <syslog.h>
#include "../config.h"

#ifdef LOGGING
/*!
  * \def LOG_RETURN
  * Uses syslog to write the error messages.
  * \param priority Is the error level of the message
  * \param _retval The value to return from the method with
  * \param message The error Message to report
  * \param arg parameters for message
  */
#define LOG_RETURN(priority, _retval, message, arg...)                                            \
do {                                                                                      \
        syslog(priority, "%s:%s:%d: " message "\n", __FILE__, __FUNCTION__, __LINE__, ## arg);  \
        return _retval;                                                                   \
   } while(0)

#define LOG(priority, message, arg...)                                                            \
do  {                                                                                     \
   syslog(priority, "%s:%s:%d: " message "\n", __FILE__, __FUNCTION__, __LINE__, ## arg);      \
} while(0)
#else
#define LOG_RETURN(priority, _retval, message, arg...) \
do {                                                   \
        return _retval;                                \
   } while(0)
#define LOG(...) do {} while(0)
#endif


#if __WORDSIZE == 32
/*!
  * \typedef Elf32_Ehdr Ehdr
  * define a new type for the Elf header struct that can be used to mask 32/64 bit differences in the underlying library
  */
typedef Elf32_Ehdr Ehdr;
/*!
  * \typedef Elf32_Phdr Phdr
  * define a new type for the Program Header struct that can be used to mask 32/64 bit differences in the underlying library
  */
typedef Elf32_Phdr Phdr;
/*!
  * \typedef Elf32_Shdr Shdr
  * define a new type for the Section Header Struct that can be used to mask 32/64 bit differences in the underlying library
  */
typedef Elf32_Shdr Shdr;
/*!
  * \typedef uint32_t ADDRESS
  * define a new type that can be used to reference Virtual memory addresses and can mask 32/64 bit differences
  */
typedef uint32_t ADDRESS;
/*!
  * \typedef Elf32_Word Elf_Word
  * define a new type for the word size used in 32/64 bit processors
  */
typedef Elf32_Word Elf_Word;
/*!
  * \typedef Elf32_Sword Elf_SWord
  * define a new type for the signed word size used in 32/64 bit processors
  */
typedef Elf32_Sword Elf_SWord;
/*!
  * \typedef Elf32_Dyn Elf_Dyn
  * define a new type for the dynamic section structure that can be used to mask 32/64 bit differences in the underlying library
  */
typedef Elf32_Dyn Elf_Dyn;
/*!
  * \typedef Elf32_Nhdr Nhdr
  * define a new type for the note section structure that can be used to mask 32/64 bit differences in the underlying library
  */
typedef Elf32_Nhdr Nhdr;
/*!
  * \def LM_NAME
  * The offset in the link map structure that points to the name string of the library that is referenced by this link
  */
#define LM_NAME 4
typedef Elf32_auxv_t Auxv;
#else
/*!
  * \typedef Elf32_Ehdr Ehdr
  * define a new type for the Elf header struct that can be used to mask 32/64 bit differences in the underlying library
  */
typedef Elf64_Ehdr Ehdr;
/*!
  * \typedef Elf64_Phdr Phdr
  * define a new type for the Program Header struct that can be used to mask 32/64 bit differences in the underlying library
  */
typedef Elf64_Phdr Phdr;
/*!
  * \typedef Elf64_Shdr Shdr
  * define a new type for the Section Header Struct that can be used to mask 32/64 bit differences in the underlying library
  */
typedef Elf64_Shdr Shdr;
/*!
  * \typedef uint64_t ADDRESS
  * define a new type that can be used to reference Virtual memory addresses and can mask 32/64 bit differences
  */
typedef uint64_t ADDRESS;
/*!
  * \typedef Elf64_Word Elf_Word
  * define a new type for the word size used in 32/64 bit processors
  */
typedef Elf64_Word Elf_Word;
/*!
  * \typedef Elf64_Sxword Elf_SWord
  * define a new type for the signed word size used in 32/64 bit processors
  */
typedef Elf64_Sxword Elf_SWord;
/*!
  * \typedef Elf64_Dyn Elf_Dyn
  * define a new type for the dynamic section structure that can be used to mask 32/64 bit differences in the underlying library
  */
typedef Elf64_Dyn Elf_Dyn;
/*!
  * \typedef Elf64_Nhdr Nhdr
  * define a new type for the note section structure that can be used to mask 32/64 bit differences in the underlying library
  */
typedef Elf64_Nhdr Nhdr;
/*!
  * \def LM_NAME
  * The offset in the link map structure that points to the name string of the library that is referenced by this link
  */
#define LM_NAME 8
typedef Elf64_auxv_t Auxv; 
#endif

#endif // DEFINES_H
