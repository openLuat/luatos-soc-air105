把需要适配luatos的接口源码放在本目录，比如luat_air105_xxx.c


# C_SRC_LIST_EX =$(wildcard $(LUATOS_PATH)/lua/src/*.c)

C_SRC_LIST_EX = $(LUATOS_PATH)/luat/packages/lfs/lfs.c $(LUATOS_PATH)/lua/src/bget.c $(LUATOS_PATH)/lua/src/printf.c

# C_SRC_LIST_EX +=$(wildcard $(LUATOS_PATH)/luat/module/*.c)
# C_SRC_LIST_EX +=$(wildcard $(LUATOS_PATH)/luat/packages/lfs/*.c)
# C_SRC_LIST_EX +=$(wildcard $(LUATOS_PATH)/luat/packages/vfs/*.c)
# C_SRC_LIST_EX +=$(wildcard $(LUATOS_PATH)/luat/packages/weak/*.c)


