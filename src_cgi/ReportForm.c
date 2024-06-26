/**
 * @file ReportFormstable.c
 * @brief   登陆后台CGI程序
 * @author DXia
 * @version 1.0
 * @date
 */

#include "fcgi_config.h"
#include "fcgi_stdio.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "make_log.h" //日志头文件
#include "util_cgi.h"
#include "deal_mysql.h"
#include "cfg.h"
#include "cJSON.h"
#include "base64.h"
#include <sys/time.h>

#define REPORTFORMS_LOG_MODULE       "cgi"
#define REPORTFORMS_LOG_PROC         "ReportForms"

static char mysql_user[128] = {0};
static char mysql_pwd[128] = {0};
static char mysql_db[128] = {0};


// 读取配置信息
void read_cfg()
{

    get_cfg_value(CFG_PATH, "mysql", "user", mysql_user);
    get_cfg_value(CFG_PATH, "mysql", "password", mysql_pwd);
    get_cfg_value(CFG_PATH, "mysql", "database", mysql_db);
    LOG(REPORTFORMS_LOG_MODULE, REPORTFORMS_LOG_PROC, "mysql:[user=%s,pwd=%s,database=%s]", mysql_user, mysql_pwd, mysql_db);
}

int get_count_json_info(char *buf, char *user, char *token){
	
	int ret = 0;
	
	cJSON* root = cJSON_Parse(buf);
	if(NULL == root)
    {
		LOG(REPORTFORMS_LOG_MODULE, REPORTFORMS_LOG_PROC, "cJSON_Parse err\n");
        ret = -1;
        goto END;
	}
	
    cJSON *child1 = cJSON_GetObjectItem(root, "user");
    if(NULL == child1)
    {
        LOG(REPORTFORMS_LOG_MODULE, REPORTFORMS_LOG_PROC, "cJSON_GetObjectItem err\n");
        ret = -1;
        goto END;
    }
    strcpy(user, child1->valuestring); 

    
    cJSON *child2 = cJSON_GetObjectItem(root, "token");
    if(NULL == child2)
    {
        LOG(REPORTFORMS_LOG_MODULE, REPORTFORMS_LOG_PROC, "cJSON_GetObjectItem err\n");
        ret = -1;
        goto END;
    }
    strcpy(token, child2->valuestring); 

END:
    if(root != NULL)
    {
        cJSON_Delete(root);
        root = NULL;
    }

    return ret;
}

//返回前端情况
void return_login_status(long num, int token_flag)
{

    char *out = NULL;
    char *token;
    char num_buf[128] = {0};

    if(token_flag == 0)
    {
        token = "110"; 
    }
    else
    {
        token = "111"; 
    }

    
    sprintf(num_buf, "%ld", num);

    cJSON *root = cJSON_CreateObject();  
    cJSON_AddStringToObject(root, "num", num_buf);
    cJSON_AddStringToObject(root, "code", token);
    out = cJSON_Print(root);
    cJSON_Delete(root);

    if(out != NULL)
    {
        printf(out); 
        free(out); 
    }
}

//获取报表信息数目
void get_ReportForms_count(int ret){
	
	
	char sql_cmd[SQL_MAX_LEN] = {0};
	MYSQL *conn = NULL;
	long line = 0;
	
	
    conn = msql_conn(mysql_user, mysql_pwd, mysql_db);
    if (conn == NULL)
    {
        LOG(REPORTFORMS_LOG_MODULE, REPORTFORMS_LOG_PROC, "msql_conn err\n");
        goto END;
    }
	
	
    mysql_query(conn, "set names utf8");

    sprintf(sql_cmd, "select count(*) from ReportForms");
    char tmp[512] = {0};
    
    int ret2 = process_result_one(conn, sql_cmd, tmp); 
    if(ret2 != 0)
    {
        LOG(REPORTFORMS_LOG_MODULE, REPORTFORMS_LOG_PROC, "%s 操作失败\n", sql_cmd);
        goto END;
    }

    line = atol(tmp); 

END:
    if(conn != NULL)
    {
        mysql_close(conn);
    }

    LOG(REPORTFORMS_LOG_MODULE, REPORTFORMS_LOG_PROC, "line = %ld\n", line);

    
    return_login_status(line, ret);
}

//解析的json包
int get_ReportFormslist_json_info(char *buf,char *user,char *token,int *p_start,int *p_count){
	int ret = 0;
	
	/*json数据如下
    {
        "user": "yoyo"
        "token": xxxx
        "start": 0
        "count": 10
    }
    */
	
	//解析json包
	//解析一个json字符串为json对象
	cJSON * root = cJSON_Parse(buf);
	if(NULL == root){
		LOG(REPORTFORMS_LOG_MODULE,REPORTFORMS_LOG_PROC,"cJSON_Parse err\n");
		ret =-1;
		goto END;
	}
	
	
	// 用户
	cJSON* child1 = cJSON_GetObjectItem(root,"user");
	if(NULL == child1){
		LOG(REPORTFORMS_LOG_MODULE,REPORTFORMS_LOG_PROC,"cJSON_GetObjectItem err\n");
		ret = -1;
		goto END;
	}
	strcpy(user, child1->valuestring); 
	
	
	// token
	cJSON* child2 = cJSON_GetObjectItem(root,"token");
	if(NULL == child2){
		LOG(REPORTFORMS_LOG_MODULE,REPORTFORMS_LOG_PROC,"cJSON_GetObjectItem err\n");
		ret = -1;
		goto END;
	}
	strcpy(token, child2->valuestring); 
	
	
	// 文件起点
	cJSON* child3 = cJSON_GetObjectItem(root,"start");
	if(NULL == child3){
		LOG(REPORTFORMS_LOG_MODULE,REPORTFORMS_LOG_PROC,"cJSON_GetObjectItem err\n");
		ret = -1;
		goto END;
	}
	*p_start = child3->valueint; 

	
	
	cJSON* child4 = cJSON_GetObjectItem(root,"count");
	if(NULL == child4){
		LOG(REPORTFORMS_LOG_MODULE,REPORTFORMS_LOG_PROC,"cJSON_GetObjectItem err\n");
		ret = -1;
		goto END;
	}
	*p_count = child4->valueint; 
	
END:
	if(NULL != root){
		cJSON_Delete(root);	
		root = NULL;
	}
	
	return ret;
}

bool is_number(char* keyword){
	for(int i = 0;keyword[i] != '\0';++i){
		if(isdigit(keyword[i])){
			continue;
		}else{
			return false;
		}
	}
	return true;
}

// 获取检索数目
void get_search_count(int ret,char *keyword){
	char sql_cmd[SQL_MAX_LEN] = {0};
	MYSQL *conn = NULL;
	long line = 0;
	
	
	conn = msql_conn(mysql_user,mysql_pwd,mysql_db);
	if(conn == NULL){
		LOG(REPORTFORMS_LOG_MODULE,REPORTFORMS_LOG_PROC,"msql_conn err\n");
		goto END;
	}
	
	
    mysql_query(conn, "set names utf8");
	
	if(is_number(keyword)){
		sprintf(sql_cmd,"select count(*) from ReportForms where OrderID = '%s'",keyword);
	}else{
		sprintf(sql_cmd,"select count(*) from ReportForms where CustomerName = '%s'",keyword);
	}
	
	char tmp[512] = {0};
	
	int ret2 = process_result_one(conn,sql_cmd,tmp);	
	if(ret2 != 0){
		LOG(REPORTFORMS_LOG_MODULE,REPORTFORMS_LOG_PROC,"%s 操作失败\n",sql_cmd);
		goto END;
	}

	line = atol(tmp); 
	
END:
	if(conn != NULL){
		mysql_close(conn);
	}
	
	LOG(REPORTFORMS_LOG_MODULE,REPORTFORMS_LOG_PROC,"line = %ld\n",line);
	
	
	return_login_status(line,ret);
}

// 获取检索报表列表
int get_search_list(char *cmd,char *user,int start,int count,char *keyword){
	int ret = 0;
	char sql_cmd[SQL_MAX_LEN] = {0};
	MYSQL *conn = NULL;
	cJSON *root = NULL;
	cJSON *array = NULL;
	char *out = NULL;
	char *out2 = NULL;
	MYSQL_RES *res_set = NULL;
	
	
	conn = msql_conn(mysql_user,mysql_pwd,mysql_db);
	if(conn == NULL){
		LOG(REPORTFORMS_LOG_MODULE,REPORTFORMS_LOG_PROC,"msql_conn err\n");
		ret = -1;
		goto END;
	}
	
	
    mysql_query(conn, "set names utf8");
	
	
	if(is_number(keyword)){
		sprintf(sql_cmd,"select * from ReportForms where OrderID = '%s'",keyword);
	}else{
		sprintf(sql_cmd,"select * from ReportForms where CustomerName = '%s'",keyword);
	}
	
	LOG(REPORTFORMS_LOG_MODULE,REPORTFORMS_LOG_PROC,"%s 在操作\n",sql_cmd);
	
	if(mysql_query(conn,sql_cmd) != 0){
		LOG(REPORTFORMS_LOG_MODULE,REPORTFORMS_LOG_PROC,"%s 操作失败:%s \n",sql_cmd,mysql_error(conn));
		ret = -1;
		goto END;
	}
	
	res_set = mysql_store_result(conn); /*生成结果集*/
	
	if (res_set == NULL)
    {
        LOG(REPORTFORMS_LOG_MODULE, REPORTFORMS_LOG_PROC, "smysql_store_result error: %s!\n", mysql_error(conn));
        ret = -1;
        goto END;
    }

    ulong line = 0;
    //mysql_num_rows接受由mysql_store_result返回的结果结构集，并返回结构集中的行数
    line = mysql_num_rows(res_set);
    if (line == 0)//没有结果
    {
        LOG(REPORTFORMS_LOG_MODULE, REPORTFORMS_LOG_PROC, "mysql_num_rows(res_set) failed：%s\n", mysql_error(conn));
        ret = -1;
        goto END;
    }
	
	MYSQL_ROW row;
	
	root = cJSON_CreateObject();
	array = cJSON_CreateArray();
    // mysql_fetch_row从使用mysql_store_result得到的结果结构中提取一行，并把它放到一个行结构中。
    // 当数据用完或发生错误时返回NULL.
	while((row = mysql_fetch_row(res_set)) != NULL){
		cJSON *item = cJSON_CreateObject();
		
		//mysql_num_fields获取结果中列的个数
		//--OrderID
		if(row[0] != NULL){
			cJSON_AddNumberToObject(item,"OrderID",atoi(row[0]));
		}
		
		//--CustomerName
		if(row[1] != NULL){
			cJSON_AddStringToObject(item,"CustomerName",row[1]);
		}
		
		//SubscriptionDate
		if(row[2] != NULL){
			cJSON_AddStringToObject(item,"SubscriptionDate",row[2]);
		}
		
		//DeliveryDate
		if(row[3] != NULL){
			cJSON_AddStringToObject(item,"DeliveryDate",row[3]);
		}
		
		//IsSuccess
		if(row[4] != NULL){
			cJSON_AddStringToObject(item,"IsSuccess",row[4]);
		}
		cJSON_AddItemToArray(array,item);
	}
	
	cJSON_AddItemToObject(root,"ReportForms",array);
	
	out = cJSON_Print(root);
	
	LOG(REPORTFORMS_LOG_MODULE,REPORTFORMS_LOG_PROC,"%s\n",out);
	
END:
	if(ret == 0){
		printf("%s",out); //给客户端返回信息
	}else{
		
        /*
			获取报表列表：
            成功：报表列表json
            失败：{"code": "015"}
        */
		out2 = NULL;
		out2 = return_status("015");
	}if(out2 != NULL){
		printf(out2);	//给客户端返回信息
		free(out2);
	}
	
    if(res_set != NULL)
    {
        //完成所有对数据的操作后，调用mysql_free_result来善后处理
        mysql_free_result(res_set);
    }

    if(conn != NULL)
    {
        mysql_close(conn);
    }

    if(root != NULL)
    {
        cJSON_Delete(root);
    }

    if(out != NULL)
    {
        free(out);
    }

    return ret;
}

//解析上传的原料信息json包
int get_ReportForms_info(char *buf,int *OrderID,char* CustomerName,char *DeliveryDate,char *IsSuccess){
	int ret = 0;

    cJSON *root = cJSON_Parse(buf);
    if (NULL == root) {
        LOG(REPORTFORMS_LOG_MODULE, REPORTFORMS_LOG_PROC, "cJSON_Parse err\n");
        ret = -1;
        goto END;
    }
	
	cJSON *child = cJSON_GetObjectItem(root, "OrderID");
    if (NULL == child) {
        LOG(REPORTFORMS_LOG_MODULE, REPORTFORMS_LOG_PROC, "cJSON_GetObjectItem err\n");
        ret = -1;
        goto END;
    }
    *OrderID = child->valueint;

	child = cJSON_GetObjectItem(root, "CustomerName");
    if (NULL == child) {
        LOG(REPORTFORMS_LOG_MODULE, REPORTFORMS_LOG_PROC, "cJSON_GetObjectItem err\n");
        ret = -1;
        goto END;
    }
    strcpy(CustomerName, child->valuestring);
	
	child = cJSON_GetObjectItem(root, "DeliveryDate");
    if (NULL == child) {
        LOG(REPORTFORMS_LOG_MODULE, REPORTFORMS_LOG_PROC, "cJSON_GetObjectItem err\n");
        ret = -1;
        goto END;
    }
    strcpy(DeliveryDate, child->valuestring);
    
	child = cJSON_GetObjectItem(root, "IsSuccess");
    if (NULL == child) {
        LOG(REPORTFORMS_LOG_MODULE, REPORTFORMS_LOG_PROC, "cJSON_GetObjectItem err\n");
        ret = -1;
        goto END;
    }
    strcpy(IsSuccess, child->valuestring);
END:
    if (root != NULL) {
        cJSON_Delete(root);
        root = NULL;
    }
    return ret;
}	

//更新报表信息
int ReportForms_update(char *buf){
	int ret = 0;
	MYSQL *conn = NULL;
	
	int OrderID;
	char CustomerName[128];
	char SubscriptionDate[128];
	char DeliveryDate[128];
	char IsSuccess[10];
	ret = get_ReportForms_info(buf,&OrderID,CustomerName,DeliveryDate,IsSuccess);
	if(ret != 0){
		goto END;
	}
	LOG(REPORTFORMS_LOG_MODULE,REPORTFORMS_LOG_PROC,"OrderID = %d, DeliveryDate = %s,IsSuccess = %s",OrderID,DeliveryDate,IsSuccess);
	conn = msql_conn(mysql_user,mysql_pwd,mysql_db);
	if(conn == NULL){
		LOG(REPORTFORMS_LOG_MODULE,REPORTFORMS_LOG_PROC,"msql_conn err\n");
		ret = -1;
		goto END;
	}
	mysql_query(conn,"set names utf8");
	
	mysql_autocommit(conn, 0);
	// 执行ReportForms表的更新
	char sql_cmd[SQL_MAX_LEN] = {0};
    sprintf(sql_cmd,"UPDATE ReportForms SET DeliveryDate = '%s',IsSuccess = '%s' WHERE OrderID = %d;",DeliveryDate,IsSuccess,OrderID);
    if (mysql_query(conn, sql_cmd)) {
        fprintf(stderr, "更新ReportForms表失败: %s\n", mysql_error(conn));
        mysql_rollback(conn);  // 回滚事务
        mysql_close(conn);     // 关闭数据库连接
        return -1;
    }


	// 查询Customer表获取产品名称和数量
    sprintf(sql_cmd,"SELECT ProductName, count FROM %s WHERE OrderID = %d;",CustomerName,OrderID);
    if (mysql_query(conn, sql_cmd)) {
        LOG(REPORTFORMS_LOG_MODULE, REPORTFORMS_LOG_PROC, "查询Customer表失败: %s\n", mysql_error(conn));
        mysql_rollback(conn);  // 回滚事务
        mysql_close(conn);     // 关闭数据库连接
        return -1;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) {
        LOG(REPORTFORMS_LOG_MODULE, REPORTFORMS_LOG_PROC, "无法获取查询结果: %s\n", mysql_error(conn));
        mysql_rollback(conn);  // 回滚事务
        mysql_close(conn);     // 关闭数据库连接
        return -1;
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        const char *productName = row[0];
        int quantity = atoi(row[1]);

        // 更新Product表的数量
        sprintf(sql_cmd,"UPDATE productinfo SET product_amount = product_amount - %d WHERE product_name = '%s';",quantity, productName);
        if (mysql_query(conn, sql_cmd)) {
            LOG(REPORTFORMS_LOG_MODULE, REPORTFORMS_LOG_PROC, "更新Product表失败: %s\n", mysql_error(conn));
            mysql_rollback(conn);  // 回滚事务
            mysql_close(conn);     // 关闭数据库连接
            mysql_free_result(result);
            return -1;
        }

        // 删除客户表中的数据
        sprintf(sql_cmd,"DELETE FROM %s WHERE product_name = '%s';",CustomerName,productName);
        if (mysql_query(conn, sql_cmd)) {
            LOG(REPORTFORMS_LOG_MODULE, REPORTFORMS_LOG_PROC, "删除%s表失败: %s\n",CustomerName, mysql_error(conn));
            mysql_rollback(conn);  // 回滚事务
            mysql_close(conn);     // 关闭数据库连接
            mysql_free_result(result);
            return -1;
        }
    }

    // 释放查询结果
    mysql_free_result(result);

    // 提交事务
    if (mysql_commit(conn)) {
        LOG(REPORTFORMS_LOG_MODULE, REPORTFORMS_LOG_PROC, "提交事务失败: %s\n", mysql_error(conn));
        mysql_rollback(conn);  // 回滚事务
        mysql_close(conn);     // 关闭数据库连接
        return -1;
    }


END:
    if(conn != NULL)
    {
        mysql_close(conn);
    }

    return ret;
}

//获取报表列表
int get_ReportForms_list(char *user,int start,int count){
	int ret = 0;
	char sql_cmd[SQL_MAX_LEN] = {0};
	MYSQL *conn = NULL;
    cJSON *root = NULL;
    cJSON *array =NULL;
    char *out = NULL;
    char *out2 = NULL;
    MYSQL_RES *res_set = NULL;
	
    
    conn = msql_conn(mysql_user, mysql_pwd, mysql_db);
    if (conn == NULL)
    {
        LOG(REPORTFORMS_LOG_MODULE, REPORTFORMS_LOG_PROC, "msql_conn err\n");
        ret = -1;
        goto END;
    }

    
    mysql_query(conn, "set names utf8");
	
	sprintf(sql_cmd,"SELECT * FROM ReportForms limit %d,%d \n",start,count);
	
	
	LOG(REPORTFORMS_LOG_MODULE,REPORTFORMS_LOG_PROC,"%s 在操作\n",sql_cmd);
	
	if(mysql_query(conn,sql_cmd) != 0){
		LOG(REPORTFORMS_LOG_MODULE,REPORTFORMS_LOG_PROC,"%s 操作失败:%s \n",sql_cmd,mysql_error(conn));
		ret = -1;
		goto END;
	}
	
	res_set = mysql_store_result(conn);/*生成结果集*/
    if (res_set == NULL)
    {
        LOG(REPORTFORMS_LOG_MODULE, REPORTFORMS_LOG_PROC, "smysql_store_result error: %s!\n", mysql_error(conn));
        ret = -1;
        goto END;
    }

    ulong line = 0;
    //mysql_num_rows接受由mysql_store_result返回的结果结构集，并返回结构集中的行数
    line = mysql_num_rows(res_set);
    if (line == 0)//没有结果
    {
        LOG(REPORTFORMS_LOG_MODULE, REPORTFORMS_LOG_PROC, "mysql_num_rows(res_set) failed：%s\n", mysql_error(conn));
        ret = -1;
        goto END;
    }
	
	MYSQL_ROW row;
	
	root = cJSON_CreateObject();
	array = cJSON_CreateArray();
    // mysql_fetch_row从使用mysql_store_result得到的结果结构中提取一行，并把它放到一个行结构中。
    // 当数据用完或发生错误时返回NULL.
	while((row = mysql_fetch_row(res_set)) != NULL){
		cJSON *item = cJSON_CreateObject();
		
		//mysql_num_fields获取结果中列的个数
		//--OrderID
		if(row[0] != NULL){
			cJSON_AddNumberToObject(item,"OrderID",atoi(row[0]));
		}
		
		//--CustomerName
		if(row[1] != NULL){
			cJSON_AddStringToObject(item,"CustomerName",row[1]);
		}
		
		//SubscriptionDate
		if(row[2] != NULL){
			cJSON_AddStringToObject(item,"SubscriptionDate",row[2]);
		}
		
		//DeliveryDate
		if(row[3] != NULL){
			cJSON_AddStringToObject(item,"DeliveryDate",row[3]);
		}
		
		//IsSuccess
		if(row[4] != NULL){
			cJSON_AddStringToObject(item,"IsSuccess",row[4]);
		}
		
		cJSON_AddItemToArray(array,item);
	}
	
	cJSON_AddItemToObject(root,"ReportForms",array);
	
	out = cJSON_Print(root);
	
	LOG(REPORTFORMS_LOG_MODULE,REPORTFORMS_LOG_PROC,"%s\n",out);
	
END:
	if(ret == 0){
		printf("%s",out); //给客户端返回信息
	}else{
		
        /*
			获取报表列表：
            成功：报表列表json
            失败：{"code": "015"}
        */
		out2 = NULL;
		out2 = return_status("015");
	}if(out2 != NULL){
		printf(out2);	//给客户端返回信息
		free(out2);
	}
	
    if(res_set != NULL)
    {
        //完成所有对数据的操作后，调用mysql_free_result来善后处理
        mysql_free_result(res_set);
    }

    if(conn != NULL)
    {
        mysql_close(conn);
    }

    if(root != NULL)
    {
        cJSON_Delete(root);
    }

    if(out != NULL)
    {
        free(out);
    }

    return ret;
}


int main(){
	// count 获取用户文件个数
    // display 获取用户文件信息，展示到前端
    char cmd[20];
    char user[USER_NAME_LEN];
    char token[TOKEN_LEN];
	
	char keyword[100];
	char keywordori[100];

     // 读取数据库配置信息
    read_cfg();
	
    // 阻塞等待用户连接
    while (FCGI_Accept() >= 0)
    {

        // 获取URL地址 "?" 后面的内容
        char *query = getenv("QUERY_STRING");

        // 解析命令
        query_parse_key_value(query, "cmd", cmd, NULL);
        LOG(REPORTFORMS_LOG_MODULE, REPORTFORMS_LOG_PROC, "cmd = %s\n", cmd);

        char *contentLength = getenv("CONTENT_LENGTH");
        int len;

        printf("Content-type: text/html\r\n\r\n");

        if( contentLength == NULL )
        {
            len = 0;
        }
        else
        {
            len = atoi(contentLength); // 字符串转整型
        }

        if (len <= 0)
        {
            printf("No data from standard input.<p>\n");
            LOG(REPORTFORMS_LOG_MODULE, REPORTFORMS_LOG_PROC, "len = 0, No data from standard input\n");
        }
        else
        {
            char buf[4*1024] = {0};
            int ret = 0;
            ret = fread(buf, 1, len, stdin); // 从标准输入(web服务器)读取内容
            if(ret == 0)
            {
                LOG(REPORTFORMS_LOG_MODULE, REPORTFORMS_LOG_PROC, "fread(buf, 1, len, stdin) err\n");
                continue;
            }

            LOG(REPORTFORMS_LOG_MODULE, REPORTFORMS_LOG_PROC, "buf = %s\n", buf);

            if (strcmp(cmd, "ReportFormCount") == 0) //count 获取报表信息数目
            {
                get_count_json_info(buf, user, token); // 通过json包获取用户名, token

                // 验证登陆token，成功返回0，失败-1
				ret = verify_token(user, token); // util_cgi.h

                get_ReportForms_count(ret); // 获取报表信息个数

            }else if(strcmp(cmd,"ReportFormNormal") == 0){	//初始化时获取报表列表的
				int start;// 文件起点
				int count;// 文件个数
				get_ReportFormslist_json_info(buf,user,token,&start,&count); // 通过json包获取信息
				LOG(REPORTFORMS_LOG_MODULE,REPORTFORMS_LOG_PROC, "user = %s, token = %s, start = %d, count = %d\n", user, token, start, count);
			
				// 验证登录token，成功返回0，失败-1
				ret = verify_token(user,token);
				if(ret == 0){
					get_ReportForms_list(user,start,count);	//获取报表列表
				}
				else{
					char *out = return_status("111"); 	// token 验证失败错误码
					if(out != NULL){
						printf(out); 	//给前端反馈错误码
						free(out);
					}
				}
			}else if(strcmp(cmd,"ReportFormResult") == 0){	//搜索时获取报表列表的
				int start; //文件起点
				int count; // 文件个数
				get_ReportFormslist_json_info(buf,user,token,&start,&count); // 通过json包获取信息
				LOG(REPORTFORMS_LOG_MODULE,REPORTFORMS_LOG_PROC,"user = %s, token = %s, start = %d, count = %d\n", user, token, start, count);
				
				// 验证登录token，成功返回0，失败-1
				ret = verify_token(user,token);
				if(ret == 0){
					get_search_list(cmd,user,start,count,keyword);	//获取报表列表
				}
				else{
					char *out = return_status("111"); 	// token 验证失败错误码
					if(out != NULL){
						printf(out); 	//给前端反馈错误码
						free(out);
					}
				}
			}else if(strcmp(cmd,"ReportFormUpdate") == 0){
				ret = ReportForms_update(buf);
				if (ret == 0) //上传成功
				{
					//返回前端更新情况， 020代表成功
					char *out = return_status("020");
					if(out != NULL){
						printf(out); 	//给前端反馈错误码
						free(out);
					}
				}
				else if(ret == -1)
				{
					//返回前端更新情况， 021代表失败
					char *out = return_status("021");
					if(out != NULL){
						printf(out); 	//给前端反馈错误码
						free(out);
					}
				}
			}else{
				query_parse_key_value(cmd,"ReportFormSearch",keywordori,NULL);
				//将base64转码的关键字转为原来的文字
				base64_decode(keywordori,keyword);
				LOG(REPORTFORMS_LOG_MODULE,REPORTFORMS_LOG_PROC,"keywordori = %s\n",keywordori);
				LOG(REPORTFORMS_LOG_MODULE,REPORTFORMS_LOG_PROC,"keyword = %s\n",keyword);
				
				get_count_json_info(buf, user, token); // 通过json包获取用户名, token
				
				// 验证登陆token，成功返回0，失败-1
				ret = verify_token(user, token); // util_cgi.h
				
				get_search_count(ret,keyword);
			}
        }
    }
    return 0;
}
