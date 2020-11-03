/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/
#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <QList>
#include <QtGlobal>

/* utility */
#include "fciconv.h"
#include "fcintl.h"
#include "support.h"

#include "fc_cmdhelp.h"

struct cmdarg {
  char shortarg;
  char *longarg;
  char *helpstr;
};

struct cmdhelp {
  char *cmdname;
  QList<cmdarg *> *cmdarglist;
};

static struct cmdarg *cmdarg_new(const char *shortarg, const char *longarg,
                                 const char *helpstr);
static void cmdarg_destroy(struct cmdarg *pcmdarg);

/*************************************************************************/ /**
   Create a new command help struct.
 *****************************************************************************/
struct cmdhelp *cmdhelp_new(const char *cmdname)
{
  struct cmdhelp *pcmdhelp = new cmdhelp[1]();

  pcmdhelp->cmdname = qstrdup(fc_basename(cmdname));
  pcmdhelp->cmdarglist = new QList<cmdarg *>;

  return pcmdhelp;
}

/*************************************************************************/ /**
   Destroy a command help struct.
 *****************************************************************************/
void cmdhelp_destroy(struct cmdhelp *pcmdhelp)
{
  if (pcmdhelp) {
    if (pcmdhelp->cmdname) {
      delete[] pcmdhelp->cmdname;
    }
    for (auto pcmdarg : qAsConst(*pcmdhelp->cmdarglist)) {
      cmdarg_destroy(pcmdarg);
    }
  }
  delete pcmdhelp->cmdarglist;
  delete[] pcmdhelp;
}

/*************************************************************************/ /**
   Add a command help moption.
 *****************************************************************************/
void cmdhelp_add(struct cmdhelp *pcmdhelp, const char *shortarg,
                 const char *longarg, const char *helpstr, ...)
{
  va_list args;
  char buf[512];
  struct cmdarg *pcmdarg;

  va_start(args, helpstr);
  fc_vsnprintf(buf, sizeof(buf), helpstr, args);
  va_end(args);

  pcmdarg = cmdarg_new(shortarg, longarg, buf);
  pcmdhelp->cmdarglist->append(pcmdarg);
}

/*************************************************************************/ /**
   Helper sort function in alphabetical order
 *****************************************************************************/
static bool lettersort(cmdarg *i, cmdarg *j)
{
  return tolower(i->shortarg) < tolower(j->shortarg);
}
/*************************************************************************/ /**
   Display the help for the command.
 *****************************************************************************/
void cmdhelp_display(struct cmdhelp *pcmdhelp, bool sort, bool gui_options,
                     bool report_bugs)
{
  fc_fprintf(stderr, _("Usage: %s [option ...]\nValid option are:\n"),
             pcmdhelp->cmdname);

  std::sort(pcmdhelp->cmdarglist->begin(), pcmdhelp->cmdarglist->end(),
            lettersort);
  for (auto pcmdarg : qAsConst(*pcmdhelp->cmdarglist)) {
    if (pcmdarg->shortarg != '\0') {
      fc_fprintf(stderr, "  -%c, --%-15s %s\n", pcmdarg->shortarg,
                 pcmdarg->longarg, pcmdarg->helpstr);
    } else {
      fc_fprintf(stderr, "      --%-15s %s\n", pcmdarg->longarg,
                 pcmdarg->helpstr);
    }
  }

  if (gui_options) {
    char buf[128];

    fc_snprintf(buf, sizeof(buf), _("Try \"%s -- --help\" for more."),
                pcmdhelp->cmdname);

    /* The nearly empty strings in the two functions below have to be adapted
     * if the format of the command argument list above is changed.*/
    fc_fprintf(stderr, "      --                %s\n",
               _("Pass any following options to the UI."));
    fc_fprintf(stderr, "                        %s\n", buf);
  }

  if (report_bugs) {
    /* TRANS: No full stop after the URL, could cause confusion. */
    fc_fprintf(stderr, _("Report bugs at %s\n"), BUG_URL);
  }
}

/*************************************************************************/ /**
   Create a new command argument struct.
 *****************************************************************************/
static struct cmdarg *cmdarg_new(const char *shortarg, const char *longarg,
                                 const char *helpstr)
{
  struct cmdarg *pcmdarg = new cmdarg[1]();

  if (shortarg && strlen(shortarg) == 1) {
    pcmdarg->shortarg = shortarg[0];
  } else {
    /* '\0' means no short argument for this option. */
    pcmdarg->shortarg = '\0';
  }
  pcmdarg->longarg = qstrdup(longarg);
  pcmdarg->helpstr = qstrdup(helpstr);

  return pcmdarg;
}

/*************************************************************************/ /**
   Destroy a command argument struct.
 *****************************************************************************/
static void cmdarg_destroy(struct cmdarg *pcmdarg)
{
  if (pcmdarg) {
    if (pcmdarg->longarg) {
      delete[] pcmdarg->longarg;
    }
    if (pcmdarg->helpstr) {
      delete[] pcmdarg->helpstr;
    }
  }
  delete[] pcmdarg;
}
