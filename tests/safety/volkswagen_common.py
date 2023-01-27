class VolkswagenMxbCommon:
  MSG_ESP_19 = 0xB2       # RX from ABS, for wheel speeds
  MSG_LH_EPS_03 = 0x9F    # RX from EPS, for driver steering torque
  MSG_ESP_05 = 0x106      # RX from ABS, for brake light state
  MSG_TSK_06 = 0x120      # RX from ECU, for ACC status from drivetrain coordinator
  MSG_MOTOR_20 = 0x121    # RX from ECU, for driver throttle input
  MSG_ACC_06 = 0x122      # TX by OP, ACC control instructions to the drivetrain coordinator
  MSG_HCA_01 = 0x126      # TX by OP, Heading Control Assist steering torque
  MSG_GRA_ACC_01 = 0x12B  # TX by OP, ACC control buttons for cancel/resume
  MSG_ACC_07 = 0x12E      # TX by OP, ACC control instructions to the drivetrain coordinator
  MSG_ACC_02 = 0x30C      # TX by OP, ACC HUD data to the instrument cluster
  MSG_LDW_02 = 0x397      # TX by OP, Lane line recognition and text alerts

  STANDSTILL_THRESHOLD = 0
  RELAY_MALFUNCTION_ADDR = MSG_HCA_01
  RELAY_MALFUNCTION_BUS = 0

  MAX_RATE_UP = 4
  MAX_RATE_DOWN = 10
  MAX_TORQUE = 300
  MAX_RT_DELTA = 75
  RT_INTERVAL = 250000

  DRIVER_TORQUE_ALLOWANCE = 80
  DRIVER_TORQUE_FACTOR = 3

  MAX_ACCEL = 2.0
  MIN_ACCEL = -3.5
