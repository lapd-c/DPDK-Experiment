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

	printf("\nDPDK Pingpong Server ====================================");

	for (portid = 0; portid < RTE_MAX_ETHPORTS; portid++) {
		/* skip disabled ports */
		if ((l2fwd_enabled_port_mask & (1 << portid)) == 0)
			continue;
		printf("\nStatistics for port %u ------------------------------"
			   "\nPackets sent: %24"PRIu64
			   "\nPackets received: %20"PRIu64
			   "\nPackets dropped: %21"PRIu64,
			   portid,
			   port_statistics[portid].tx,
			   port_statistics[portid].rx,
			   port_statistics[portid].dropped);

		total_packets_dropped += port_statistics[portid].dropped;
		total_packets_tx += port_statistics[portid].tx;
		total_packets_rx += port_statistics[portid].rx;
	}

	latency_diff = difftime( time(0), start);

	printf("\nAggregate statistics ==============================="
		   "\nTotal Packets sent: %18"PRIu64
		   "\nTotal Packets received: %14"PRIu64
		   "\nTotal packets dropped: %15"PRIu64
			 "\nAggregated time (sec): %f",
		   total_packets_tx,
		   total_packets_rx,
		   total_packets_dropped,
       latency_diff);
	printf("\n====================================================\n");
}



static void
l2fwd_mac_updating(struct rte_mbuf *m, unsigned dest_portid){
	struct ether_hdr *eth;
	void *tmp;

	eth = rte_pktmbuf_mtod(m, struct ether_hdr *);

	/* 02:00:00:00:00:xx */
	tmp = &eth->d_addr.addr_bytes[0];

	// 00:1b:21:a6:d4:d4
	*((uint64_t *)tmp) = 0xd4d4a6211b00 + ((uint64_t)dest_portid << 40);

	/* src addr */
	ether_addr_copy(&l2fwd_ports_eth_addr[dest_portid], &eth->s_addr);

}


/* main processing loop */
static void l2fwd_main_loop(void){
    struct rte_mbuf *pkts_burst[MAX_PKT_BURST];
  	struct rte_mbuf *m;
  	int sent;
  	unsigned lcore_id;
  	uint64_t prev_tsc, diff_tsc, cur_tsc, timer_tsc;
  	unsigned i, j, portid, nb_rx, nb_tx;
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

									if(latency_diff>=latency_timelimit) force_quit=1;

      						/* reset the timer */
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

          	nb_rx = rte_eth_rx_burst((uint8_t) portid, 0, pkts_burst, MAX_PKT_BURST);
          	port_statistics[portid].rx += nb_rx;

						for (j = 0; j < nb_rx; j++) {
								m = pkts_burst[j];
								// port_statistics[portid].rx += rte_pktmbuf_pkt_len(m);
								rte_prefetch0(rte_pktmbuf_mtod(m, void *));
								l2fwd_mac_updating(m, portid);
						    buffer = tx_buffer[portid];
								sent = rte_eth_tx_buffer(portid, 0, buffer, m);

								if(sent)
								port_statistics[portid].tx += sent * rte_pktmbuf_pkt_len(m);
						}
        	}
      }

}


static int
l2fwd_launch_one_lcore(__attribute__((unused)) void *dummy){
	l2fwd_main_loop();
	return 0;
}
