#include <time.h>


/* Print out statistics on packets dropped */
static void print_stats(void){
	uint64_t total_packets_dropped, total_packets_tx, total_packets_rx;
  uint64_t curr_time, duration;
	unsigned portid;


	total_packets_dropped = 0;
	total_packets_tx = 0;
	total_packets_rx = 0;

	const char clr[] = { 27, '[', '2', 'J', '\0' };
	const char topLeft[] = { 27, '[', '1', ';', '1', 'H','\0' };

		/* Clear screen and move to top left */
	printf("%s%s", clr, topLeft);

	printf("\nDPDK Pingpong Client ====================================");

	for (portid = 0; portid < RTE_MAX_ETHPORTS; portid++) {
		/* skip disabled ports */
		if ((l2fwd_enabled_port_mask & (1 << portid)) == 0)
			continue;


		latency_diff = difftime( time(0), start);


		printf("\nByte statistics for port %u ------------------------------"
				 "\nPKT-SIZE: %d"
			   "\nByte received: %24"PRIu64
			   "\nLatency: %f",
			   portid,
				 PKT_SIZE, 
			   port_statistics[portid].rx_bytes,
				 latency_diff);

		// total_packets_dropped += port_statistics[portid].dropped;
		// total_packets_tx += port_statistics[portid].tx;
		// total_packets_rx += port_statistics[portid].rx;

		printf("\nPacket statistics for port %u ------------------------------"
			   "\nPacket sent: %24"PRIu64
			   "\nPacket received: %20"PRIu64
			   "\nPacket dropped: %21"PRIu64,
			   portid,
			   port_statistics[portid].tx,
			   port_statistics[portid].rx,
			   port_statistics[portid].dropped);

		// total_packets_dropped += port_statistics[portid].dropped;
		// total_packets_tx += port_statistics[portid].tx;
		// total_packets_rx += port_statistics[portid].rx;
	}


	// printf("\nPackets statistics ==============================="
	// 	   "\nPackets sent: %18"PRIu64
	// 	   "\nPackets received: %14"PRIu64
	// 	   "\nPackets dropped: %15"PRIu64
  //      "\nAggregated time (sec): %f",
	// 	   total_packets_tx,
	// 	   total_packets_rx,
	// 	   total_packets_dropped,
  //      latency_diff);
	printf("\n====================================================\n");
}


static void
l2fwd_mac_updating(struct rte_mbuf *m, unsigned dest_portid){
	struct ether_hdr *eth;
	void *tmp;

	eth = rte_pktmbuf_mtod(m, struct ether_hdr *);

	/* 02:00:00:00:00:xx */
	tmp = &eth->d_addr.addr_bytes[0];

	// a0:36:9f:83:ab:bc
	*((uint64_t *)tmp) = 0xd4d4a6211b00 + ((uint64_t)dest_portid << 40);

	/* src addr */
	ether_addr_copy(&l2fwd_ports_eth_addr[dest_portid], &eth->s_addr);

}



/* main processing loop */
static void l2fwd_main_loop(void){
    struct rte_mbuf *pkts_burst[MAX_PKT_BURST];
		struct rte_mbuf *rm[1];
  	struct rte_mbuf *m;
  	int sent;
  	unsigned lcore_id;
  	uint64_t prev_tsc, diff_tsc, cur_tsc, timer_tsc;
  	unsigned i, j, portid, nb_rx;
  	struct lcore_queue_conf *qconf;
  	const uint64_t drain_tsc = (rte_get_tsc_hz() + US_PER_S - 1) / US_PER_S * BURST_TX_DRAIN_US;
  	struct rte_eth_dev_tx_buffer *buffer;

    prev_tsc = 0;
  	timer_tsc = 0;

    lcore_id = rte_lcore_id();
  	qconf = &lcore_queue_conf[lcore_id];

  	if (qconf->n_rx_port == 0) {
  		RTE_LOG(INFO, L2FWD, "lcore %u has nothing to do\n", lcore_id);
  		return;
  	}

		time (&start); //useful call
		char *data, *rtn;

    while (!force_quit) {
        cur_tsc = rte_rdtsc();
        /*
         * TX burst queue drain
         */
        diff_tsc = cur_tsc - prev_tsc;
        if (unlikely(diff_tsc > drain_tsc)) {

            for (i = 0; i < qconf->n_rx_port; i++) {
                portid = l2fwd_dst_ports[qconf->rx_port_list[i]];
                buffer = tx_buffer[portid];
                sent = rte_eth_tx_buffer_flush(portid, 0, buffer);
                if (sent)
                  port_statistics[portid].tx += sent;
            }

            /* if timer is enabled */
      			if (timer_period > 0) {
      				/* advance the timer */
      				timer_tsc += diff_tsc;
      				/* if timer has reached its timeout */
      				if (unlikely(timer_tsc >= timer_period)) {
      					/* do this only on master core */
      					if (lcore_id == rte_get_master_lcore()) {
      						print_stats();
      						/* reset the timer */
									if(latency_diff>=latency_timelimit) force_quit=1;

									timer_tsc = 0;
      					}
      				}
      			}
      			prev_tsc = cur_tsc;
        }

        /*
         * Read packet from RX queues
         */
        for (i = 0; i < qconf->n_rx_port; i++) {
          	portid = qconf->rx_port_list[i];
          	nb_rx = rte_eth_rx_burst((uint8_t) portid, 0,
                 		pkts_burst, MAX_PKT_BURST);

          	port_statistics[portid].rx += nb_rx;

						for (j = 0; j < nb_rx; j++) {
								rtn = rte_pktmbuf_mtod_offset(pkts_burst[j], char *, sizeof(data));
							  port_statistics[portid].rx_bytes += strlen(rtn); //rte_pktmbuf_pkt_len(pkts_burst[j]);
								rte_pktmbuf_free(pkts_burst[j]);
						}


						/*
		         * Sending packets in bulk
		         */

						// unsigned i;
						// struct rte_mbuf *mrm[NB_MBUF];
						// int ret = 0;
            //
						// for (i=0; i<NB_MBUF; i++)
						// 	mrm[i] = NULL;
            //
						// char *data;
						// /* alloc NB_MBUF mbufs */
						// for (i=0; i<NB_MBUF; i++) {
						// 	mrm[i] = rte_pktmbuf_alloc(test_pktmbuf_pool);
						// 	data = rte_pktmbuf_append(mrm[i], PKT_SIZE);
						// 	memset(data, 0xff, rte_pktmbuf_pkt_len(mrm[i]));
						// 	l2fwd_mac_updating(mrm[i], portid);
            //
						// 	if (mrm[i] == NULL) {
						// 		printf("rte_pktmbuf_alloc() failed (%u)\n", i);
						// 		ret = -1;
						// 		break;
						// 	}
						// }
            //
						// sent = rte_eth_tx_burst(portid, 0, mrm, i);
            //
						// if (sent){
						// 	port_statistics[portid].tx += sent; //* rte_pktmbuf_pkt_len(rm[0]);
						// }
            //
						// for (i=0; i<NB_MBUF; i++)
						// 		rte_pktmbuf_free(mrm[i]);


						/*
						 * sending the packet individually
						 */

						rm[0] = rte_pktmbuf_alloc(test_pktmbuf_pool);

						data = rte_pktmbuf_append(rm[0], PKT_SIZE);
						memset(data, '*', rte_pktmbuf_pkt_len(rm[0]));

						rte_prefetch0(rte_pktmbuf_mtod(rm[0], void *));
						l2fwd_mac_updating(rm[0], portid);

						sent = rte_eth_tx_burst(portid, 0, rm, 1);

						if (sent){
							port_statistics[portid].tx += sent;
						}
						rte_pktmbuf_free(rm[0]);
        }
      }

}


static int
l2fwd_launch_one_lcore(__attribute__((unused)) void *dummy){
	l2fwd_main_loop();
	return 0;
}
