target_sources(app PRIVATE
	main.c
	thermal_manager.c
	led_manager.c
	gimbal_actuator.c
	gimbal_control.c
	mavlink_agent.c
)

target_include_directories(app PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/drivers/icm42688
	${CMAKE_SOURCE_DIR}/drivers/feetech
	${CMAKE_SOURCE_DIR}/lib/madgwick
	${CMAKE_SOURCE_DIR}/lib/pid
	${CMAKE_SOURCE_DIR}/lib/mavlink
)
