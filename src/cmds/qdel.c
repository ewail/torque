#include "license_pbs.h" /* See here for the software license */
/*
 *
 * qdel - (PBS) delete batch job
 *
 * Authors:
 *      Terry Heidelberg
 *      Livermore Computing
 *
 *      Bruce Kelly
 *      National Energy Research Supercomputer Center
 *
 *      Lawrence Livermore National Laboratory
 *      University of California
 */

#include "cmds.h"
#include "net_cache.h"
#include <pbs_config.h>   /* the master config generated by configure */
#include "../lib/Libifl/lib_ifl.h"

/* qdel */

int main(

  int    argc,
  char **argv)

  {
  int c;
  int errflg = 0;
  int any_failed = 0;
  int purge_completed = FALSE;
  int located = FALSE;
  char *pc;
  bool hasbracket = false;
  bool isarray = false;

  char job_id[PBS_MAXCLTJOBID]; /* from the command line */

  char job_id_out[PBS_MAXCLTJOBID];
  char server_out[MAXSERVERNAME] = "";
  char rmt_server[MAXSERVERNAME] = "";

  char extend[1024];

#define GETOPT_ARGS "acm:pW:t:"

  extend[0] = '\0';
  
  int brackcount;
  // brackcount used to check for brackets in case of -t

  while ((c = getopt(argc, argv, GETOPT_ARGS)) != EOF)
    {
    switch (c)
      {

      case 'a': /* Async job deletion */

        if (extend[0] != '\0')
          {
          errflg++;

          break;
          }

        snprintf(extend, sizeof(extend), "%s", DELASYNC);

        break;

      case 'c':

        if (extend[0] != '\0')
          {
          errflg++;

          break;
          }

        snprintf(extend,sizeof(extend),"%s%ld",PURGECOMP,(long)(time(NULL)));
        purge_completed = TRUE;

        break;

      case 'm':

        /* add delete message */

        if (extend[0] != '\0')
          {
          /* extension option already specified */

          errflg++;

          break;
          }

        snprintf(extend, sizeof(extend), "%s", optarg);

        break;

      case 'p':

        if (extend[0] != '\0')
          {
          errflg++;

          break;
          }

        snprintf(extend, sizeof(extend), "%s1", DELPURGE);

        break;

      case 't':

        if (extend[0] != '\0')
          {
          errflg++;

          break;
          }
        
        pc = optarg;

        if (strlen(pc) == 0)
          {
          fprintf(stderr, "qdel: illegal -t value (array range cannot be zero length)\n");

          errflg++;

          break;
          }

        snprintf(extend,sizeof(extend),"%s%s",
          ARRAY_RANGE,
          pc);

        break;

      case 'W':

        if (extend[0] != '\0')
          {
          errflg++;

          break;
          }

        pc = optarg;

        if (strlen(pc) == 0)
          {
          fprintf(stderr, "qdel: illegal -W value\n");

          errflg++;

          break;
          }

        while (*pc != '\0')
          {
          if (!isdigit(*pc))
            {
            fprintf(stderr, "qdel: illegal -W value\n");

            errflg++;

            break;
            }

          pc++;
          }

        snprintf(extend, sizeof(extend), "%s%s", DELDELAY, optarg);

        break;

      default:
    	
        errflg++;

        break;
      }
    }    /* END while (c) */

  if (purge_completed)
    {
    snprintf(server_out, sizeof(server_out), "%s", pbs_default());
    goto cnt;
    }
  
  if ((errflg != 0) || (optind >= argc))
    {
    static char usage[] = "usage: qdel [{ -a | -c | -p | -t | -W delay | -m message}] [<JOBID>[<JOBID>]|'all'|'ALL']...\n";

    fprintf(stderr, "%s", usage);

    fprintf(stderr, "       -a -c, -m, -p, -t, and -W are mutually exclusive\n");

    exit(2);
    }

  for (;optind < argc;optind++)
    {
    int connect;
    int stat;

    /* check to see if user specified 'all' to delete all jobs */

   snprintf(job_id, sizeof(job_id), "%s", argv[optind]);
   
   for (int i = 0; i < strlen(job_id); i++)
     {
	   
     if (job_id[i] == '[' || job_id[i] == ']')
       { 
       isarray = true;
       
       hasbracket = true;
       
       brackcount++;
       }
     
     if (job_id[i] == 't')
       {
       isarray = TRUE; 
       }
    	 
     }
   
   if (isarray == TRUE && brackcount != 2)
     {
     fprintf(stderr, "qdel: illegally formed array identifier: %s\n",
         job_id);
     
     any_failed = 1;
     
     exit(any_failed);
     }
   
   if (get_server(job_id, job_id_out, sizeof(job_id_out), server_out, sizeof(server_out)))
     {
     fprintf(stderr, "qdel: illegally formed job identifier: %s\n",
             job_id);
      
     any_failed = 1;

     exit(any_failed);
     }

cnt:

    connect = cnt2server(server_out);

    if (connect <= 0)
      {
      any_failed = -1 * connect;

      if(server_out[0] != 0)
        fprintf(stderr, "qdel: cannot connect to server %s (errno=%d) %s\n",
              server_out,
              any_failed,
              pbs_strerror(any_failed));
      else
        fprintf(stderr, "qdel: cannot connect to server %s (errno=%d) %s\n",
              pbs_server,
              any_failed,
              pbs_strerror(any_failed));

      continue;
      }

    stat = pbs_deljob_err(connect, job_id_out, extend, &any_failed);

    if (stat &&
        (any_failed != PBSE_UNKJOBID))
      {
      if (!located)
        {
        located = TRUE;

        if (locate_job(job_id_out, server_out, rmt_server))
          {
          pbs_disconnect(connect);

          strcpy(server_out, rmt_server);

          goto cnt;
          }

        }
        
      prt_job_err((char *)"qdel", connect, job_id_out);
      }
    
    if (!located && any_failed != 0)
      {
      fprintf(stderr, "qdel: nonexistent job id: %s\n", job_id);
      // need to change to PBSE_ERROR.db
      }

    pbs_disconnect(connect);
    }

  exit(any_failed);
  }  /* END main() */


