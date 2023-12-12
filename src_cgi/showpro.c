/**
 * @file showprotable.c
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

#define SHOWPRO_LOG_MODULE       "cgi"
#define SHOWPRO_LOG_PROC         "showpro"

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
    LOG(SHOWPRO_LOG_MODULE, SHOWPRO_LOG_PROC, "mysql:[user=%s,pwd=%s,database=%s]", mysql_user, mysql_pwd, mysql_db);
}

//解析的json包
int get_showpro_json_info(char *buf,char *user,char *token,int *p_start,int *p_count){
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
		LOG(SHOWPRO_LOG_MODULE, SHOWPRO_LOG_PROC,"cJSON_Parse err\n");
		ret =-1;
		goto END;
	}
	
	// 返回指定字符串对应的json对象
	// 用户
	cJSON* child1 = cJSON_GetObjectItem(root,"user");
	if(NULL == child1){
		LOG(SHOWPRO_LOG_MODULE, SHOWPRO_LOG_PROC,"cJSON_GetObjectItem err\n");
		ret = -1;
		goto END;
	}
	strcpy(user, child1->valuestring); //拷贝内容
	
	// 返回指定字符串对应的json对象
	// token
	cJSON* child2 = cJSON_GetObjectItem(root,"token");
	if(NULL == child2){
		LOG(SHOWPRO_LOG_MODULE, SHOWPRO_LOG_PROC,"cJSON_GetObjectItem err\n");
		ret = -1;
		goto END;
	}
	strcpy(token, child2->valuestring); //拷贝内容
	
	// 返回指定字符串对应的json对象
	// 文件起点
	cJSON* child3 = cJSON_GetObjectItem(root,"start");
	if(NULL == child3){
		LOG(SHOWPRO_LOG_MODULE, SHOWPRO_LOG_PROC,"cJSON_GetObjectItem err\n");
		ret = -1;
		goto END;
	}
	*p_start = child3->valueint; //拷贝内容

	// 返回指定字符串对应的json对象
	// 文件请求个数
	cJSON* child4 = cJSON_GetObjectItem(root,"count");
	if(NULL == child4){
		LOG(SHOWPRO_LOG_MODULE, SHOWPRO_LOG_PROC,"cJSON_GetObjectItem err\n");
		ret = -1;
		goto END;
	}
	*p_count = child4->valueint; //拷贝内容
	
END:
	if(NULL != root){
		cJSON_Delete(root);	//删除json对象
		root = NULL;
	}
	
	return ret;
}

int Get_Upload_Json_Info(char *buf,const char*** keys,int** values){
    cJSON* root = cJSON_Parse(buf);
    if (root == NULL)
    {
        printf("Failed to parse JSON data.\n");
        return;
    }

    // 获取第一行的key
    cJSON* firstItem = root->child;
    const char* firstKey = firstItem->string;
    LOG(SHOWPRO_LOG_MODULE, SHOWPRO_LOG_PROC,"First Key: %s\n", firstKey);

    // 遍历之后的数据，保存为key数组和value
    int itemCount = cJSON_GetArraySize(root);
    *keys = (const char**)malloc(itemCount * sizeof(const char*));
    *values = (int*)malloc(itemCount * sizeof(int));
    int i = 0;
    cJSON* currentItem = firstItem->next;
    while (currentItem != NULL)
    {
        (*keys)[i] = currentItem->string;
        (*values)[i] = currentItem->valueint;
        i++;
        currentItem = currentItem->next;
    }

    cJSON_Delete(root);
    return itemCount;
}

//解析的json包, 登陆token
int get_count_json_info(char *buf, char *user, char *token){
	
	int ret = 0;
	
	/*json数据如下
    {
        "token": "9e894efc0b2a898a82765d0a7f2c94cb",
        user:xxxx
    }
    */
	//解析json包
	//解析一个json字符串为json对象
	cJSON* root = cJSON_Parse(buf);
	if(NULL == root)
    {
		LOG(SHOWPRO_LOG_MODULE, SHOWPRO_LOG_PROC, "cJSON_Parse err\n");
        ret = -1;
        goto END;
	}
	
	//返回指定字符串对应的json对象
    //用户
    cJSON *child1 = cJSON_GetObjectItem(root, "user");
    if(NULL == child1)
    {
        LOG(SHOWPRO_LOG_MODULE, SHOWPRO_LOG_PROC, "cJSON_GetObjectItem err\n");
        ret = -1;
        goto END;
    }

    //LOG(SHOWPRO_LOG_MODULE, SHOWPRO_LOG_PROC, "child1->valuestring = %s\n", child1->valuestring);
    strcpy(user, child1->valuestring); //拷贝内容

    //登陆token
    cJSON *child2 = cJSON_GetObjectItem(root, "token");
    if(NULL == child2)
    {
        LOG(SHOWPRO_LOG_MODULE, SHOWPRO_LOG_PROC, "cJSON_GetObjectItem err\n");
        ret = -1;
        goto END;
    }

    //LOG(SHOWPRO_LOG_MODULE, SHOWPRO_LOG_PROC, "child2->valuestring = %s\n", child2->valuestring);
    strcpy(token, child2->valuestring); //拷贝内容

END:
    if(root != NULL)
    {
        cJSON_Delete(root);//删除json对象
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

//获取商品信息数目
void get_showpro_count(int ret){
	char sql_cmd[SQL_MAX_LEN] = {0};
	MYSQL *conn = NULL;
	long line = 0;
	
	//connect the database
    conn = msql_conn(mysql_user, mysql_pwd, mysql_db);
    if (conn == NULL)
    {
        LOG(SHOWPRO_LOG_MODULE, SHOWPRO_LOG_PROC, "msql_conn err\n");
        goto END;
    }
	
	//设置数据库编码，主要处理中文编码问题
    mysql_query(conn, "set names utf8");

    sprintf(sql_cmd, "select count(*) from showproinfo");
    char tmp[512] = {0};
    //返回值： 0成功并保存记录集，1没有记录集，2有记录集但是没有保存，-1失败
    int ret2 = process_result_one(conn, sql_cmd, tmp); //指向sql语句
    if(ret2 != 0)
    {
        LOG(SHOWPRO_LOG_MODULE, SHOWPRO_LOG_PROC, "%s 操作失败\n", sql_cmd);
        goto END;
    }

    line = atol(tmp); //字符串转长整形

END:
    if(conn != NULL)
    {
        mysql_close(conn);
    }

    LOG(SHOWPRO_LOG_MODULE, SHOWPRO_LOG_PROC, "line = %ld\n", line);

    //给前端反馈的信息
    return_login_status(line, ret);
}


//获取商品列表
int get_showpro_list(char *cmd,char *user,int start,int count){
	int ret = 0;
	char sql_cmd[SQL_MAX_LEN] = {0};
	MYSQL *conn = NULL;
    cJSON *root = NULL;
    cJSON *array =NULL;
    char *out = NULL;
    char *out2 = NULL;
    MYSQL_RES *res_set = NULL;
	
    //connect the database
    conn = msql_conn(mysql_user, mysql_pwd, mysql_db);
    if (conn == NULL)
    {
        LOG(SHOWPRO_LOG_MODULE, SHOWPRO_LOG_PROC, "msql_conn err\n");
        ret = -1;
        goto END;
    }

    //设置数据库编码，主要处理中文编码问题
    mysql_query(conn, "set names utf8");
	
	if(strcmp(cmd,"showpronormal") == 0) // 获取商品信息
	{
		//sql语句
		sprintf(sql_cmd,"SELECT * FROM showproinfo limit %d,%d \n",start,count);
	}
	
	LOG(SHOWPRO_LOG_MODULE,SHOWPRO_LOG_PROC,"%s 在操作\n",sql_cmd);
	
	if(mysql_query(conn,sql_cmd) != 0){
		LOG(SHOWPRO_LOG_MODULE,SHOWPRO_LOG_PROC,"%s 操作失败:%s \n",sql_cmd,mysql_error(conn));
		ret = -1;
		goto END;
	}
	
	res_set = mysql_store_result(conn);/*生成结果集*/
    if (res_set == NULL)
    {
        LOG(SHOWPRO_LOG_MODULE, SHOWPRO_LOG_PROC, "smysql_store_result error: %s!\n", mysql_error(conn));
        ret = -1;
        goto END;
    }

    ulong line = 0;
    //mysql_num_rows接受由mysql_store_result返回的结果结构集，并返回结构集中的行数
    line = mysql_num_rows(res_set);
    if (line == 0)//没有结果
    {
        LOG(SHOWPRO_LOG_MODULE, SHOWPRO_LOG_PROC, "mysql_num_rows(res_set) failed：%s\n", mysql_error(conn));
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
        
		//--showpro_id
		if(row[0] != NULL){
			cJSON_AddNumberToObject(item,"showpro_id",atoi(row[0]));
		}
		
		//--showpro_name
		if(row[1] != NULL){
			cJSON_AddStringToObject(item,"showpro_name",row[1]);
		}
		
		//showpro_url
		if(row[2] != NULL){
			cJSON_AddStringToObject(item,"showpro_url",row[2]);
		}
		
		cJSON_AddItemToArray(array,item);
	}
	
	cJSON_AddItemToObject(root,"showpro",array);
	
	out = cJSON_Print(root);
	
	LOG(SHOWPRO_LOG_MODULE,SHOWPRO_LOG_PROC,"%s\n",out);
	
END:
	if(ret == 0){
		printf("%s",out); //给客户端返回信息
	}else{
		//失败
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

     // 读取数据库配置信息
    read_cfg();
	
    // 阻塞等待用户连接
    while (FCGI_Accept() >= 0)
    {

        // 获取URL地址 "?" 后面的内容
        char *query = getenv("QUERY_STRING");

        // 解析命令
        query_parse_key_value(query, "cmd", cmd, NULL);
        LOG(SHOWPRO_LOG_MODULE, SHOWPRO_LOG_PROC, "cmd = %s\n", cmd);

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
            LOG(SHOWPRO_LOG_MODULE, SHOWPRO_LOG_PROC, "len = 0, No data from standard input\n");
        }
        else
        {
            char buf[4*1024] = {0};
            int ret = 0;
            ret = fread(buf, 1, len, stdin); // 从标准输入(web服务器)读取内容
            if(ret == 0)
            {
                LOG(SHOWPRO_LOG_MODULE, SHOWPRO_LOG_PROC, "fread(buf, 1, len, stdin) err\n");
                continue;
            }

            LOG(SHOWPRO_LOG_MODULE, SHOWPRO_LOG_PROC, "buf = %s\n", buf);

            if (strcmp(cmd, "showprocount") == 0) //count 获取商品信息数目
            {
                get_count_json_info(buf, user, token); // 通过json包获取用户名, token

                // 验证登陆token，成功返回0，失败-1
				ret = verify_token(user, token); // util_cgi.h

                get_showpro_count(ret); // 获取商品信息个数

            }else if(strcmp(cmd,"showpronormal") == 0){	//初始化时获取商品列表的
				int start;// 文件起点
				int count;// 文件个数
				get_showpro_json_info(buf,user,token,&start,&count); // 通过json包获取信息
				LOG(SHOWPRO_LOG_MODULE,SHOWPRO_LOG_PROC, "user = %s, token = %s, start = %d, count = %d\n", user, token, start, count);
			
				// 验证登录token，成功返回0，失败-1
				ret = verify_token(user,token);
				if(ret == 0){
					get_showpro_list(cmd,user,start,count);	//获取商品列表
				}
				else{
					char *out = return_status("111"); 	// token 验证失败错误码
					if(out != NULL){
						printf(out); 	//给前端反馈错误码
						free(out);
					}
				}
			}else if(strcmp(cmd,"upload") == 0){
                const char** keys;
                int* values;
                int itemCount = Get_Upload_Json_Info(buf, &keys, &values); //通过Json获取上传来的购物数据
                for (int i = 0; i < itemCount; i++) {
                    const char* key = keys[i];
                    int value = values[i];
                    // 在这里处理key和value
                }

                // 释放内存
                free(keys);
                free(values);
            }
        }
    }
    return 0;
}