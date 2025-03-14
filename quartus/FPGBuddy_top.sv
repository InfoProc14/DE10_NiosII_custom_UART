module FPGBuddy_top(

	//////////// CLOCK //////////
	input 		          		ADC_CLK_10,
	input 		          		MAX10_CLK1_50,
	input 		          		MAX10_CLK2_50,

	//////////// SDRAM //////////
	output		    [12:0]		DRAM_ADDR,
	output		     [1:0]		DRAM_BA,
	output		          		DRAM_CAS_N,
	output		          		DRAM_CKE,
	output		          		DRAM_CLK,
	output		          		DRAM_CS_N,
	inout 		    [15:0]		DRAM_DQ,
	output		          		DRAM_LDQM,
	output		          		DRAM_RAS_N,
	output		          		DRAM_UDQM,
	output		          		DRAM_WE_N,

	//////////// SEG7 //////////
	output		     [7:0]		HEX0,
	output		     [7:0]		HEX1,
	output		     [7:0]		HEX2,
	output		     [7:0]		HEX3,
	output		     [7:0]		HEX4,
	output		     [7:0]		HEX5,

	//////////// KEY (PUSH BUTTONS) //////////
	input 		     [1:0]		KEY,

	//////////// LED //////////
	output		     [9:0]		LEDR,

	//////////// SW (SWITCHES) //////////
	input 		     [9:0]		SW,

	//////////// VGA //////////
	output		     [3:0]		VGA_B,
	output		     [3:0]		VGA_G,
	output		          		VGA_HS,
	output		     [3:0]		VGA_R,
	output		          		VGA_VS,

	//////////// Accelerometer //////////
	output		          		GSENSOR_CS_N,
	input 		     [2:1]		GSENSOR_INT,
	output		          		GSENSOR_SCLK,
	inout 		          		GSENSOR_SDI,
	inout 		          		GSENSOR_SDO,

	//////////// Arduino //////////
	inout 		    [15:0]		ARDUINO_IO,
	inout 		          		ARDUINO_RESET_N,

	//////////// GPIO, GPIO connect to GPIO Default //////////
	inout 		    [35:0]		GPIO
);

	FPGBuddy u0 (
		.accelerometer_spi_external_interface_I2C_SDAT      (GSENSOR_SDI),
		.accelerometer_spi_external_interface_I2C_SCLK      (GSENSOR_SCLK),
		.accelerometer_spi_external_interface_G_SENSOR_CS_N (GSENSOR_CS_N),
		.accelerometer_spi_external_interface_G_SENSOR_INT  (GSENSOR_INT[1]),
		.button_external_connection_export              	(KEY),
		.switch_external_connection_export                  (SW),
		.clk_clk                                        	(MAX10_CLK1_50),
		.hex0_external_connection_export                	(HEX0),
		.hex1_external_connection_export                	(HEX1),
		.hex2_external_connection_export                	(HEX2),
		.hex3_external_connection_export                	(HEX3),
		.hex4_external_connection_export                	(HEX4),
		.hex5_external_connection_export                	(HEX5),
		.led_external_connection_export                 	(LEDR),
		.reset_reset_n                                  	(1'b1),
		.uart_external_connection_rxd                       (ARDUINO_IO[15]), 		// FPGA uart RX maps to Arduino PIO pin 15
		.uart_external_connection_txd                       (ARDUINO_IO[14])		// FPGA uart TX maps to Arduino PIO pin 14
	);

endmodule