#pragma once

int mavlink_agent_init(void);
void mavlink_rx_run(void);   /* blocking; call from mavlink_rx_thread */
void mavlink_tx_run(void);   /* blocking; call from mavlink_tx_thread */
