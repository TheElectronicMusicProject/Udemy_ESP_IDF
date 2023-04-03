# See the build system documentation in IDF programming guide
# for more information about component CMakeLists.txt files.

idf_component_register(
	SRCS
		"main.c"
		"bme680_sensor.c"
	INCLUDE_DIRS
		"."
)