/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_log.c - Kids love log
 *
 * $Id$
 *
 * Copyright (C) 2004 David Beckett - http://purl.org/net/dajobe/
 * Institute for Learning and Research Technology - http://www.ilrt.bris.ac.uk/
 * University of Bristol - http://www.bristol.ac.uk/
 * 
 * This package is Free Software or Open Source available under the
 * following licenses (these are alternatives):
 *   1. GNU Lesser General Public License (LGPL)
 *   2. GNU General Public License (GPL)
 *   3. Mozilla Public License (MPL)
 * 
 * See LICENSE.html or LICENSE.txt at the top of this package for the
 * full license terms.
 * 
 * 
 */


#include <rdf_config.h>

#include <stdio.h>
#ifdef WITH_THREADS
#include <pthread.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* for abort() as used in errors */
#endif

/* for gettimeofday */
#if TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif

#include <librdf.h>
#include <raptor.h>


static const char *log_level_names[LIBRDF_LOG_LAST+1]={
  "none", "debug", "info", "warning", "error", "fatal"
};


/*
 * librdf_log_simple - Log a message - Internal
 * @world: redland world object or NULL
 * @code: error code
 * @level: &librdf_log_level log level
 * @facility: &librdf_log_facility log facility
 * @locator: raptor_locator if available or NULL
 * @message: message to record
 *
 * If world is NULL, the error ocurred in redland startup before
 * the world was created.
 **/
void
librdf_log_simple(librdf_world* world, int code, 
                  librdf_log_level level, librdf_log_facility facility,
                  void *locator, const char *message)
{
  if(level > LIBRDF_LOG_LAST)
    level=LIBRDF_LOG_NONE;
  
  if(facility > LIBRDF_FROM_LAST)
    facility=LIBRDF_FROM_NONE;
  
  if(world) {
    if(world->log_handler) {
      world->log.code=code;
      world->log.level=level;
      world->log.facility=facility;
      world->log.message=message;
      world->log.locator=locator;

      if(world->log_handler(world->error_user_data, &world->log))
        return;

    } else {
      switch(level) {
        case LIBRDF_LOG_ERROR:
          if(world->error_handler)
            world->error_handler(world->error_user_data, message, NULL);
          break;

        case LIBRDF_LOG_WARN:
          if(world->warning_handler)
            world->warning_handler(world->warning_user_data, message, NULL);
          break;

        default:
          break;
      }
    }
  }

  fputs("librdf ", stderr);
  fputs(log_level_names[level], stderr);
  if(locator) {
    int locator_len=raptor_format_locator(NULL, 0, locator);
    char *buffer;

    buffer=(char*)LIBRDF_MALLOC(cstring, locator_len+2);
    *buffer=' ';
    raptor_format_locator(buffer+1, locator_len, locator);
    fputs(buffer, stderr);
    LIBRDF_FREE(cstring, buffer);
  }
  
  fputs(" - ", stderr);
  fputs(message, stderr);
  fputc('\n', stderr);

  if(level == LIBRDF_LOG_FATAL)
    abort();
#ifdef LIBRDF_DEBUG
  if(level >= LIBRDF_LOG_ERROR)
    abort();
#endif
}


/*
 * librdf_log - Log a message - Internal
 * @world: redland world object or NULL
 * @code: error code
 * @level: &librdf_log_level log level
 * @facility: &librdf_log_facility log facility
 * @locator: raptor_locator if available or NULL
 * @message: message to record
 *
 * If world is NULL, the error ocurred in redland startup before
 * the world was created.
 **/
void
librdf_log(librdf_world* world, int code, 
           librdf_log_level level, librdf_log_facility facility,
           void *locator, const char *message, ...)
{
  va_list arguments;
  char *buffer;
  
  va_start(arguments, message);

  buffer=raptor_vsnprintf(message, arguments);
  librdf_log_simple(world, code, level, facility, locator, buffer);
  raptor_free_memory(buffer);

  va_end(arguments);
}

/*
 * librdf_fatal - Fatal error - Internal
 * @world: redland world object or NULL
 * @message: message arguments
 *
 * If world is NULL, the error ocurred in redland startup before
 * the world was created.
 **/
void
librdf_fatal(librdf_world* world, int facility,
             const char *file, int line, const char *function,
             const char *message)
{
  char empty_buffer[1];
  char *buffer;

  /* Not passing NULL to snprintf since that seems to not be portable  */
  size_t length=snprintf(empty_buffer, 1, "%s:%d:%s: fatal error: %s", 
                         file, line, function, message);

  buffer=(char*)LIBRDF_MALLOC(cstring, length+1);
  if(!buffer)
    return;
  
  snprintf(buffer, length, "%s:%d:%s: fatal error: %s", 
           file, line, function, message);
  librdf_log(world, 0, LIBRDF_LOG_FATAL, facility, NULL, buffer);
  LIBRDF_FREE(cstring, buffer);
  abort();
}


/* prototypes for testing errors only - NOT PART OF API */
void
librdf_test_error(librdf_world* world, const char *message) 
{
  librdf_log_simple(world, 0, LIBRDF_LOG_ERROR, LIBRDF_FROM_NONE, NULL, message);
}

void
librdf_test_warning(librdf_world* world, const char *message)
{
  librdf_log_simple(world, 0, LIBRDF_LOG_WARN, LIBRDF_FROM_NONE, NULL, message);
}
