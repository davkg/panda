#!/usr/bin/env python3
import unittest
from panda import Panda
from panda.tests.libpanda import libpanda_py
import panda.tests.safety.common as common

class TestPsyonixSafety(common.PandaSafetyTest):
  TX_MSGS = []
  FWD_BLACKLISTED_ADDRS = {}
  FWD_BUS_LOOKUP = {}

  def setUp(self):
    self.safety = libpanda_py.libpanda
    self.safety.set_safety_hooks(Panda.SAFETY_PSYONIX, 0)
    self.safety.init_tests()

  # FIXME: temporary boilerplate below, complete these CAN message generator functions

  def _speed_msg(self, speed):
    pass

  def _user_brake_msg(self, brake):
    pass

  def _user_gas_msg(self, gas):
    pass

  def _pcm_status_msg(self, enable):
    pass

  # FIXME: temporary boilerplate below, disabling these common tests during early development

  def test_vehicle_moving(self):
    pass

  def test_cruise_engaged_prev(self):
    pass

  def test_enable_control_allowed_from_cruise(self):
    pass

  def test_disable_control_allowed_from_cruise(self):
    pass

  def test_prev_gas(self):
    pass

  def test_disengage_on_gas(self):
    pass

  def test_allow_engage_with_gas_pressed(self):
    pass

  def test_alternative_experience_no_disengage_on_gas(self):
    pass

  def test_prev_user_brake(self, _user_brake_msg=None, get_brake_pressed_prev=None):
    pass

  def test_allow_user_brake_at_zero_speed(self, _user_brake_msg=None, get_brake_pressed_prev=None):
    pass

  def test_not_allow_user_brake_when_moving(self, _user_brake_msg=None, get_brake_pressed_prev=None):
    pass

  def test_relay_malfunction(self):
    pass


if __name__ == "__main__":
  unittest.main()
