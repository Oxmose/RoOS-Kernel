DEP_INCLUDES =
DEP_LIBS     =
DEP_MODULES  =

DEP_INCLUDES += -I../../../libs/libuser/includes
DEP_INCLUDES += -I../../../libs/libc/includes

DEP_LIBS += -luser
DEP_LIBS += -llibc

DEP_MODULES += -L../../../libs/bin