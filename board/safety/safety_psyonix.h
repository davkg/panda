static int psyonix_fwd_hook(int bus_num, int addr) {
  int bus_fwd = -1;
  UNUSED(addr);

  if (bus_num == 0) {
    bus_fwd = 2;
  }
  if (bus_num == 2) {
    bus_fwd = 0;
  }

  return bus_fwd;
}
const safety_hooks psyonix_hooks = {
  .init = nooutput_init,
  .rx = default_rx_hook,
  .tx = nooutput_tx_hook,
  .tx_lin = nooutput_tx_lin_hook,
  .fwd = psyonix_fwd_hook,
};
