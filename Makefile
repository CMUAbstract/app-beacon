override BOARD = msp-exp430fr6989
export BOARD

TOOLCHAINS = \
	gcc \

include ext/maker/Makefile

# Paths to toolchains here if not in or different from defaults in Makefile.env
