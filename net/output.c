#include <kernel/e1000.h>
#include <net/ns.h>

extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	struct tx_desc td = {0};
	int ret;

	while (1) {
		// 	- read a packet from the network server
		ret = sys_ipc_recv(&nsipcbuf);
		if (ret)
			continue;

		//	- send the packet to the device driver
		if (thisenv->env_ipc_value == NSREQ_OUTPUT) {
			ret = sys_tx_pkt((uint8_t *)nsipcbuf.pkt.jp_data, nsipcbuf.pkt.jp_len);
			if (ret)
				panic("Output ENV fail to transmit packets");
		}
	}
}