/*
 * Copyright (c) 2010  Axel Neumann
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <json/json.h>


#include "bmx.h"
#include "plugin.h"
#include "schedule.h"
#include "tools.h"
#include "ip.h"
#include "json.h"


#define CODE_CATEGORY_NAME "json"


STATIC_FUNC
int32_t opt_json_test(uint8_t cmd, uint8_t _save, struct opt_type *opt, struct opt_parent *patch, struct ctrl_node *cn)
{

	if ( cmd != OPT_APPLY )
		return SUCCESS;
        if (!cn)
		return FAILURE;

        static char test[10];

        json_object * jobj = json_object_new_object();
        json_object *jopts = json_object_new_array();


        json_object *jopt;
        json_object *jopt_name;

        sprintf(test, "A");
        jopt = json_object_new_object();
        jopt_name = json_object_new_string(test);
        json_object_object_add(jopt, "name", jopt_name);
        json_object_array_add(jopts, jopt);

        sprintf(test, "B");
        jopt = json_object_new_object();
        jopt_name = json_object_new_string(test);
        json_object_object_add(jopt, "name", jopt_name);
        json_object_array_add(jopts, jopt);

        sprintf(test, "C");

        json_object_object_add(jobj, "options", jopts);

        dbg_printf(cn, "%s\n", json_object_to_json_string(jobj));

        json_object_put(jobj);

	return SUCCESS;
}

STATIC_FUNC
int32_t opt_json_help(uint8_t cmd, uint8_t _save, struct opt_type *opt, struct opt_parent *patch, struct ctrl_node *cn)
{

	if ( cmd != OPT_APPLY )
		return SUCCESS;
	if ( !cn  )
		return FAILURE;


        struct opt_type * p_opt = NULL;
        json_object * jobj = json_object_new_object();
        json_object *jopts = json_object_new_array();

        while ((p_opt = list_iterate(&opt_list, p_opt))) {

                if (p_opt->parent_name)
                        continue;

                assertion(-501240, (p_opt->long_name));

                json_object *jopt = json_object_new_object();

                json_object *jopt_name = json_object_new_string(p_opt->long_name);
                json_object_object_add(jopt, "name", jopt_name);

                if (p_opt->opt_t != A_PS0 && p_opt->imin != p_opt->imax) {
                        json_object *jopt_min = json_object_new_int(p_opt->imin);
                        json_object_object_add(jopt, "min", jopt_min);
                        json_object *jopt_max = json_object_new_int(p_opt->imax);
                        json_object_object_add(jopt, "max", jopt_max);
                        json_object *jopt_def = json_object_new_int(p_opt->idef);
                        json_object_object_add(jopt, "def", jopt_def);

                } else if (p_opt->sdef) {
                        json_object *jopt_def = json_object_new_string(p_opt->sdef);
                        json_object_object_add(jopt, "def", jopt_def);
                }

                if (p_opt->syntax) {
                        json_object *jopt_syntax = json_object_new_string(p_opt->syntax);
                        json_object_object_add(jopt, "syntax", jopt_syntax);
                }

                if (p_opt->help) {
                        json_object *jopt_help = json_object_new_string(p_opt->help);
                        json_object_object_add(jopt, "help", jopt_help);
                }

                if (p_opt->d.childs_type_list.items) {
                        struct opt_type *c_opt = NULL;
                        json_object *jchilds = json_object_new_array();

                        while ((c_opt = list_iterate(&p_opt->d.childs_type_list, c_opt))) {

                                assertion(-501241, (c_opt->parent_name && c_opt->long_name));

                                json_object *jchild = json_object_new_object();

                                json_object *jopt_name = json_object_new_string(p_opt->long_name);
                                json_object_object_add(jchild, "name", jopt_name);



                                json_object_array_add(jchilds, jchild);
                        }
                        json_object_object_add(jopt, "child_options", jchilds);
                }
                json_object_array_add(jopts, jopt);
        }

        json_object_object_add(jobj, "options", jopts);

        dbg_printf(cn, "%s\n", json_object_to_json_string(jobj));

        json_object_put(jobj);


	if ( initializing )
		cleanup_all(CLEANUP_SUCCESS);

	return SUCCESS;
}

static struct opt_type json_options[]= {
//        ord parent long_name          shrt Attributes				*ival		min		max		default		*func,*syntax,*help
	
	{ODI,0,ARG_JSON_HELP,		0,0,A_PS0,A_USR,A_DYI,A_ARG,A_END,	0,		0, 		0,		0,0, 		opt_json_help,
			0,		"summarize available parameters and options"}
                        ,
	{ODI,0,ARG_JSON_TEST,		0,0,A_PS0,A_USR,A_DYI,A_ARG,A_END,	0,		0, 		0,		0,0, 		opt_json_test,
			0,		"test json options"}

	
};


static void json_cleanup( void ) {
	
	
}

static int32_t json_init( void ) {

        register_options_array(json_options, sizeof ( json_options), CODE_CATEGORY_NAME);
	
	return SUCCESS;
	
}


struct plugin* get_plugin( void ) {
	
	static struct plugin json_plugin;
	
	memset( &json_plugin, 0, sizeof ( struct plugin ) );
	

	json_plugin.plugin_name = CODE_CATEGORY_NAME;
	json_plugin.plugin_size = sizeof ( struct plugin );
        json_plugin.plugin_code_version = CODE_VERSION;
	json_plugin.cb_init = json_init;
	json_plugin.cb_cleanup = json_cleanup;
	
	return &json_plugin;
}

