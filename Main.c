/* Main.c
   $Id: Main.c,v 1.5 2005/01/30 14:41:22 joty Exp $

   Copyright (c) 2003-2005 Dave Appleby / John Tytgat

   This file is part of CCres.

   CCres is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   CCres is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with CCres; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* Std C headers :
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* OSLib headers :
 */
#include <OSLib/proginfo.h>
#include <OSLib/saveas.h>
#include <OSLib/taskwindow.h>
#include <OSLib/wimpreadsysinfo.h>

/* Project headers :
 */
#include "ccres.h"
#include "Error.h"

#define APPDIR	"<"APPNAME"$Dir>"

#define action_MENU_QUIT	0x01

const char achProgName[] = APPNAME;
int returnStatus = EXIT_SUCCESS;

// Toolbox action codes returned by WimpPoll - each action has an associated handler
static const toolbox_action_list Action[] =
  {
  {{action_MENU_QUIT}},
  {{action_SAVE_AS_SAVE_TO_FILE}},
  {{action_SAVE_AS_SAVE_COMPLETED}},
  {{action_ERROR}},
  {{0}}
  };

// List of handler functions for the Toolbox actions - each handler corresponds to the action in the list above
static const action_handler Handler[] =
  {
  menu_quit,
  action_save_to_file,
  action_save_completed,
  toolbox_error
  };

// Wimp messages (except message_QUIT) that we want to receive
static const wimp_message_list Message[] =
  {
  {{message_DATA_SAVE}},
  {{message_DATA_LOAD}},
  {{0}}
  };


static  BOOL ccres_initialise(PDATA data)
//      =================================
{
data->idBaricon = toolbox_create_object(0, (toolbox_id) "Iconbar");
data->idSaveAs  = toolbox_create_object(0, (toolbox_id) "SaveAs");
proginfo_set_version(0, toolbox_create_object(0, (toolbox_id) "ProgInfo"), VERSION);

return TRUE;
}


static  void ccres_pollloop(PDATA data)
//      ===============================
{
  wimp_event_no e;
  int a;
  bits nAction;

do {
  if ((e = wimp_poll(wimp_MASK_NULL | wimp_MASK_LEAVING | wimp_MASK_ENTERING, &data->poll.wb, 0)) == toolbox_EVENT)
    {
    // is it a toolbox event?
    // if so, look-up the action number in the Action list, then call the associated handler
    nAction = data->poll.ta.action_no;
    for (a = 0; a < ELEMENTS(Handler); a++)
      {
      if (nAction == Action[a].action_nos[0])
        {
        Handler[a](data);
        break;
        }
      }
    }
  else if (e == wimp_USER_MESSAGE || e == wimp_USER_MESSAGE_RECORDED)
    {
    if ((nAction = data->poll.wb.message.action) == message_QUIT || nAction == message_SHUTDOWN)
      data->fRunning = FALSE;
    else if (nAction == message_DATA_SAVE)
      message_data_save(data);
    else if (nAction == message_DATA_LOAD)
      message_data_load(data);
    }
  } while (data->fRunning);
}


        int main(int argc, PSTR argv[])
//      ===============================
{
  static char achSyntax[] = "Wrong number of arguments - Syntax: !CCres <infile> <outfile>";
  bits nFileType;
  int nVersion;
  messagetrans_control_block cb;
  DATA data;
  os_error * perr;

memset(&data, 0, sizeof(data));
if (argc == 1
    && wimpreadsysinfo_desktop_state() == wimpreadsysinfo_STATE_DESKTOP
    && !taskwindowtaskinfo_window_task())
  {
  if (!is_running())
    {
    log_on();
    MyAlloc_Init();
    if ((perr = xtoolbox_initialise(0, 310, Message, Action, APPDIR, &cb, &data.tb, &nVersion, &data.task, &data.pSprites)) != NULL)
      error("%s  OR if using from the command line  %s", perr->errmess, achSyntax);
    else
      {
      data.fRunning = ccres_initialise(&data);
      ccres_pollloop(&data);
      wimp_close_down(data.task);
      }

    if (data.pszIn != NULL)
      MyFree(data.pszIn);
    MyAlloc_Report();
    }
  }
else if (argc == 3)
  {
  log_on();
  MyAlloc_Init();
  if (((nFileType = my_osfile_filetype(argv[1])) == osfile_TYPE_TEXT
       || nFileType == osfile_TYPE_RESOURCE
       || nFileType == osfile_TYPE_TEMPLATE)
      && load_file(&data, argv[1], nFileType))
    convert(&data, argv[2]);
  else if (nFileType == osfile_TYPE_UNTYPED)
    error("File not found '%s'", argv[1]);
  else
    error("Not a valid file type '%s'", argv[1]);

  if (data.pszIn != NULL)
    MyFree(data.pszIn);

  MyAlloc_Report();
  }
else
  {
  fprintf(stderr, "CCres " VERSION "\n"
                  "Convertor between RISC OS Toolbox Resource (filetype &FAE) & Wimp Template (filetype &FEC) files to and from text format.\n"
                  "Syntax: CCres <infile> <outfile>\n"
                  "  <infile>  : either Template, Resource or Text file\n"
                  "  <outfile> : output Text file (case <infile> is a Template or Resource file) or output Template/Resource file (case <infile> is a Text file)\n");
  return EXIT_FAILURE;
  }

return returnStatus;
}
