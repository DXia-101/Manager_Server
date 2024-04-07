/**
 * @file UserOrder.c
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

#define USERORDER_LOG_MODULE       "cgi"
#define USERORDER_LOG_PROC         "userorder"

//mysql 数据库配置信息 用户名， 密码， 数据库名称
static char mysql_user[128] = {0};
static char mysql_pwd[128] = {0};
static char mysql_db[128] = {0};

// 读取配置信息
void read_cfg()
{
	//读取mysql数据库配置信息
    get_cfg_value(CFG_PATH, "mysql", "user", mysql_user);
    get_cfg_value(CFG_PATH, "mysql", "password", mysql_pwd);
    get_cfg_value(CFG_PATH, "mysql", "database", mysql_db);
    LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC, "mysql:[user=%s,pwd=%s,database=%s]", mysql_user, mysql_pwd, mysql_db);
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

int GetUserNameFromTable(char ***array, int arraySize) {
    int ret = 0;
    MYSQL* conn = NULL;
    MYSQL_RES *res_set = NULL;
    conn = msql_conn(mysql_user, mysql_pwd, mysql_db);
    if (conn == NULL) {
        LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC, "msql_conn err\n");
        ret = -1;
        goto END;
    }
    mysql_query(conn, "set names utf8");

    char sql_cmd[SQL_MAX_LEN] = {0};
    sprintf(sql_cmd, "SELECT name FROM user");

    if (mysql_query(conn, sql_cmd) != 0) {
        LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC, "%s 操作失败:%s \n", sql_cmd, mysql_error(conn));
        ret = -1;
        goto END;
    }
    res_set = mysql_store_result(conn); /*生成结果集*/
    if (res_set == NULL) {
        LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC, "获取结果集 error: %s!\n", mysql_error(conn));
        ret = -1;
        goto END;
    }
    long line = 0;
    //mysql_num_rows接受由mysql_store_result返回的结果结构集，并返回结构集中的行数
    line = mysql_num_rows(res_set);
    if (line == 0) //没有结果
    {
        LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC, "mysql_num_rows(res_set) failed：%s\n", mysql_error(conn));
        ret = -1;
        goto END;
    }

    MYSQL_ROW row;
    int index = 0;
    *array = (char**)malloc(arraySize * sizeof(char *));
    if (*array == NULL) {
        LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC, "内存分配失败\n");
        ret = -1;
        goto END;
    }
    while ((row = mysql_fetch_row(res_set))) {
        const char *string = row[0];
        if (index < arraySize) {
            (*array)[index] = strdup(string);
            if ((*array)[index] == NULL) {
                LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC, "内存分配失败\n");
                ret = -1;
                goto END;
            }
            index++;
        } else {
            break;
        }
    }
END:
    if (res_set != NULL) {
        //完成所有对数据的操作后，调用mysql_free_result来善后处理
        mysql_free_result(res_set);
    }

    if (conn != NULL) {
        mysql_close(conn);
    }

    return ret;
}
int Get_User_Count(){
    char sql_cmd[SQL_MAX_LEN] = {0};
	MYSQL *conn = NULL;
	
	
    conn = msql_conn(mysql_user, mysql_pwd, mysql_db);
    if (conn == NULL)
    {
        LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC, "msql_conn err\n");
        goto END;
    }
	
	
    mysql_query(conn, "set names utf8");

    sprintf(sql_cmd, "select count(*) from user");
    char tmp[512] = {0};
    
    int ret = process_result_one(conn, sql_cmd, tmp); 
    if(ret != 0)
    {
        LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC, "%s 操作失败\n", sql_cmd);
        goto END;
    }

    return atol(tmp); 
END:
    if(conn != NULL)
    {
        mysql_close(conn);
    }

    return -1;
}

cJSON* convertArrayToJson(const char** array,int arraySize){
    cJSON* jsonArray = cJSON_CreateArray();

    for(int i = 0;i < arraySize;++i){
        cJSON* jsonString = cJSON_CreateString(array[i]);
        cJSON_AddItemToArray(jsonArray,jsonString);
    }
    return jsonArray;
}

//解析的json包, 登陆token
int get_count_json_info(char *buf, char *user, char *token){
	int ret = 0;

	cJSON* root = cJSON_Parse(buf);
	if(NULL == root)
    {
		LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"cJSON_Parse err\n");
        ret = -1;
        goto END;
	}
	
    cJSON *child1 = cJSON_GetObjectItem(root, "user");
    if(NULL == child1)
    {
        LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"cJSON_GetObjectItem err\n");
        ret = -1;
        goto END;
    }
    strcpy(user, child1->valuestring); 

    cJSON *child2 = cJSON_GetObjectItem(root, "token");
    if(NULL == child2)
    {
        LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"cJSON_GetObjectItem err\n");
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

//获取商品信息数目
void get_userOrder_count(int ret,char *curUserName){
	char sql_cmd[SQL_MAX_LEN] = {0};
	MYSQL *conn = NULL;
	long line = 0;
    conn = msql_conn(mysql_user, mysql_pwd, mysql_db);
    if (conn == NULL)
    {
        LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"msql_conn err\n");
        goto END;
    }
	
    mysql_query(conn, "set names utf8");

    sprintf(sql_cmd, "select count(*) from %s",curUserName);
    char tmp[512] = {0};
    
    int ret2 = process_result_one(conn, sql_cmd, tmp); 
    if(ret2 != 0)
    {
        LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"%s 操作失败\n", sql_cmd);
        goto END;
    }
    line = atol(tmp); 
END:
    if(conn != NULL)
    {
        mysql_close(conn);
    }
    LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"line = %ld\n", line);
    
    return_login_status(line, ret);
}

int get_userOrderlist_json_info(char *buf,char *user,char *token,int *p_start,int *p_count){
	int ret = 0;
	
	/*json数据如下
    {
        "user": "yoyo"
        "token": xxxx
        "start": 0
        "count": 10
    }
    */
	cJSON * root = cJSON_Parse(buf);
	if(NULL == root){
		LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"cJSON_Parse err\n");
		ret =-1;
		goto END;
	}
	
	
	// 用户
	cJSON* child1 = cJSON_GetObjectItem(root,"user");
	if(NULL == child1){
		LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"cJSON_GetObjectItem err\n");
		ret = -1;
		goto END;
	}
	strcpy(user, child1->valuestring); 
	
	
	// token
	cJSON* child2 = cJSON_GetObjectItem(root,"token");
	if(NULL == child2){
		LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"cJSON_GetObjectItem err\n");
		ret = -1;
		goto END;
	}
	strcpy(token, child2->valuestring); 
	
	
	// 文件起点
	cJSON* child3 = cJSON_GetObjectItem(root,"start");
	if(NULL == child3){
		LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"cJSON_GetObjectItem err\n");
		ret = -1;
		goto END;
	}
	*p_start = child3->valueint; 

	
	
	cJSON* child4 = cJSON_GetObjectItem(root,"count");
	if(NULL == child4){
		LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"cJSON_GetObjectItem err\n");
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

int get_userOrder_list(char *user,int start,int count,char *curUserName){
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
        LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"msql_conn err\n");
        ret = -1;
        goto END;
    }

    
    mysql_query(conn, "set names utf8");

	sprintf(sql_cmd,"SELECT * FROM %s limit %d,%d \n",curUserName,start,count);

	
	LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"%s 在操作\n",sql_cmd);
	
	if(mysql_query(conn,sql_cmd) != 0){
		LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"%s 操作失败:%s \n",sql_cmd,mysql_error(conn));
		ret = -1;
		goto END;
	}
	
	res_set = mysql_store_result(conn);/*生成结果集*/
    if (res_set == NULL)
    {
        LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"smysql_store_result error: %s!\n", mysql_error(conn));
        ret = -1;
        goto END;
    }

    ulong line = 0;
    //mysql_num_rows接受由mysql_store_result返回的结果结构集，并返回结构集中的行数
    line = mysql_num_rows(res_set);
    if (line == 0)//没有结果
    {
        LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"mysql_num_rows(res_set) failed：%s\n", mysql_error(conn));
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
		//--UserOrderTable_OrderID
		if(row[0] != NULL){
			cJSON_AddNumberToObject(item,"UserOrderTable_OrderID",atoi(row[1]));
		}

		//--UserOrderTable_Productname
		if(row[1] != NULL){
			cJSON_AddStringToObject(item,"UserOrderTable_Productname",row[0]);
		}
		
		//--UserOrderTable_count
		if(row[2] != NULL){
			cJSON_AddNumberToObject(item,"UserOrderTable_count",atoi(row[1]));
		}

        //--UserOrderTable_time
		if(row[3] != NULL){
			cJSON_AddStringToObject(item,"UserOrderTable_time",row[2]);
		}
		
		cJSON_AddItemToArray(array,item);
	}
	
	cJSON_AddItemToObject(root,"UserOrder",array);
	
	out = cJSON_Print(root);
	
	LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"%s\n",out);
	
END:
	if(ret == 0){
		printf("%s",out); //给客户端返回信息
	}else{
		
        /*
			获取商品列表：
            成功：商品列表json
            失败：{"code": "015"}
        */
		out2 = NULL;
		out2 = return_status("015");
	}if(out2 != NULL){
		printf(out2);	//给客户端返回信息
		free(out2);
	}
    if(res_set != NULL)
        //完成所有对数据的操作后，调用mysql_free_result来善后处理
        mysql_free_result(res_set);

    if(conn != NULL)
        mysql_close(conn);

    if(root != NULL)
        cJSON_Delete(root);

    if(out != NULL)
        free(out);
    return ret;
}

int get_search_list(char *cmd,char *user,int start,int count,char *curUserName,char *searchTarget){
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
		LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"msql_conn err\n");
		ret = -1;
		goto END;
	}
	
	
    mysql_query(conn, "set names utf8");
	
	sprintf(sql_cmd,"select * from %s where ProductName = '%s'",curUserName,searchTarget);
	
	LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"%s 在操作\n",sql_cmd);
	
	if(mysql_query(conn,sql_cmd) != 0){
		LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"%s 操作失败:%s \n",sql_cmd,mysql_error(conn));
		ret = -1;
		goto END;
	}
	
	res_set = mysql_store_result(conn); /*生成结果集*/
	
	if (res_set == NULL)
    {
        LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"smysql_store_result error: %s!\n", mysql_error(conn));
        ret = -1;
        goto END;
    }

    ulong line = 0;
    //mysql_num_rows接受由mysql_store_result返回的结果结构集，并返回结构集中的行数
    line = mysql_num_rows(res_set);
    if (line == 0)//没有结果
    {
        LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"mysql_num_rows(res_set) failed：%s\n", mysql_error(conn));
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
		//--UserOrderTable_OrderID
		if(row[0] != NULL){
			cJSON_AddNumberToObject(item,"UserOrderTable_OrderID",atoi(row[1]));
		}

		//--UserOrderTable_Productname
		if(row[1] != NULL){
			cJSON_AddStringToObject(item,"UserOrderTable_Productname",row[0]);
		}
		
		//--UserOrderTable_count
		if(row[2] != NULL){
			cJSON_AddNumberToObject(item,"UserOrderTable_count",atoi(row[1]));
		}

        //--UserOrderTable_time
		if(row[3] != NULL){
			cJSON_AddStringToObject(item,"UserOrderTable_time",row[2]);
		}
		
		cJSON_AddItemToArray(array,item);
	}
	
	cJSON_AddItemToObject(root,"UserOrder",array);
	
	out = cJSON_Print(root);
	
	LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"%s\n",out);
	
END:
	if(ret == 0){
		printf("%s",out); //给客户端返回信息
	}else{
		
        /*
			获取商品列表：
            成功：商品列表json
            失败：{"code": "015"}
        */
		out2 = NULL;
		out2 = return_status("015");
	}if(out2 != NULL){
		printf(out2);	//给客户端返回信息
		free(out2);
	}
	
    if(res_set != NULL)
        //完成所有对数据的操作后，调用mysql_free_result来善后处理
        mysql_free_result(res_set);

    if(conn != NULL)
        mysql_close(conn);

    if(root != NULL)
        cJSON_Delete(root);
    if(out != NULL)
        free(out);
    return ret;
}

//解析上传的原料信息json包
int get_userOrder_info(char *buf,int *count,char *ProductName){
	int ret = 0;

    cJSON *root = cJSON_Parse(buf);
    if (NULL == root) {
        LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC, "cJSON_Parse err\n");
        ret = -1;
        goto END;
    }
	
	cJSON *child = cJSON_GetObjectItem(root, "count");
    if (NULL == child) {
        LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC, "cJSON_GetObjectItem err\n");
        ret = -1;
        goto END;
    }
    *count = child->valueint;
	
    child = cJSON_GetObjectItem(root, "ProductName");
    if (NULL == child) {
        LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC, "cJSON_GetObjectItem err\n");
        ret = -1;
        goto END;
    }
    strcpy(ProductName, child->valuestring);

END:
    if (root != NULL) {
        cJSON_Delete(root);
        root = NULL;
    }
    return ret;
}	

int userOrder_delete(char *buf,char* curUserName){
	int ret = 0;
	MYSQL *conn = NULL;
	
	char ProductName[128];
    int count;

	ret = get_userOrder_info(buf,&count,ProductName);
	if(ret != 0){
		goto END;
	}
	LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"count = %d, ProductName = %s",count,ProductName);
	conn = msql_conn(mysql_user,mysql_pwd,mysql_db);
	if(conn == NULL){
		LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"msql_conn err\n");
		ret = -1;
		goto END;
	}
	mysql_query(conn,"set names utf8");
	
	char sql_cmd[SQL_MAX_LEN] = {0};
	sprintf(sql_cmd,"DELETE FROM %s WHERE ProductName = %s;",curUserName,ProductName);
	int affected_rows = 0;
	int ret2 = process_no_result(conn,sql_cmd,&affected_rows);
	if(ret2 != 0){
        LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"%s 删除失败\n", sql_cmd);
		ret = -1;
        goto END;
	}else{
		LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"affected %d rows\n",affected_rows);
	}
	
END:
    if(conn != NULL)
        mysql_close(conn);

    return ret;
}

int userOrder_update(char *buf,char *curUserName){
	int ret = 0;
	MYSQL *conn = NULL;
	
	char ProductName[128];
    int count;

	ret = get_userOrder_info(buf,&count,ProductName);
	if(ret != 0){
		goto END;
	}
	LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"count = %d, ProductName = %s",count,ProductName);
    conn = msql_conn(mysql_user,mysql_pwd,mysql_db);
	if(conn == NULL){
		LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"msql_conn err\n");
		ret = -1;
		goto END;
	}
	mysql_query(conn,"set names utf8");
	
	char sql_cmd[SQL_MAX_LEN] = {0};
    sprintf(sql_cmd,"UPDATE %s SET ProductName = '%s',count = '%d'",curUserName,ProductName,count);
    int affected_rows = 0;
	int ret2 = process_no_result(conn,sql_cmd,&affected_rows);
	if(ret2 != 0){
        LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC, "%s 更新失败\n", sql_cmd);
		ret = -1;
        goto END;
	}else{
		LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"affected %d rows\n",affected_rows);
	}
	
END:
    if(conn != NULL)
        mysql_close(conn);

    return ret;
}

// 获取检索数目
void get_search_count(int ret,char *UserName,char *searchTarget){
	char sql_cmd[SQL_MAX_LEN] = {0};
	MYSQL *conn = NULL;
	long line = 0;
	
	
	conn = msql_conn(mysql_user,mysql_pwd,mysql_db);
	if(conn == NULL){
		LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"msql_conn err\n");
		goto END;
	}
	
	
    mysql_query(conn, "set names utf8");
	sprintf(sql_cmd,"select count(*) from %s where ProductName = '%s'",UserName,searchTarget);
	
	char tmp[512] = {0};
	
	int ret2 = process_result_one(conn,sql_cmd,tmp);	
	if(ret2 != 0){
		LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"%s 操作失败\n",sql_cmd);
		goto END;
	}

	line = atol(tmp); 
	
END:
	if(conn != NULL)
		mysql_close(conn);
	
	LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"line = %ld\n",line);
	
	
	return_login_status(line,ret);
}


int main(){
	// count 获取用户文件个数
    // display 获取用户文件信息，展示到前端
    char cmd[20];
    char user[USER_NAME_LEN];
    char token[TOKEN_LEN];

     // 读取数据库配置信息
    read_cfg();
	
    // 阻塞等待用户连接
    while (FCGI_Accept() >= 0)
    {

        // 获取URL地址 "?" 后面的内容
        char *query = getenv("QUERY_STRING");

        // 解析命令
        query_parse_key_value(query, "cmd", cmd, NULL);
        LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC, "cmd = %s\n", cmd);

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
            if (strcmp(cmd, "GetNames") == 0) //count 获取商品信息数目
            {
                int arraySize = Get_User_Count();
                char** UserNames;
                int ret = GetUserNameFromTable(&UserNames,arraySize);
                if(ret != 0){
					char *out = return_status("044"); 	// token 验证失败错误码
					if(out != NULL){
						printf(out); 	//给前端反馈错误码
						free(out);
					}
				}
                cJSON* json = convertArrayToJson(UserNames,arraySize);
                char* out = cJSON_Print(json);
                if(out != NULL){
                    printf(out);
                    free(out);
                }
            }
        }else{
            char buf[4*1024] = {0};
            char userOrdercountPrefix[] = "userOrdercount=";
            char userOrdernormalPrefix[] = "userOrdernormal=";
            char userOrderresultPrefix[] = "userOrderresult=";
            char userOrderdeletePrefix[] = "userOrderdelete=";
            char userOrderupdatePrefix[] = "userOrderupdate=";
            char searchTarget[100];
            int ret = 0;
            ret = fread(buf, 1, len, stdin); // 从标准输入(web服务器)读取内容
            if(ret == 0)
            {
                LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"fread(buf, 1, len, stdin) err\n");
                continue;
            }

            LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"buf = %s\n", buf);

            if(strncmp(cmd,userOrdercountPrefix,strlen(userOrdercountPrefix))==0){
            	char curUserName[100];
                query_parse_key_value(cmd,"userOrdercount",curUserName,NULL);
				
                get_count_json_info(buf, user, token); // 通过json包获取用户名, token

                // 验证登陆token，成功返回0，失败-1
				ret = verify_token(user, token); // util_cgi.h

                get_userOrder_count(ret,curUserName); // 获取商品信息个数
            }else if(strncmp(cmd,userOrdernormalPrefix,strlen(userOrdernormalPrefix)) == 0){	//初始化时获取商品列表的
                char curUserName[100];
                query_parse_key_value(cmd,"userOrdernormal",curUserName,NULL);
				int start;// 文件起点
				int count;// 文件个数
				get_userOrderlist_json_info(buf,user,token,&start,&count); // 通过json包获取信息
				LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"user = %s, token = %s, start = %d, count = %d\n", user, token, start, count);
			
				// 验证登录token，成功返回0，失败-1
				ret = verify_token(user,token);
				if(ret == 0){
					get_userOrder_list(user,start,count,curUserName);
				}
				else{
					char *out = return_status("111"); 	// token 验证失败错误码
					if(out != NULL){
						printf(out); 	//给前端反馈错误码
						free(out);
					}
				}
			}else if(strncmp(cmd,userOrderresultPrefix,strlen(userOrderresultPrefix)) == 0){	//搜索时获取商品列表的
                char curUserName[100];
                query_parse_key_value(cmd,"userOrderresult",curUserName,NULL);
				int start; //文件起点
				int count; // 文件个数
				get_userOrderlist_json_info(buf,user,token,&start,&count); // 通过json包获取信息
				LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"user = %s, token = %s, start = %d, count = %d\n", user, token, start, count);
				
				// 验证登录token，成功返回0，失败-1
				ret = verify_token(user,token);
				if(ret == 0){
					get_search_list(cmd,user,start,count,curUserName,searchTarget);	//获取商品列表
				}
				else{
					char *out = return_status("111"); 	// token 验证失败错误码
					if(out != NULL){
						printf(out); 	//给前端反馈错误码
						free(out);
					}
				}
			}else if(strncmp(cmd,userOrderdeletePrefix,strlen(userOrderdeletePrefix)) == 0){
                char curUserName[100];
                query_parse_key_value(cmd,"userOrderdelete",curUserName,NULL);
				ret = userOrder_delete(buf,curUserName);
				if (ret == 0) //上传成功
				{
					//返回前端删除情况， 023代表成功
					char *out = return_status("023");
					if(out != NULL){
						printf(out); 	//给前端反馈错误码
						free(out);
					}
				}
				else if(ret == -1)
				{
					//返回前端删除情况， 024代表失败
					char *out = return_status("024");
					if(out != NULL){
						printf(out); 	//给前端反馈错误码
						free(out);
					}
				}
			}else if(strncmp(cmd,userOrderupdatePrefix,strlen(userOrderupdatePrefix)) == 0){
                char curUserName[100];
                query_parse_key_value(cmd,"userOrderupdate",curUserName,NULL);
				ret = userOrder_update(buf,curUserName);
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
                char AcceptString[100];
	            char AcceptStringOri[100];
                query_parse_key_value(cmd,"userOrdersearch",AcceptStringOri,NULL);
				//将base64转码的关键字转为原来的文字
				base64_decode(AcceptStringOri,AcceptString);
				LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"AcceptStringOri = %s\n",AcceptStringOri);
				LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"AcceptString = %s\n",AcceptString);
				
                char UserName[100];
                char searchKeyword[100];
                char* delimiter = strchr(AcceptString,'&');
                if(delimiter != NULL){
                    int index = delimiter - AcceptString;
                    strncpy(UserName,AcceptString,index);
                    UserName[index]='\0';
                    strcpy(searchKeyword,delimiter+1);
                    LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"UserName = %s\n",UserName);
				    LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC,"searchKeyword = %s\n",searchKeyword);
                }
                strcpy(searchTarget,searchKeyword);
				get_count_json_info(buf, user, token); // 通过json包获取用户名, token
				
				// 验证登陆token，成功返回0，失败-1
				ret = verify_token(user, token); // util_cgi.h
				
				get_search_count(ret,UserName,searchTarget);
			}
        }
    }
    return 0;
}
