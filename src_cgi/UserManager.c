/**
 * @file UserManagertable.c
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

#define UserManager_LOG_MODULE       "cgi"
#define UserManager_LOG_PROC         "UserManager"

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
    LOG(UserManager_LOG_MODULE, UserManager_LOG_PROC, "mysql:[user=%s,pwd=%s,database=%s]", mysql_user, mysql_pwd, mysql_db);
}

int get_count_json_info(char *buf, char *user, char *token){
	
	int ret = 0;

	cJSON* root = cJSON_Parse(buf);
	if(NULL == root)
    {
		LOG(UserManager_LOG_MODULE, UserManager_LOG_PROC, "cJSON_Parse err\n");
        ret = -1;
        goto END;
	}

    cJSON *child1 = cJSON_GetObjectItem(root, "user");
    if(NULL == child1)
    {
        LOG(UserManager_LOG_MODULE, UserManager_LOG_PROC, "cJSON_GetObjectItem err\n");
        ret = -1;
        goto END;
    }

    strcpy(user, child1->valuestring); 

    
    cJSON *child2 = cJSON_GetObjectItem(root, "token");
    if(NULL == child2)
    {
        LOG(UserManager_LOG_MODULE, UserManager_LOG_PROC, "cJSON_GetObjectItem err\n");
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

//获取生产记录信息数目
void get_UserManager_count(int ret){
	char sql_cmd[SQL_MAX_LEN] = {0};
	MYSQL *conn = NULL;
	long line = 0;
	
    conn = msql_conn(mysql_user, mysql_pwd, mysql_db);
    if (conn == NULL)
    {
        LOG(UserManager_LOG_MODULE, UserManager_LOG_PROC, "msql_conn err\n");
        goto END;
    }
	
    mysql_query(conn, "set names utf8");

    sprintf(sql_cmd, "select count(*) from user");
    char tmp[512] = {0};
    
    int ret2 = process_result_one(conn, sql_cmd, tmp); 
    if(ret2 != 0)
    {
        LOG(UserManager_LOG_MODULE, UserManager_LOG_PROC, "%s 操作失败\n", sql_cmd);
        goto END;
    }

    line = atol(tmp); 

END:
    if(conn != NULL)
    {
        mysql_close(conn);
    }

    LOG(UserManager_LOG_MODULE, UserManager_LOG_PROC, "line = %ld\n", line);

    return_login_status(line, ret);
}

//解析的json包
int get_UserManagerlist_json_info(char *buf,char *user,char *token,int *p_start,int *p_count){
	int ret = 0;
	
	cJSON * root = cJSON_Parse(buf);
	if(NULL == root){
		LOG(UserManager_LOG_MODULE,UserManager_LOG_PROC,"cJSON_Parse err\n");
		ret =-1;
		goto END;
	}
	cJSON* child1 = cJSON_GetObjectItem(root,"user");
	if(NULL == child1){
		LOG(UserManager_LOG_MODULE,UserManager_LOG_PROC,"cJSON_GetObjectItem err\n");
		ret = -1;
		goto END;
	}
	strcpy(user, child1->valuestring);
	
	cJSON* child2 = cJSON_GetObjectItem(root,"token");
	if(NULL == child2){
		LOG(UserManager_LOG_MODULE,UserManager_LOG_PROC,"cJSON_GetObjectItem err\n");
		ret = -1;
		goto END;
	}
	strcpy(token, child2->valuestring);
	
	cJSON* child3 = cJSON_GetObjectItem(root,"start");
	if(NULL == child3){
		LOG(UserManager_LOG_MODULE,UserManager_LOG_PROC,"cJSON_GetObjectItem err\n");
		ret = -1;
		goto END;
	}
	*p_start = child3->valueint; 

	cJSON* child4 = cJSON_GetObjectItem(root,"count");
	if(NULL == child4){
		LOG(UserManager_LOG_MODULE,UserManager_LOG_PROC,"cJSON_GetObjectItem err\n");
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
		LOG(UserManager_LOG_MODULE,UserManager_LOG_PROC,"msql_conn err\n");
		goto END;
	}
	
    mysql_query(conn, "set names utf8");
	
	if(is_number(keyword)){
		sprintf(sql_cmd,"select count(*) from user where id = '%s'",keyword);
	}else{
		sprintf(sql_cmd,"select count(*) from user where name = '%s'",keyword);
	}
	
	char tmp[512] = {0};
	
	int ret2 = process_result_one(conn,sql_cmd,tmp);	
	if(ret2 != 0){
		LOG(UserManager_LOG_MODULE,UserManager_LOG_PROC,"%s 操作失败\n",sql_cmd);
		goto END;
	}

	line = atol(tmp); 
	
END:
	if(conn != NULL){
		mysql_close(conn);
	}
	
	LOG(UserManager_LOG_MODULE,UserManager_LOG_PROC,"line = %ld\n",line);
	
	return_login_status(line,ret);
}

// 获取检索生产记录列表
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
		LOG(UserManager_LOG_MODULE,UserManager_LOG_PROC,"msql_conn err\n");
		ret = -1;
		goto END;
	}
	
    mysql_query(conn, "set names utf8");
	
	if(is_number(keyword)){
		sprintf(sql_cmd,"select * from user where id = '%s'",keyword);
	}else{
		sprintf(sql_cmd,"select * from user where name = '%s'",keyword);
	}
	
	LOG(UserManager_LOG_MODULE,UserManager_LOG_PROC,"%s 在操作\n",sql_cmd);
	
	if(mysql_query(conn,sql_cmd) != 0){
		LOG(UserManager_LOG_MODULE,UserManager_LOG_PROC,"%s 操作失败:%s \n",sql_cmd,mysql_error(conn));
		ret = -1;
		goto END;
	}
	
	res_set = mysql_store_result(conn);
	
	if (res_set == NULL)
    {
        LOG(UserManager_LOG_MODULE, UserManager_LOG_PROC, "smysql_store_result error: %s!\n", mysql_error(conn));
        ret = -1;
        goto END;
    }

    ulong line = 0;
    line = mysql_num_rows(res_set);
    if (line == 0)
    {
        LOG(UserManager_LOG_MODULE, UserManager_LOG_PROC, "mysql_num_rows(res_set) failed：%s\n", mysql_error(conn));
        ret = -1;
        goto END;
    }
	
	MYSQL_ROW row;
	
	root = cJSON_CreateObject();
	array = cJSON_CreateArray();
	while((row = mysql_fetch_row(res_set)) != NULL){
		cJSON *item = cJSON_CreateObject();
		if(row[0] != NULL){
			cJSON_AddNumberToObject(item,"id",atoi(row[0]));
		}
		if(row[1] != NULL){
			cJSON_AddStringToObject(item,"name",row[1]);
		}
		if(row[2] != NULL){
			cJSON_AddStringToObject(item,"nickname",row[2]);
		}
		if(row[3] != NULL){
			cJSON_AddStringToObject(item,"password",row[3]);
		}
		if(row[4] != NULL){
			cJSON_AddStringToObject(item,"phone",row[4]);
		}
		if(row[5] != NULL){
			cJSON_AddStringToObject(item,"createtime",row[5]);
		}
		if(row[6] != NULL){
			cJSON_AddStringToObject(item,"email",row[6]);
		}
		if(row[7] != NULL){
			cJSON_AddNumberToObject(item,"power",atoi(row[7]));
		}
		
		cJSON_AddItemToArray(array,item);
	}
	
	cJSON_AddItemToObject(root,"UserManager",array);
	
	out = cJSON_Print(root);
	
	LOG(UserManager_LOG_MODULE,UserManager_LOG_PROC,"%s\n",out);
	
END:
	if(ret == 0){
		printf("%s",out);
	}else{
		out2 = NULL;
		out2 = return_status("015");
	}if(out2 != NULL){
		printf(out2);
		free(out2);
	}
	
    if(res_set != NULL)
        mysql_free_result(res_set);
    if(conn != NULL)
        mysql_close(conn);
    if(root != NULL)
        cJSON_Delete(root);
    if(out != NULL)
        free(out);

    return ret;
}

int get_UserManager_info(char* buf, int* id, char* name, char* nickname, char* password,char* phone,char* createtime,char* email,int* power) {
	int ret = 0;

    cJSON *root = cJSON_Parse(buf);
    if (NULL == root) {
        LOG(UserManager_LOG_MODULE, UserManager_LOG_PROC, "cJSON_Parse err\n");
        ret = -1;
        goto END;
    }
	
	cJSON *child = cJSON_GetObjectItem(root, "id");
    if (NULL == child) {
        LOG(UserManager_LOG_MODULE, UserManager_LOG_PROC, "cJSON_GetObjectItem err\n");
        ret = -1;
        goto END;
    }
    *id = child->valueint;
	
    child = cJSON_GetObjectItem(root, "name");
    if (NULL == child) {
        LOG(UserManager_LOG_MODULE, UserManager_LOG_PROC, "cJSON_GetObjectItem err\n");
        ret = -1;
        goto END;
    }
    strcpy(name, child->valuestring);

    child = cJSON_GetObjectItem(root, "nickname");
    if (NULL == child) {
        LOG(UserManager_LOG_MODULE, UserManager_LOG_PROC, "cJSON_GetObjectItem err\n");
        ret = -1;
        goto END;
    }
    strcpy(nickname, child->valuestring);
	
    child = cJSON_GetObjectItem(root, "password");
    if (NULL == child) {
        LOG(UserManager_LOG_MODULE, UserManager_LOG_PROC, "cJSON_GetObjectItem err\n");
        ret = -1;
        goto END;
    }
    strcpy(password, child->valuestring);

	    child = cJSON_GetObjectItem(root, "phone");
    if (NULL == child) {
        LOG(UserManager_LOG_MODULE, UserManager_LOG_PROC, "cJSON_GetObjectItem err\n");
        ret = -1;
        goto END;
    }
    strcpy(phone, child->valuestring);

	    child = cJSON_GetObjectItem(root, "createtime");
    if (NULL == child) {
        LOG(UserManager_LOG_MODULE, UserManager_LOG_PROC, "cJSON_GetObjectItem err\n");
        ret = -1;
        goto END;
    }
    strcpy(createtime, child->valuestring);

	    child = cJSON_GetObjectItem(root, "email");
    if (NULL == child) {
        LOG(UserManager_LOG_MODULE, UserManager_LOG_PROC, "cJSON_GetObjectItem err\n");
        ret = -1;
        goto END;
    }
    strcpy(email, child->valuestring);

	    child = cJSON_GetObjectItem(root, "power");
    if (NULL == child) {
        LOG(UserManager_LOG_MODULE, UserManager_LOG_PROC, "cJSON_GetObjectItem err\n");
        ret = -1;
        goto END;
    }
    *power = child->valueint;

END:
    if (root != NULL) {
        cJSON_Delete(root);
        root = NULL;
    }
    return ret;
}

//更新生产记录信息
int UserManager_update(char* buf) {
    int ret = 0;
    MYSQL* conn = NULL;

    int id;
    char name[128];
    char nickname[128];
    char password[128];
	char phone[128];
	char createtime[128];
	char email[128];
	int power;

    ret = get_UserManager_info(buf, &id, name, nickname, password,phone,createtime,email,&power);
    if (ret != 0) {
        goto END;
    }
    LOG(UserManager_LOG_MODULE, UserManager_LOG_PROC, "id = %d, name = %s, nickname = %s, password = %s,phone = %s,createtime = %s,email = %s,power = %s", id, name, nickname, password,phone,createtime,email,power);
    
	conn = msql_conn(mysql_user, mysql_pwd, mysql_db);
    if (conn == NULL) {
        LOG(UserManager_LOG_MODULE, UserManager_LOG_PROC, "msql_conn err\n");
        ret = -1;
        goto END;
    }
    mysql_query(conn, "set names utf8");

    char sql_cmd[SQL_MAX_LEN] = {0};
    sprintf(sql_cmd, "UPDATE user SET name = '%s', nickname = '%s', password = '%s',phone = '%s',createtime = '%s',email = '%s',power = %d WHERE id = %d", name, nickname, password,phone,createtime,email,power,id);
    int affected_rows = 0;
    int ret2 = process_no_result(conn, sql_cmd, &affected_rows);
    if (ret2 != 0) {
        LOG(UserManager_LOG_MODULE, UserManager_LOG_PROC, "%s 更新失败\n", sql_cmd);
        ret = -1;
        goto END;
    } else {
        LOG(UserManager_LOG_MODULE, UserManager_LOG_PROC, "affected %d rows\n", affected_rows);
    }

END:
    if (conn != NULL) {
        mysql_close(conn);
    }

    return ret;
}

int UserManager_delete(char *buf){
	int ret = 0;
	MYSQL *conn = NULL;

    int id;
    char name[128];
    char nickname[128];
    char password[128];
	char phone[128];
	char createtime[128];
	char email[128];
	int power;

    ret = get_UserManager_info(buf, &id, name, nickname, password,phone,createtime,email,&power);
    if (ret != 0) {
        goto END;
    }
    LOG(UserManager_LOG_MODULE, UserManager_LOG_PROC, "id = %d, name = %s, nickname = %s, password = %s,phone = %s,createtime = %s,email = %s,power = %s", id, name, nickname, password,phone,createtime,email,power);
	conn = msql_conn(mysql_user,mysql_pwd,mysql_db);
	if(conn == NULL){
		LOG(UserManager_LOG_MODULE,UserManager_LOG_PROC,"msql_conn err\n");
		ret = -1;
		goto END;
	}
	mysql_query(conn,"set names utf8");
	
	char sql_cmd[SQL_MAX_LEN] = {0};
	sprintf(sql_cmd,"DELETE FROM user WHERE id = '%d';",id);
	int affected_rows = 0;
	int ret2 = process_no_result(conn,sql_cmd,&affected_rows);
	if(ret2 != 0){
        LOG(UserManager_LOG_MODULE, UserManager_LOG_PROC, "%s 删除失败\n", sql_cmd);
		ret = -1;
        goto END;
	}else{
		LOG(UserManager_LOG_MODULE,UserManager_LOG_PROC,"affected %d rows\n",affected_rows);
	}
	
END:
    if(conn != NULL)
    {
        mysql_close(conn);
    }

    return ret;
}

//添加原料信息
int UserManager_add(char* buf) {
    int ret = 0;
    MYSQL* conn = NULL;

    int id;
    char name[128];
    char nickname[128];
    char password[128];
	char phone[128];
	char createtime[128];
	char email[128];
	int power;

    ret = get_UserManager_info(buf, &id, name, nickname, password,phone,createtime,email,&power);
    if (ret != 0) {
        goto END;
    }
    LOG(UserManager_LOG_MODULE, UserManager_LOG_PROC, "id = %d, name = %s, nickname = %s, password = %s,phone = %s,createtime = %s,email = %s,power = %s", id, name, nickname, password,phone,createtime,email,power);
    conn = msql_conn(mysql_user, mysql_pwd, mysql_db);
    if (conn == NULL) {
        LOG(UserManager_LOG_MODULE, UserManager_LOG_PROC, "msql_conn err\n");
        ret = -1;
        goto END;
    }
    mysql_query(conn, "set names utf8");

    char sql_cmd[SQL_MAX_LEN] = {0};

    sprintf(sql_cmd, "INSERT INTO user (id, name, nickname, password,phone,createtime,email,power) VALUES ('%d', '%s', '%s', '%s','%s','%s','%s','%d')", id, name, nickname, password,phone,createtime,email,power);
    int affected_rows = 0;
    int ret2 = process_no_result(conn, sql_cmd, &affected_rows);
    if (ret2 != 0) {
        LOG(UserManager_LOG_MODULE, UserManager_LOG_PROC, "%s 插入失败\n", sql_cmd);
        ret = -1;
        goto END;
    } else {
        LOG(UserManager_LOG_MODULE, UserManager_LOG_PROC, "affected %d rows\n", affected_rows);
    }

END:
    if (conn != NULL) {
        mysql_close(conn);
    }

    return ret;
}

//获取生产记录列表
int get_UserManager_list(char *cmd,char *user,int start,int count){
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
        LOG(UserManager_LOG_MODULE, UserManager_LOG_PROC, "msql_conn err\n");
        ret = -1;
        goto END;
    }

    mysql_query(conn, "set names utf8");
	
	if(strcmp(cmd,"UserManagerNormal") == 0) // 获取商品信息
	{
		//sql语句
		sprintf(sql_cmd,"SELECT * FROM user limit %d,%d \n",start,count);
	}
	
	LOG(UserManager_LOG_MODULE,UserManager_LOG_PROC,"%s 在操作\n",sql_cmd);
	
	if(mysql_query(conn,sql_cmd) != 0){
		LOG(UserManager_LOG_MODULE,UserManager_LOG_PROC,"%s 操作失败:%s \n",sql_cmd,mysql_error(conn));
		ret = -1;
		goto END;
	}
	
	res_set = mysql_store_result(conn);/*生成结果集*/
    if (res_set == NULL)
    {
        LOG(UserManager_LOG_MODULE, UserManager_LOG_PROC, "smysql_store_result error: %s!\n", mysql_error(conn));
        ret = -1;
        goto END;
    }

    ulong line = 0;
    line = mysql_num_rows(res_set);
    if (line == 0)//没有结果
    {
        LOG(UserManager_LOG_MODULE, UserManager_LOG_PROC, "mysql_num_rows(res_set) failed：%s\n", mysql_error(conn));
        ret = -1;
        goto END;
    }
	
	MYSQL_ROW row;
	
	root = cJSON_CreateObject();
	array = cJSON_CreateArray();
	while((row = mysql_fetch_row(res_set)) != NULL){
		cJSON *item = cJSON_CreateObject();
		
		if(row[0] != NULL){
			cJSON_AddNumberToObject(item,"id",atoi(row[0]));
		}
		if(row[1] != NULL){
			cJSON_AddStringToObject(item,"name",row[1]);
		}
		if(row[2] != NULL){
			cJSON_AddStringToObject(item,"nickname",row[2]);
		}
		if(row[3] != NULL){
			cJSON_AddStringToObject(item,"password",row[3]);
		}
		if(row[4] != NULL){
			cJSON_AddStringToObject(item,"phone",row[4]);
		}
		if(row[5] != NULL){
			cJSON_AddStringToObject(item,"createtime",row[5]);
		}
		if(row[6] != NULL){
			cJSON_AddStringToObject(item,"email",row[6]);
		}
		if(row[7] != NULL){
			cJSON_AddNumberToObject(item,"power",atoi(row[7]));
		}
		
		cJSON_AddItemToArray(array,item);
	}
	
	cJSON_AddItemToObject(root,"UserManager",array);
	
	out = cJSON_Print(root);
	
	LOG(UserManager_LOG_MODULE,UserManager_LOG_PROC,"%s\n",out);
	
END:
	if(ret == 0){
		printf("%s",out);
	}else{
		out2 = NULL;
		out2 = return_status("015");
	}if(out2 != NULL){
		printf(out2);
		free(out2);
	}
	
    if(res_set != NULL)
        mysql_free_result(res_set);

    if(conn != NULL)
        mysql_close(conn);

    if(root != NULL)
        cJSON_Delete(root);

    if(out != NULL)
        free(out);

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

        query_parse_key_value(query, "cmd", cmd, NULL);
        LOG(UserManager_LOG_MODULE, UserManager_LOG_PROC, "cmd = %s\n", cmd);

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
            LOG(UserManager_LOG_MODULE, UserManager_LOG_PROC, "len = 0, No data from standard input\n");
        }
        else
        {
            char buf[4*1024] = {0};
            int ret = 0;
            ret = fread(buf, 1, len, stdin);
            if(ret == 0)
            {
                LOG(UserManager_LOG_MODULE, UserManager_LOG_PROC, "fread(buf, 1, len, stdin) err\n");
                continue;
            }

            LOG(UserManager_LOG_MODULE, UserManager_LOG_PROC, "buf = %s\n", buf);

            if (strcmp(cmd, "UserManagerCount") == 0) 
            {
                get_count_json_info(buf, user, token); 

				ret = verify_token(user, token);

                get_UserManager_count(ret);

            }else if(strcmp(cmd,"UserManagerNormal") == 0){
				int start;
				int count;
				get_UserManagerlist_json_info(buf,user,token,&start,&count); 
				LOG(UserManager_LOG_MODULE,UserManager_LOG_PROC, "user = %s, token = %s, start = %d, count = %d\n", user, token, start, count);
			
				ret = verify_token(user,token);
				if(ret == 0){
					get_UserManager_list(cmd,user,start,count);
				}
				else{
					char *out = return_status("111");
					if(out != NULL){
						printf(out); 
						free(out);
					}
				}
			}else if(strcmp(cmd,"UserManagerResult") == 0){	
				int start;
				int count;
				get_UserManagerlist_json_info(buf,user,token,&start,&count); 
				LOG(UserManager_LOG_MODULE,UserManager_LOG_PROC,"user = %s, token = %s, start = %d, count = %d\n", user, token, start, count);
				
				ret = verify_token(user,token);
				if(ret == 0){
					get_search_list(cmd,user,start,count,keyword);
				}
				else{
					char *out = return_status("111");
					if(out != NULL){
						printf(out); 
						free(out);
					}
				}
			}else if(strcmp(cmd,"UserManagerDelete") == 0){
				ret = UserManager_delete(buf);
				if (ret == 0)
				{
					char *out = return_status("023");
					if(out != NULL){
						printf(out); 
						free(out);
					}
				}
				else if(ret == -1)
				{
					char *out = return_status("024");
					if(out != NULL){
						printf(out); 
						free(out);
					}
				}
			}else if(strcmp(cmd,"UserManagerUpdate") == 0){
				ret = UserManager_update(buf);
				if (ret == 0)
				{
					char *out = return_status("020");
					if(out != NULL){
						printf(out); 
						free(out);
					}
				}
				else if(ret == -1)
				{
					char *out = return_status("021");
					if(out != NULL){
						printf(out); 
						free(out);
					}
				}
			}else if(strcmp(cmd,"UserManagerAdd") == 0){
				ret = UserManager_add(buf);
				if (ret == 0)
				{
					char *out = return_status("020");
					if(out != NULL){
						printf(out); 
						free(out);
					}
				}
				else if(ret == -1)
				{
					char *out = return_status("021");
					if(out != NULL){
						printf(out); 
						free(out);
					}
				}
			}else{
				query_parse_key_value(cmd,"UserManagerSearch",keywordori,NULL);
				base64_decode(keywordori,keyword);
				LOG(UserManager_LOG_MODULE,UserManager_LOG_PROC,"keywordori = %s\n",keywordori);
				LOG(UserManager_LOG_MODULE,UserManager_LOG_PROC,"keyword = %s\n",keyword);
				
				get_count_json_info(buf, user, token);
				
				ret = verify_token(user, token); 
				
				get_search_count(ret,keyword);
			}
        }
    }
    return 0;
}
