# 使用的编译器
CC=gcc
# 预处理参数
CPPLFAGS=-I./include \
		 -I/usr/local/include/hiredis/ \
		 -I/usr/include/mysql/
# 选项
CFLAGS=-Wall
# 需要链接的动态库
LIBS=-lhiredis \
	-lfcgi \
	-lm \
	-lmysqlclient
# 目录路径
COMMON_PATH=common
CGI_BIN_PATH=bin_cgi
CGI_SRC_PATH=src_cgi

# 项目用
login=$(CGI_BIN_PATH)/login
register=$(CGI_BIN_PATH)/register
md5=$(CGI_BIN_PATH)/md5
wares=$(CGI_BIN_PATH)/wares
product=$(CGI_BIN_PATH)/product
showpro=$(CGI_BIN_PATH)/showpro
UserOrder=$(CGI_BIN_PATH)/UserOrder
ReportForm=$(CGI_BIN_PATH)/ReportForm

# 最终目标
target=$(login) \
	   $(register) \
	   $(md5) \
	   $(wares) \
	   $(product) \
	   $(showpro) \
	   $(UserOrder) \
	   $(ReportForm)

ALL:$(target)

#######################################################################

# =====================================================================
#							项目程序规则
# 登录
$(login): $(CGI_SRC_PATH)/login_cgi.o \
			$(COMMON_PATH)/make_log.o  \
			$(COMMON_PATH)/cJSON.o \
			$(COMMON_PATH)/deal_mysql.o \
			$(COMMON_PATH)/redis_op.o  \
			$(COMMON_PATH)/cfg.o \
			$(COMMON_PATH)/util_cgi.o \
			$(COMMON_PATH)/des.o \
			$(COMMON_PATH)/base64.o \
			$(COMMON_PATH)/md5.o
	$(CC) $^ -o $@ $(LIBS)
# 注册
$(register): $(CGI_SRC_PATH)/reg_cgi.o \
				$(COMMON_PATH)/make_log.o  \
				$(COMMON_PATH)/util_cgi.o \
				$(COMMON_PATH)/cJSON.o \
				$(COMMON_PATH)/deal_mysql.o \
				$(COMMON_PATH)/redis_op.o  \
				$(COMMON_PATH)/cfg.o
	$(CC) $^ -o $@ $(LIBS)
# 秒传
$(md5): $(CGI_SRC_PATH)/md5_cgi.o \
		$(COMMON_PATH)/make_log.o  \
		$(COMMON_PATH)/util_cgi.o \
		$(COMMON_PATH)/cJSON.o \
		$(COMMON_PATH)/deal_mysql.o \
		$(COMMON_PATH)/redis_op.o  \
		$(COMMON_PATH)/cfg.o
	$(CC) $^ -o $@ $(LIBS)
# 原料信息表
$(wares):	$(CGI_SRC_PATH)/warestable.o \
			$(COMMON_PATH)/make_log.o  \
			$(COMMON_PATH)/util_cgi.o \
			$(COMMON_PATH)/cJSON.o \
			$(COMMON_PATH)/deal_mysql.o \
			$(COMMON_PATH)/base64.o \
			$(COMMON_PATH)/redis_op.o  \
			$(COMMON_PATH)/cfg.o
	$(CC) $^ -o $@ $(LIBS)
# 产品信息表
$(product):	$(CGI_SRC_PATH)/producttable.o \
			$(COMMON_PATH)/make_log.o  \
			$(COMMON_PATH)/util_cgi.o \
			$(COMMON_PATH)/cJSON.o \
			$(COMMON_PATH)/deal_mysql.o \
			$(COMMON_PATH)/base64.o \
			$(COMMON_PATH)/redis_op.o  \
			$(COMMON_PATH)/cfg.o
	$(CC) $^ -o $@ $(LIBS)
# 商品预览页面
$(showpro):	$(CGI_SRC_PATH)/showpro.o \
			$(COMMON_PATH)/make_log.o  \
			$(COMMON_PATH)/util_cgi.o \
			$(COMMON_PATH)/cJSON.o \
			$(COMMON_PATH)/deal_mysql.o \
			$(COMMON_PATH)/base64.o \
			$(COMMON_PATH)/redis_op.o  \
			$(COMMON_PATH)/cfg.o
	$(CC) $^ -o $@ $(LIBS)
$(UserOrder):	$(CGI_SRC_PATH)/UserOrder.o \
			$(COMMON_PATH)/make_log.o  \
			$(COMMON_PATH)/util_cgi.o \
			$(COMMON_PATH)/cJSON.o \
			$(COMMON_PATH)/deal_mysql.o \
			$(COMMON_PATH)/base64.o \
			$(COMMON_PATH)/redis_op.o  \
			$(COMMON_PATH)/cfg.o
	$(CC) $^ -o $@ $(LIBS)
$(ReportForm):	$(CGI_SRC_PATH)/ReportForm.o \
			$(COMMON_PATH)/make_log.o  \
			$(COMMON_PATH)/util_cgi.o \
			$(COMMON_PATH)/cJSON.o \
			$(COMMON_PATH)/deal_mysql.o \
			$(COMMON_PATH)/base64.o \
			$(COMMON_PATH)/redis_op.o  \
			$(COMMON_PATH)/cfg.o
	$(CC) $^ -o $@ $(LIBS)

# =====================================================================

#######################################################################
#                         所有程序都需要的规则
# 生成所有的.o 文件
%.o:%.c
	$(CC) -c $< -o $@ $(CPPLFAGS) $(CFLAGS)

# 项目清除
clean:
	-rm -rf *.o $(target) $(TEST_PATH)/*.o $(CGI_SRC_PATH)/*.o $(COMMON_PATH)/*.o

# 声明伪文件
.PHONY:clean ALL
#######################################################################
