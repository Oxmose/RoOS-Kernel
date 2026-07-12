DEP_INCLUDES =
DEP_LIBS     =
DEP_MODULES  =

DEP_LIBS += --whole-archive -larch
DEP_LIBS += -lcore
DEP_LIBS += -llibc
DEP_LIBS += -lapi
DEP_LIBS += -ltime
DEP_LIBS += -lio
DEP_LIBS += --whole-archive -lrawdtb

DEP_MODULES += -L../arch/bin
DEP_MODULES += -L../ARTIFACTS
DEP_MODULES += -L../core/bin
DEP_MODULES += -L../libs/bin
DEP_MODULES += -L../time/bin
DEP_MODULES += -L../io/bin