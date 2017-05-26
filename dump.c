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
#include <stdarg.h>
#include <string.h>


#include "bmx.h"
#include "msg.h"
#include "ip.h"
#include "dump.h"
#include "schedule.h"
#include "plugin.h"
#include "tools.h"

#define CODE_CATEGORY_NAME "traffic"

#define IMPROVE_ROUNDOFF 10

static int64_t curr_dump_period = DEF_DUMP_PERIOD;
static int32_t next_dump_period = DEF_DUMP_PERIOD;
static int32_t prev_dump_period = DEF_DUMP_PERIOD;

static int32_t dump_regression_exp = DEF_DUMP_REGRESSION_EXP;
static uint32_t dump_iteration = 0;

static IDM_T dump_terminating = NO;
static int32_t data_dev_plugin_registry = FAILURE;

static struct dump_data dump_all;


STATIC_FUNC
void update_traffic_statistics_data(struct dump_data *data)
{
        uint16_t i, t;

        for (i = 0; i < DUMP_DIRECTION_ARRSZ; i++) {

                for (t = 0; t < DUMP_TYPE_ARRSZ; t++) {
                        data->pre_all[i][t] = (((int64_t)(data->tmp_all[i][t])) * 1000) / curr_dump_period;

                        data->avg_all[i][t] -= (data->avg_all[i][t] >> dump_regression_exp);
                        data->avg_all[i][t] += ((data->pre_all[i][t]) >> dump_regression_exp);
                        data->tmp_all[i][t] = 0;
                }


                for (t = 0; t < FRAME_TYPE_ARRSZ; t++) {
                        data->pre_frame[i][t] = (((int64_t)(data->tmp_frame[i][t])) * 1000) / curr_dump_period;

                        data->avg_frame[i][t] -= (data->avg_frame[i][t] >> dump_regression_exp);
                        data->avg_frame[i][t] += ((data->pre_frame[i][t]) >> dump_regression_exp);
                        data->tmp_frame[i][t] = 0;
                }
        }
}


STATIC_FUNC
void update_traffic_statistics_task(void *data)
{
        struct dev_node *dev;
        struct avl_node *an = NULL;

	dump_iteration++;
	curr_dump_period = prev_dump_period;

        update_traffic_statistics_data( &dump_all );

        while ((dev = avl_iterate_item(&dev_name_tree, &an))) {

                struct dump_data **dump_dev_plugin_data =
                        (struct dump_data **) (get_plugin_data(dev, PLUGIN_DATA_DEV, data_dev_plugin_registry));

                if (*dump_dev_plugin_data)
                        update_traffic_statistics_data(*dump_dev_plugin_data);

        }

        task_register(next_dump_period, update_traffic_statistics_task, NULL, -300348);
	prev_dump_period = next_dump_period;

}

STATIC_FUNC
void dump(struct packet_buff *pb)
{

        assertion(-500760, (XOR((pb->i.oif), (pb->i.iif))));

        IDM_T direction = pb->i.iif ? DUMP_DIRECTION_IN : DUMP_DIRECTION_OUT;
        struct dev_node *dev = pb->i.iif ? pb->i.iif : pb->i.oif;
        struct packet_header *phdr = &pb->packet.header;

        struct dump_data **dev_plugin_data =
                (struct dump_data **) (get_plugin_data(dev, PLUGIN_DATA_DEV, data_dev_plugin_registry));

        assertion(-500761, (*dev_plugin_data));

        uint16_t plength = ntohs(phdr->pkt_length);

        dbgf_dump(DBGT_NONE, "%s srcIP=%-16s dev=%-12s udpPayload=%-d",
                direction == DUMP_DIRECTION_IN ? "in " : "out", pb->i.llip_str, dev->label_cfg.str, plength);

        dbgf_dump(DBGT_NONE, "%s data: %s",
                direction == DUMP_DIRECTION_IN ? "in " : "out", memAsHexString(((uint8_t*) phdr), plength));

        dbgf_dump(DBGT_NONE, "          bmx_version     pkt_sqn local_id  next frame...");
        dbgf_dump(DBGT_NONE, "            reserved  link_adv_sqn        dev_idx");
        dbgf_dump(DBGT_NONE, "              pkt_length");
        dbgf_dump(DBGT_NONE, "                  transmitterIID");



        (*dev_plugin_data)->tmp_all[direction][DUMP_TYPE_UDP_PAYLOAD] += (plength << IMPROVE_ROUNDOFF);

        dump_all.tmp_all[direction][DUMP_TYPE_UDP_PAYLOAD] += (plength << IMPROVE_ROUNDOFF);


        dbgf_dump(DBGT_NONE,
                "%s       headerVersion=%-2d reserved=%-2X IID=%-5d headerSize=%-4zu",
                direction == DUMP_DIRECTION_IN ? "in " : "out",
                phdr->comp_version, phdr->capabilities, ntohs(phdr->transmitterIID), sizeof (struct packet_header));

        (*dev_plugin_data)->tmp_all[direction][DUMP_TYPE_PACKET_HEADER] += (sizeof (struct packet_header) << IMPROVE_ROUNDOFF);

        dump_all.tmp_all[direction][DUMP_TYPE_PACKET_HEADER] += (sizeof (struct packet_header) << IMPROVE_ROUNDOFF);


        struct rx_frame_iterator it = {
                .caller = __FUNCTION__, .on = NULL, .cn = NULL, .op = 0, .pb = NULL,
                .handls = packet_frame_handler, .handl_max = FRAME_TYPE_MAX, .process_filter = FRAME_TYPE_PROCESS_NONE,
                .data = ((uint8_t*) phdr), .frame_type = -1,
                .frames_in = (((uint8_t*) phdr) + sizeof (struct packet_header)),
                .frames_length = (plength - sizeof (struct packet_header)), .frames_pos = 0
        };

        int32_t iterator_result;
        uint16_t frame_length = 0;
        uint16_t pkt_pos = sizeof (struct packet_header);

        while ((iterator_result = rx_frame_iterate(&it)) > TLV_RX_DATA_DONE) {

                char tnum[4];
                char *tname;
                int16_t frame_msgs = -1;

                if (!(tname = packet_frame_handler[it.frame_type].name)) {
                        sprintf(tnum, "%d", it.frame_type);
                        tname = tnum;
                }

                frame_length = it.frame_data_length +
                        (it.is_short_header ? sizeof (struct frame_header_short) : sizeof (struct frame_header_long));

                if (packet_frame_handler[it.frame_type].fixed_msg_size) {
                        frame_msgs = it.frame_msgs_length / packet_frame_handler[it.frame_type].min_msg_size;
                }


                dbgf_dump(DBGT_NONE, "%s             frame_type=%-12s data_header_size=%-3d msgs=%-3d frame_length=%-4d",
                        direction == DUMP_DIRECTION_IN ? "in " : "out", tname,
                        packet_frame_handler[it.frame_type].data_header_size, frame_msgs, frame_length);

                dbgf_dump(DBGT_NONE, "%s         data [%3d...%3d]:%s",
                        direction == DUMP_DIRECTION_IN ? "in  hex" : "out hex",
                        pkt_pos, pkt_pos + frame_length - 1, memAsHexString(((uint8_t*) phdr) + pkt_pos, frame_length));

                //assertion(-500991, (packet_frame_handler[it.frame_type].min_msg_size));
                assertion(-500992, (it.frame_msgs_length >= 0));

                (*dev_plugin_data)->tmp_frame[direction][it.frame_type] += (frame_length << IMPROVE_ROUNDOFF);

                dump_all.tmp_frame[direction][it.frame_type] += (frame_length << IMPROVE_ROUNDOFF);

                (*dev_plugin_data)->tmp_all[direction][DUMP_TYPE_FRAME_HEADER] += ((frame_length - it.frame_data_length) << IMPROVE_ROUNDOFF);

                dump_all.tmp_all[direction][DUMP_TYPE_FRAME_HEADER] += ((frame_length - it.frame_data_length) << IMPROVE_ROUNDOFF);

                pkt_pos += frame_length;
        }

        if (iterator_result != TLV_RX_DATA_DONE) {

                frame_length = it.frame_data_length +
                        (it.is_short_header ? sizeof (struct frame_header_short) : sizeof (struct frame_header_long));

                dbgf_dump(DBGT_NONE, "%s             ERROR frame_type=%d frame_length=%d frame_data_lenghth=%d short_header=%d iterator_result=%d - ignoring further frames!!",
                        direction == DUMP_DIRECTION_IN ? "in " : "out", it.frame_type, frame_length, it.frame_data_length, it.is_short_header, iterator_result);

                dbgf_dump(DBGT_NONE, "%s         data [%3d...%3d]:%s",
                        direction == DUMP_DIRECTION_IN ? "in  hex" : "out hex",
                        pkt_pos, pkt_pos + frame_length - 1, memAsHexString(((uint8_t*) phdr) + pkt_pos, frame_length));
        }

        assertion(-500990, (IMPLIES(direction == DUMP_DIRECTION_OUT, iterator_result == TLV_RX_DATA_DONE)));

}


STATIC_FUNC
void dbg_traffic_statistics(struct dump_data *data, struct ctrl_node *cn, char* dbg_name)
{
        uint16_t t;

        dbg_printf(cn, "%-20s \n", dbg_name);


        for (t = 0; t < DUMP_TYPE_ARRSZ; t++) {
                dbg_printf(cn, "%13s  %5d (%3d)  %5d (%3d)  %5d (%3d)  | %5d (%3d)  %5d (%3d)  %5d (%3d)\n",
                        t == DUMP_TYPE_UDP_PAYLOAD ? "UDP_PAYLOAD" : (t == DUMP_TYPE_PACKET_HEADER ? "PACKET_HEADER" : "FRAME_HEADER"),


                        ((data->pre_all[DUMP_DIRECTION_IN][t] + data->pre_all[DUMP_DIRECTION_OUT][t]) >> IMPROVE_ROUNDOFF),

                        ((((data->pre_all[DUMP_DIRECTION_IN][DUMP_TYPE_UDP_PAYLOAD]) || (data->pre_all[DUMP_DIRECTION_OUT][DUMP_TYPE_UDP_PAYLOAD])) ?
                        (((data->pre_all[DUMP_DIRECTION_IN][t] + data->pre_all[DUMP_DIRECTION_OUT][t])*100) /
                        (data->pre_all[DUMP_DIRECTION_IN][DUMP_TYPE_UDP_PAYLOAD] + data->pre_all[DUMP_DIRECTION_OUT][DUMP_TYPE_UDP_PAYLOAD])) : 0)),


                        ((data->pre_all[DUMP_DIRECTION_IN][t]) >> IMPROVE_ROUNDOFF),

                        ((data->pre_all[DUMP_DIRECTION_IN][DUMP_TYPE_UDP_PAYLOAD] ?
                        ((data->pre_all[DUMP_DIRECTION_IN][t]*100) / data->pre_all[DUMP_DIRECTION_IN][DUMP_TYPE_UDP_PAYLOAD]) : 0)),


                        ((data->pre_all[DUMP_DIRECTION_OUT][t]) >> IMPROVE_ROUNDOFF),

                        ((data->pre_all[DUMP_DIRECTION_OUT][DUMP_TYPE_UDP_PAYLOAD] ?
                        ((data->pre_all[DUMP_DIRECTION_OUT][t]*100) / data->pre_all[DUMP_DIRECTION_OUT][DUMP_TYPE_UDP_PAYLOAD]) : 0)),



                        ((data->avg_all[DUMP_DIRECTION_IN][t] + data->avg_all[DUMP_DIRECTION_OUT][t]) >> IMPROVE_ROUNDOFF),

                        (((data->avg_all[DUMP_DIRECTION_IN][DUMP_TYPE_UDP_PAYLOAD] || data->avg_all[DUMP_DIRECTION_OUT][DUMP_TYPE_UDP_PAYLOAD]) ?
                        (((data->avg_all[DUMP_DIRECTION_IN][t] + data->avg_all[DUMP_DIRECTION_OUT][t])*100) /
                        (data->avg_all[DUMP_DIRECTION_IN][DUMP_TYPE_UDP_PAYLOAD] + data->avg_all[DUMP_DIRECTION_OUT][DUMP_TYPE_UDP_PAYLOAD])) : 0)),


                        ((data->avg_all[DUMP_DIRECTION_IN][t]) >> IMPROVE_ROUNDOFF),

                        ((data->avg_all[DUMP_DIRECTION_IN][DUMP_TYPE_UDP_PAYLOAD] ?
                        ((data->avg_all[DUMP_DIRECTION_IN][t]*100) / data->avg_all[DUMP_DIRECTION_IN][DUMP_TYPE_UDP_PAYLOAD]) : 0)),


                        ((data->avg_all[DUMP_DIRECTION_OUT][t]) >> IMPROVE_ROUNDOFF),

                        ((data->avg_all[DUMP_DIRECTION_OUT][DUMP_TYPE_UDP_PAYLOAD] ?
                        ((data->avg_all[DUMP_DIRECTION_OUT][t]*100) / data->avg_all[DUMP_DIRECTION_OUT][DUMP_TYPE_UDP_PAYLOAD]) : 0))

                        );
        }

        for (t = 0; t < FRAME_TYPE_NOP; t++) {

                if (packet_frame_handler[t].name || data->avg_frame[DUMP_DIRECTION_IN][t] || data->avg_frame[DUMP_DIRECTION_OUT][t]) {

                        char tnum[4];
                        char *tname = packet_frame_handler[t].name;
                        if (!tname) {
                                sprintf(tnum, "%d", t);
                                tname = tnum;
                        }


                        dbg_printf(cn, "%13s  %5d (%3d)  %5d (%3d)  %5d (%3d)  | %5d (%3d)  %5d (%3d)  %5d (%3d)\n",
                                tname,

                                ((data->pre_frame[DUMP_DIRECTION_IN][t] + data->pre_frame[DUMP_DIRECTION_OUT][t]) >> IMPROVE_ROUNDOFF),

                                ((data->pre_all[DUMP_DIRECTION_IN][DUMP_TYPE_UDP_PAYLOAD] || data->pre_all[DUMP_DIRECTION_OUT][DUMP_TYPE_UDP_PAYLOAD] ?
                                (((data->pre_frame[DUMP_DIRECTION_IN][t] + data->pre_frame[DUMP_DIRECTION_OUT][t])*100) /
                                (data->pre_all[DUMP_DIRECTION_IN][DUMP_TYPE_UDP_PAYLOAD] + data->pre_all[DUMP_DIRECTION_OUT][DUMP_TYPE_UDP_PAYLOAD])) : 0)),


                                ((data->pre_frame[DUMP_DIRECTION_IN][t]) >> IMPROVE_ROUNDOFF),

                                ((data->pre_all[DUMP_DIRECTION_IN][DUMP_TYPE_UDP_PAYLOAD] ?
                                ((data->pre_frame[DUMP_DIRECTION_IN][t]*100) / data->pre_all[DUMP_DIRECTION_IN][DUMP_TYPE_UDP_PAYLOAD]) : 0)),


                                ((data->pre_frame[DUMP_DIRECTION_OUT][t]) >> IMPROVE_ROUNDOFF),

                                ((data->pre_all[DUMP_DIRECTION_OUT][DUMP_TYPE_UDP_PAYLOAD] ?
                                ((data->pre_frame[DUMP_DIRECTION_OUT][t]*100) / data->pre_all[DUMP_DIRECTION_OUT][DUMP_TYPE_UDP_PAYLOAD]) : 0)),



                                ((data->avg_frame[DUMP_DIRECTION_IN][t] + data->avg_frame[DUMP_DIRECTION_OUT][t]) >> IMPROVE_ROUNDOFF),

                                ((data->avg_all[DUMP_DIRECTION_IN][DUMP_TYPE_UDP_PAYLOAD] || data->avg_all[DUMP_DIRECTION_OUT][DUMP_TYPE_UDP_PAYLOAD] ?
                                (((data->avg_frame[DUMP_DIRECTION_IN][t] + data->avg_frame[DUMP_DIRECTION_OUT][t])*100) /
                                (data->avg_all[DUMP_DIRECTION_IN][DUMP_TYPE_UDP_PAYLOAD] + data->avg_all[DUMP_DIRECTION_OUT][DUMP_TYPE_UDP_PAYLOAD])) : 0)),


                                ((data->avg_frame[DUMP_DIRECTION_IN][t]) >> IMPROVE_ROUNDOFF),

                                ((data->avg_all[DUMP_DIRECTION_IN][DUMP_TYPE_UDP_PAYLOAD] ?
                                ((data->avg_frame[DUMP_DIRECTION_IN][t]*100) / data->avg_all[DUMP_DIRECTION_IN][DUMP_TYPE_UDP_PAYLOAD]) : 0)),


                                ((data->avg_frame[DUMP_DIRECTION_OUT][t]) >> IMPROVE_ROUNDOFF),

                                ((data->avg_all[DUMP_DIRECTION_OUT][DUMP_TYPE_UDP_PAYLOAD] ?
                                ((data->avg_frame[DUMP_DIRECTION_OUT][t]*100) / data->avg_all[DUMP_DIRECTION_OUT][DUMP_TYPE_UDP_PAYLOAD]) : 0))

                                );
                }
        }
}


STATIC_FUNC
int32_t opt_traffic_statistics(uint8_t cmd, uint8_t _save, struct opt_type *opt, struct opt_parent *patch, struct ctrl_node *cn)
{

	if ( cmd == OPT_APPLY ) {

                struct dev_node *dev;
                struct avl_node *an = NULL;

                dbg_printf(cn, "iteration=%d as [Bytes/sec], averaged over %.1f secs and as weighted averages\n",
                        dump_iteration, (float)curr_dump_period / 1000);

                dbg_printf(cn, "%20s ( %% )     in ( %% )    out ( %% )  |   all ( %% )     in ( %% )    out ( %% )\n"," ");

                if (!strcmp(patch->val, ARG_DUMP_ALL) || !strcmp(patch->val, ARG_DUMP_SUMMARY))
                        dbg_traffic_statistics(&dump_all, cn, ARG_DUMP_ALL);

                while ((dev = avl_iterate_item(&dev_name_tree, &an))) {

                        if (!strcmp(patch->val, ARG_DUMP_ALL) || !strcmp(patch->val, dev->label_cfg.str)) {

				struct dump_data **dump_dev_plugin_data = (struct dump_data **)
				(get_plugin_data(dev, PLUGIN_DATA_DEV, data_dev_plugin_registry));

				if (dev->active && *dump_dev_plugin_data)
					dbg_traffic_statistics(*dump_dev_plugin_data, cn, dev->label_cfg.str);
			}
                }

                dbg_printf(cn, "\n");


        }

        return SUCCESS;
}


STATIC_FUNC
struct opt_type dump_options[]=
{
//       ord parent long_name             shrt Attributes                            *ival              min                 max                default              *func,*syntax,*help

        {ODI, 0, ARG_DUMP_PERIOD,          0,  9,1, A_PS1, A_ADM, A_DYI, A_ARG, A_ANY, &next_dump_period,    MIN_DUMP_PERIOD,    MAX_DUMP_PERIOD,   DEF_DUMP_PERIOD,0,   0,
			ARG_VALUE_FORM,	"set duration in ms for how long traffic is measured for calculating interval averages"}
        ,
        {ODI, 0, ARG_DUMP_REGRESSION_EXP,  0,  9,1, A_PS1, A_ADM, A_DYI, A_ARG, A_ANY, &dump_regression_exp,MIN_DUMP_REGRESSION_EXP,MAX_DUMP_REGRESSION_EXP,DEF_DUMP_REGRESSION_EXP,0,0,
			ARG_VALUE_FORM,	"set regression exponent for traffic-dump statistics "}
        ,
	{ODI, 0, ARG_DUMP,     	           0,  9,2, A_PS1, A_USR, A_DYN, A_ARG, A_ANY, 0,                 0,                  0,                 0,0,                  opt_traffic_statistics,
			"<DEV>",		"show traffic statistics for given device name, summary, or all\n"}

};
STATIC_FUNC
void init_cleanup_dev_traffic_data(int32_t cb_id, void* devp)
{

        struct dev_node *dev;
        struct avl_node *an;


        for (an = NULL; (dev = avl_iterate_item(&dev_name_tree, &an));) {

                struct dump_data **dump_dev_plugin_data = (struct dump_data **) (get_plugin_data(dev, PLUGIN_DATA_DEV, data_dev_plugin_registry));

                if (dump_terminating || (!dev->active && *dump_dev_plugin_data)) {

                        if (*dump_dev_plugin_data)
                                debugFree(*dump_dev_plugin_data, -300305);

                        *dump_dev_plugin_data = NULL;


                } else if (dev->active && !(*dump_dev_plugin_data)) {

                        *dump_dev_plugin_data = debugMallocReset(sizeof (struct dump_data), -300306);
                }
        }
}



STATIC_FUNC
void cleanup_dump( void )
{
        dump_terminating = YES;
        init_cleanup_dev_traffic_data(0, NULL);
        set_packet_hook(dump, DEL);
}


STATIC_FUNC
int32_t init_dump( void )
{
        memset(&dump_all, 0, sizeof (struct dump_data));

        data_dev_plugin_registry = get_plugin_data_registry(PLUGIN_DATA_DEV);

        register_options_array(dump_options, sizeof ( dump_options), CODE_CATEGORY_NAME);

        task_register(next_dump_period, update_traffic_statistics_task, NULL, -300349);

        set_packet_hook(dump, ADD);

        return SUCCESS;
}



struct plugin *dump_get_plugin( void ) {

	static struct plugin dump_plugin;
	memset( &dump_plugin, 0, sizeof ( struct plugin ) );

	dump_plugin.plugin_name = CODE_CATEGORY_NAME;
	dump_plugin.plugin_size = sizeof ( struct plugin );
	dump_plugin.cb_init = init_dump;
	dump_plugin.cb_cleanup = cleanup_dump;
        dump_plugin.cb_plugin_handler[PLUGIN_CB_BMX_DEV_EVENT] = init_cleanup_dev_traffic_data;

        return &dump_plugin;
}

