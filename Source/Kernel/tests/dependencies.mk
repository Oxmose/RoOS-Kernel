DEP_INCLUDES  = -I ../io/includes
DEP_INCLUDES += -I ../core/includes
DEP_INCLUDES += -I ../libs/libc/includes
DEP_INCLUDES += -I ../libs/libapi/includes
DEP_INCLUDES += -I ../arch/cpu/includes

ifeq ($(target), x86_64)
	DEP_INCLUDES += -I ../arch/cpu/x86_64/includes
endif