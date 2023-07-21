const safety_hooks psyonix_hooks = {
  .init = nooutput_init,
  .rx = default_rx_hook,
  .tx = nooutput_tx_hook,
  .tx_lin = nooutput_tx_lin_hook,
  .fwd = alloutput_fwd_hook,
};
