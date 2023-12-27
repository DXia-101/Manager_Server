/**
 * @file UserOrder.c
 * @brief   登陆后台CGI程序
 * @author Mike
 * @version 2.0
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
        token = "110"; //成功
    }
    else
    {
        token = "111"; //失败
    }

    //数字
    sprintf(num_buf, "%ld", num);

    cJSON *root = cJSON_CreateObject();  //创建json项目
    cJSON_AddStringToObject(root, "num", num_buf);// {"num":"1111"}
    cJSON_AddStringToObject(root, "code", token);// {"code":"110"}
    out = cJSON_Print(root);//cJSON to string(char *)

    cJSON_Delete(root);

    if(out != NULL)
    {
        printf(out); //给前端反馈信息
        free(out); //记得释放
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
	
	//connect the database
    conn = msql_conn(mysql_user, mysql_pwd, mysql_db);
    if (conn == NULL)
    {
        LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC, "msql_conn err\n");
        goto END;
    }
	
	//设置数据库编码，主要处理中文编码问题
    mysql_query(conn, "set names utf8");

    sprintf(sql_cmd, "select count(*) from user");
    char tmp[512] = {0};
    //返回值： 0成功并保存记录集，1没有记录集，2有记录集但是没有保存，-1失败
    int ret = process_result_one(conn, sql_cmd, tmp); //指向sql语句
    if(ret != 0)
    {
        LOG(USERORDER_LOG_MODULE, USERORDER_LOG_PROC, "%s 操作失败\n", sql_cmd);
        goto END;
    }

    return atol(tmp); //字符串转长整形
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
        }
    }
    return 0;
}
