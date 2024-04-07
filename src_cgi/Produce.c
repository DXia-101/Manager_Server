/**
 * @file Producetable.c
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

#define Produce_LOG_MODULE       "cgi"
#define Produce_LOG_PROC         "Produce"

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
    LOG(Produce_LOG_MODULE, Produce_LOG_PROC, "mysql:[user=%s,pwd=%s,database=%s]", mysql_user, mysql_pwd, mysql_db);
}

//解析的json包, 登陆token
int get_count_json_info(char *buf, char *user, char *token){
	
	int ret = 0;

	cJSON* root = cJSON_Parse(buf);
	if(NULL == root)
    {
		LOG(Produce_LOG_MODULE, Produce_LOG_PROC, "cJSON_Parse err\n");
        ret = -1;
        goto END;
	}

    cJSON *child1 = cJSON_GetObjectItem(root, "user");
    if(NULL == child1)
    {
        LOG(Produce_LOG_MODULE, Produce_LOG_PROC, "cJSON_GetObjectItem err\n");
        ret = -1;
        goto END;
    }

    strcpy(user, child1->valuestring); 

    
    cJSON *child2 = cJSON_GetObjectItem(root, "token");
    if(NULL == child2)
    {
        LOG(Produce_LOG_MODULE, Produce_LOG_PROC, "cJSON_GetObjectItem err\n");
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

//获取生产记录信息数目
void get_Produce_count(int ret){
	char sql_cmd[SQL_MAX_LEN] = {0};
	MYSQL *conn = NULL;
	long line = 0;
	
    conn = msql_conn(mysql_user, mysql_pwd, mysql_db);
    if (conn == NULL)
    {
        LOG(Produce_LOG_MODULE, Produce_LOG_PROC, "msql_conn err\n");
        goto END;
    }
	
    mysql_query(conn, "set names utf8");

    sprintf(sql_cmd, "select count(*) from ProduceRecords");
    char tmp[512] = {0};
    
    int ret2 = process_result_one(conn, sql_cmd, tmp); 
    if(ret2 != 0)
    {
        LOG(Produce_LOG_MODULE, Produce_LOG_PROC, "%s 操作失败\n", sql_cmd);
        goto END;
    }

    line = atol(tmp); 

END:
    if(conn != NULL)
    {
        mysql_close(conn);
    }

    LOG(Produce_LOG_MODULE, Produce_LOG_PROC, "line = %ld\n", line);

    return_login_status(line, ret);
}

//解析的json包
int get_Producelist_json_info(char *buf,char *user,char *token,int *p_start,int *p_count){
	int ret = 0;
	
	cJSON * root = cJSON_Parse(buf);
	if(NULL == root){
		LOG(Produce_LOG_MODULE,Produce_LOG_PROC,"cJSON_Parse err\n");
		ret =-1;
		goto END;
	}
	cJSON* child1 = cJSON_GetObjectItem(root,"user");
	if(NULL == child1){
		LOG(Produce_LOG_MODULE,Produce_LOG_PROC,"cJSON_GetObjectItem err\n");
		ret = -1;
		goto END;
	}
	strcpy(user, child1->valuestring);
	
	cJSON* child2 = cJSON_GetObjectItem(root,"token");
	if(NULL == child2){
		LOG(Produce_LOG_MODULE,Produce_LOG_PROC,"cJSON_GetObjectItem err\n");
		ret = -1;
		goto END;
	}
	strcpy(token, child2->valuestring);
	
	cJSON* child3 = cJSON_GetObjectItem(root,"start");
	if(NULL == child3){
		LOG(Produce_LOG_MODULE,Produce_LOG_PROC,"cJSON_GetObjectItem err\n");
		ret = -1;
		goto END;
	}
	*p_start = child3->valueint; 

	cJSON* child4 = cJSON_GetObjectItem(root,"count");
	if(NULL == child4){
		LOG(Produce_LOG_MODULE,Produce_LOG_PROC,"cJSON_GetObjectItem err\n");
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
		LOG(Produce_LOG_MODULE,Produce_LOG_PROC,"msql_conn err\n");
		goto END;
	}
	
    mysql_query(conn, "set names utf8");
	
	if(is_number(keyword)){
		sprintf(sql_cmd,"select count(*) from ProduceRecords where produce_id = '%s'",keyword);
	}else{
		sprintf(sql_cmd,"select count(*) from ProduceRecords where product_name = '%s'",keyword);
	}
	
	char tmp[512] = {0};
	
	int ret2 = process_result_one(conn,sql_cmd,tmp);	
	if(ret2 != 0){
		LOG(Produce_LOG_MODULE,Produce_LOG_PROC,"%s 操作失败\n",sql_cmd);
		goto END;
	}

	line = atol(tmp); 
	
END:
	if(conn != NULL){
		mysql_close(conn);
	}
	
	LOG(Produce_LOG_MODULE,Produce_LOG_PROC,"line = %ld\n",line);
	
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
		LOG(Produce_LOG_MODULE,Produce_LOG_PROC,"msql_conn err\n");
		ret = -1;
		goto END;
	}
	
    mysql_query(conn, "set names utf8");
	
	if(is_number(keyword)){
		sprintf(sql_cmd,"select * from ProduceRecords where produce_id = '%s'",keyword);
	}else{
		sprintf(sql_cmd,"select * from ProduceRecords where product_name = '%s'",keyword);
	}
	
	LOG(Produce_LOG_MODULE,Produce_LOG_PROC,"%s 在操作\n",sql_cmd);
	
	if(mysql_query(conn,sql_cmd) != 0){
		LOG(Produce_LOG_MODULE,Produce_LOG_PROC,"%s 操作失败:%s \n",sql_cmd,mysql_error(conn));
		ret = -1;
		goto END;
	}
	
	res_set = mysql_store_result(conn);
	
	if (res_set == NULL)
    {
        LOG(Produce_LOG_MODULE, Produce_LOG_PROC, "smysql_store_result error: %s!\n", mysql_error(conn));
        ret = -1;
        goto END;
    }

    ulong line = 0;
    line = mysql_num_rows(res_set);
    if (line == 0)
    {
        LOG(Produce_LOG_MODULE, Produce_LOG_PROC, "mysql_num_rows(res_set) failed：%s\n", mysql_error(conn));
        ret = -1;
        goto END;
    }
	
	MYSQL_ROW row;
	
	root = cJSON_CreateObject();
	array = cJSON_CreateArray();
	while((row = mysql_fetch_row(res_set)) != NULL){
		cJSON *item = cJSON_CreateObject();
		if(row[0] != NULL){
			cJSON_AddNumberToObject(item,"produce_id",atoi(row[0]));
		}
		if(row[1] != NULL){
			cJSON_AddStringToObject(item,"product_name",row[1]);
		}
		if(row[2] != NULL){
			cJSON_AddNumberToObject(item,"product_quantity",atoi(row[2]));
		}
		if(row[3] != NULL){
			cJSON_AddStringToObject(item,"product_time",row[3]);
		}
		if(row[4] != NULL){
			cJSON* rawMaterial = cJSON_Parse(row[4]);
			if (rawMaterial != NULL) {
				cJSON_AddItemToObject(item, "RawMaterial", rawMaterial);
			}
		}
		
		cJSON_AddItemToArray(array,item);
	}
	
	cJSON_AddItemToObject(root,"Produce",array);
	
	out = cJSON_Print(root);
	
	LOG(Produce_LOG_MODULE,Produce_LOG_PROC,"%s\n",out);
	
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

int get_Produce_info(char* buf, int* produce_id, char* product_name, int* product_quantity, char* product_time, char RawMaterial[][100], int* material_quantity, int max_materials) {
    cJSON* root = cJSON_Parse(buf);
    if (root == NULL) {
        LOG(Produce_LOG_MODULE,Produce_LOG_PROC,"Failed to parse JSON data.\n");
        return -1;
    }

    cJSON* dataObject = cJSON_GetObjectItem(root, "data");
    if (dataObject == NULL) {
        LOG(Produce_LOG_MODULE,Produce_LOG_PROC,"Failed to get 'data' object.\n");
        cJSON_Delete(root);
        return -1;
    }

    cJSON* produceIdObject = cJSON_GetObjectItem(dataObject, "produce_id");
    if (produceIdObject == NULL) {
        LOG(Produce_LOG_MODULE,Produce_LOG_PROC,"Failed to get 'produce_id' value.\n");
        cJSON_Delete(root);
        return -1;
    }
    *produce_id = produceIdObject->valueint;

    cJSON* productNameObject = cJSON_GetObjectItem(dataObject, "product_name");
    if (productNameObject == NULL) {
        LOG(Produce_LOG_MODULE,Produce_LOG_PROC,"Failed to get 'product_name' value.\n");
        cJSON_Delete(root);
        return -1;
    }
    strcpy(product_name, productNameObject->valuestring);

    cJSON* productQuantityObject = cJSON_GetObjectItem(dataObject, "product_quantity");
    if (productQuantityObject == NULL) {
        LOG(Produce_LOG_MODULE,Produce_LOG_PROC,"Failed to get 'product_quantity' value.\n");
        cJSON_Delete(root);
        return -1;
    }
    *product_quantity = productQuantityObject->valueint;

    cJSON* productTimeObject = cJSON_GetObjectItem(dataObject, "product_time");
    if (productTimeObject == NULL) {
        LOG(Produce_LOG_MODULE,Produce_LOG_PROC,"Failed to get 'product_time' value.\n");
        cJSON_Delete(root);
        return -1;
    }
    strcpy(product_time, productTimeObject->valuestring);

    cJSON* materialTableArray = cJSON_GetObjectItem(root, "material_table");
    if (materialTableArray == NULL) {
        LOG(Produce_LOG_MODULE,Produce_LOG_PROC,"Failed to get 'material_table' array.\n");
        cJSON_Delete(root);
        return -1;
    }

    int materialCount = cJSON_GetArraySize(materialTableArray);
    if (materialCount > max_materials) {
        LOG(Produce_LOG_MODULE,Produce_LOG_PROC,"Exceeded maximum number of materials.\n");
        cJSON_Delete(root);
        return -1;
    }

    for (int i = 0; i < materialCount; ++i) {
        cJSON* materialObject = cJSON_GetArrayItem(materialTableArray, i);
        if (materialObject == NULL) {
            LOG(Produce_LOG_MODULE,Produce_LOG_PROC,"Failed to get material object at index %d.\n", i);
            cJSON_Delete(root);
            return -1;
        }

        cJSON* materialNameObject = cJSON_GetObjectItem(materialObject, "material_name");
        if (materialNameObject == NULL) {
            LOG(Produce_LOG_MODULE,Produce_LOG_PROC,"Failed to get 'material_name' value at index %d.\n", i);
            cJSON_Delete(root);
            return -1;
        }
        strcpy(RawMaterial[i], materialNameObject->valuestring);

        cJSON* materialQuantityObject = cJSON_GetObjectItem(materialObject, "quantity");
        if (materialQuantityObject == NULL) {
            LOG(Produce_LOG_MODULE,Produce_LOG_PROC,"Failed to get 'quantity' value at index %d.\n", i);
            cJSON_Delete(root);
            return -1;
        }
        material_quantity[i] = materialQuantityObject->valueint;
    }

    cJSON_Delete(root);
    return 0;
}

//更新生产记录信息
int Produce_update(char* buf) {
    int ret = 0;
    MYSQL* conn = NULL;

    int produce_id;
    char product_name[128];
    int product_quantity;
    char product_time[128];
    char RawMaterial[10][128];  // 假设最多有 10 种原料
    int material_quantity[10];  // 对应原料的数量
    ret = get_Produce_info(buf, &produce_id, product_name, &product_quantity, product_time, RawMaterial, material_quantity, 10);
    if (ret != 0) {
        goto END;
    }
    LOG(Produce_LOG_MODULE, Produce_LOG_PROC, "produce_id = %d, product_name = %s, product_quantity = %d, product_time = %s, material_quantity = %d", produce_id, product_name, product_quantity, product_time, material_quantity);
    
	conn = msql_conn(mysql_user, mysql_pwd, mysql_db);
    if (conn == NULL) {
        LOG(Produce_LOG_MODULE, Produce_LOG_PROC, "msql_conn err\n");
        ret = -1;
        goto END;
    }
    mysql_query(conn, "set names utf8");

    char sql_cmd[SQL_MAX_LEN] = {0};
    sprintf(sql_cmd, "UPDATE ProduceRecords SET product_name = '%s', product_quantity = %d, product_time = '%s', RawMaterial = '%s' WHERE produce_id = %d", product_name, product_quantity, product_time, RawMaterial, produce_id);
    int affected_rows = 0;
    int ret2 = process_no_result(conn, sql_cmd, &affected_rows);
    if (ret2 != 0) {
        LOG(Produce_LOG_MODULE, Produce_LOG_PROC, "%s 更新失败\n", sql_cmd);
        ret = -1;
        goto END;
    } else {
        LOG(Produce_LOG_MODULE, Produce_LOG_PROC, "affected %d rows\n", affected_rows);
    }

END:
    if (conn != NULL) {
        mysql_close(conn);
    }

    return ret;
}

int Produce_delete(char *buf){
	int ret = 0;
	MYSQL *conn = NULL;

	int produce_id;
    char product_name[128];
    int product_quantity;
    char product_time[128];
    char RawMaterial[10][128];  // 假设最多有 10 种原料
    int material_quantity[10];  // 对应原料的数量
    ret = get_Produce_info(buf, &produce_id, product_name, &product_quantity, product_time, RawMaterial, material_quantity, 10);
    if (ret != 0) {
        goto END;
    }
    LOG(Produce_LOG_MODULE, Produce_LOG_PROC, "produce_id = %d, product_name = %s, product_quantity = %d, product_time = %s, material_quantity = %d", produce_id, product_name, product_quantity, product_time, material_quantity);
    
	conn = msql_conn(mysql_user,mysql_pwd,mysql_db);
	if(conn == NULL){
		LOG(Produce_LOG_MODULE,Produce_LOG_PROC,"msql_conn err\n");
		ret = -1;
		goto END;
	}
	mysql_query(conn,"set names utf8");
	
	char sql_cmd[SQL_MAX_LEN] = {0};
	sprintf(sql_cmd,"DELETE FROM ProduceRecords WHERE produce_id = '%d';",produce_id);
	int affected_rows = 0;
	int ret2 = process_no_result(conn,sql_cmd,&affected_rows);
	if(ret2 != 0){
        LOG(Produce_LOG_MODULE, Produce_LOG_PROC, "%s 删除失败\n", sql_cmd);
		ret = -1;
        goto END;
	}else{
		LOG(Produce_LOG_MODULE,Produce_LOG_PROC,"affected %d rows\n",affected_rows);
	}
	
END:
    if(conn != NULL)
    {
        mysql_close(conn);
    }

    return ret;
}

//添加原料信息
int Produce_add(char* buf) {
    int ret = 0;
    MYSQL* conn = NULL;

    int produce_id;
    char product_name[128];
    int product_quantity;
    char product_time[128];
    char RawMaterial[10][128];  // 假设最多有 10 种原料
    int material_quantity[10];  // 对应原料的数量
    ret = get_Produce_info(buf, &produce_id, product_name, &product_quantity, product_time, RawMaterial, material_quantity, 10);
    if (ret != 0) {
        goto END;
    }
    LOG(Produce_LOG_MODULE, Produce_LOG_PROC, "produce_id = %d, product_name = %s, product_quantity = %d, product_time = %s, material_quantity = %d", produce_id, product_name, product_quantity, product_time, material_quantity);

    conn = msql_conn(mysql_user, mysql_pwd, mysql_db);
    if (conn == NULL) {
        LOG(Produce_LOG_MODULE, Produce_LOG_PROC, "msql_conn err\n");
        ret = -1;
        goto END;
    }
    mysql_query(conn, "set names utf8");

    char sql_cmd[SQL_MAX_LEN] = {0};

    sprintf(sql_cmd, "INSERT INTO ProduceRecords (produce_id, product_name, product_quantity, product_time, RawMaterial) VALUES ('%d', '%s', '%d', '%s', '%s')", produce_id, product_name, product_quantity, product_time, RawMaterial);
    int affected_rows = 0;
    int ret2 = process_no_result(conn, sql_cmd, &affected_rows);
    if (ret2 != 0) {
        LOG(Produce_LOG_MODULE, Produce_LOG_PROC, "%s 插入失败\n", sql_cmd);
        ret = -1;
        goto END;
    } else {
        LOG(Produce_LOG_MODULE, Produce_LOG_PROC, "affected %d rows\n", affected_rows);
    }

END:
    if (conn != NULL) {
        mysql_close(conn);
    }

    return ret;
}

//获取生产记录列表
int get_Produce_list(char *cmd,char *user,int start,int count){
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
        LOG(Produce_LOG_MODULE, Produce_LOG_PROC, "msql_conn err\n");
        ret = -1;
        goto END;
    }

    mysql_query(conn, "set names utf8");
	
	if(strcmp(cmd,"ProduceNormal") == 0)
	{
		//sql语句
		sprintf(sql_cmd,"SELECT * FROM ProduceRecords limit %d,%d \n",start,count);
	}
	
	LOG(Produce_LOG_MODULE,Produce_LOG_PROC,"%s 在操作\n",sql_cmd);
	
	if(mysql_query(conn,sql_cmd) != 0){
		LOG(Produce_LOG_MODULE,Produce_LOG_PROC,"%s 操作失败:%s \n",sql_cmd,mysql_error(conn));
		ret = -1;
		goto END;
	}
	
	res_set = mysql_store_result(conn);
    if (res_set == NULL)
    {
        LOG(Produce_LOG_MODULE, Produce_LOG_PROC, "smysql_store_result error: %s!\n", mysql_error(conn));
        ret = -1;
        goto END;
    }

    ulong line = 0;
    line = mysql_num_rows(res_set);
    if (line == 0)
    {
        LOG(Produce_LOG_MODULE, Produce_LOG_PROC, "mysql_num_rows(res_set) failed：%s\n", mysql_error(conn));
        ret = -1;
        goto END;
    }
	
	MYSQL_ROW row;
	
	root = cJSON_CreateObject();
	array = cJSON_CreateArray();
	while((row = mysql_fetch_row(res_set)) != NULL){
		cJSON *item = cJSON_CreateObject();
		if(row[0] != NULL){
			cJSON_AddNumberToObject(item,"produce_id",atoi(row[0]));
		}
		if(row[1] != NULL){
			cJSON_AddStringToObject(item,"product_name",row[1]);
		}
		if(row[2] != NULL){
			cJSON_AddNumberToObject(item,"product_quantity",atoi(row[2]));
		}
		if(row[3] != NULL){
			cJSON_AddStringToObject(item,"product_time",row[3]);
		}
		if(row[4] != NULL){
			cJSON* rawMaterial = cJSON_Parse(row[4]);
			if (rawMaterial != NULL) {
				cJSON_AddItemToObject(item, "RawMaterial", rawMaterial);
			}
		}
		
		cJSON_AddItemToArray(array,item);
	}
	
	cJSON_AddItemToObject(root,"Produce",array);
	
	out = cJSON_Print(root);
	
	LOG(Produce_LOG_MODULE,Produce_LOG_PROC,"%s\n",out);
	
END:
	if(ret == 0){
		printf("%s",out); //给客户端返回信息
	}else{
		
        /*
			获取生产记录列表：
            成功：生产记录列表json
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
	LOG(Produce_LOG_MODULE, Produce_LOG_PROC, "确实是Produce \n");
    // 阻塞等待用户连接
    while (FCGI_Accept() >= 0)
    {
        LOG(Produce_LOG_MODULE, Produce_LOG_PROC, "确实是Produce确实确实\n");
        // 获取URL地址 "?" 后面的内容
        char *query = getenv("QUERY_STRING");

        // 解析命令
        query_parse_key_value(query, "cmd", cmd, NULL);
        LOG(Produce_LOG_MODULE, Produce_LOG_PROC, "cmd = %s\n", cmd);

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
            LOG(Produce_LOG_MODULE, Produce_LOG_PROC, "len = 0, No data from standard input\n");
        }
        else
        {
            char buf[4*1024] = {0};
            int ret = 0;
            ret = fread(buf, 1, len, stdin); // 从标准输入(web服务器)读取内容
            if(ret == 0)
            {
                LOG(Produce_LOG_MODULE, Produce_LOG_PROC, "fread(buf, 1, len, stdin) err\n");
                continue;
            }

            LOG(Produce_LOG_MODULE, Produce_LOG_PROC, "buf = %s\n", buf);

            LOG(Produce_LOG_MODULE, Produce_LOG_PROC, "Cmd is :%s \n", cmd);
            if (strcmp(cmd, "ProduceCount") == 0) //count 获取生产记录信息数目
            {
                LOG(Produce_LOG_MODULE, Produce_LOG_PROC, "Join In ProduceCount cmd is : %s\n", cmd);
                get_count_json_info(buf, user, token); // 通过json包获取用户名, token

                // 验证登陆token，成功返回0，失败-1
				ret = verify_token(user, token); // util_cgi.h

                get_Produce_count(ret); // 获取生产记录信息个数

            }else if(strcmp(cmd,"ProduceNormal") == 0){	//初始化时获取生产记录列表的
				int start;// 文件起点
				int count;// 文件个数
				get_Producelist_json_info(buf,user,token,&start,&count); // 通过json包获取信息
				LOG(Produce_LOG_MODULE,Produce_LOG_PROC, "user = %s, token = %s, start = %d, count = %d\n", user, token, start, count);
			
				// 验证登录token，成功返回0，失败-1
				ret = verify_token(user,token);
				if(ret == 0){
					get_Produce_list(cmd,user,start,count);	//获取生产记录列表
				}
				else{
					char *out = return_status("111"); 	// token 验证失败错误码
					if(out != NULL){
						printf(out); 	//给前端反馈错误码
						free(out);
					}
				}
			}else if(strcmp(cmd,"ProduceResult") == 0){	//搜索时获取生产记录列表的
				int start; //文件起点
				int count; // 文件个数
				get_Producelist_json_info(buf,user,token,&start,&count); // 通过json包获取信息
				LOG(Produce_LOG_MODULE,Produce_LOG_PROC,"user = %s, token = %s, start = %d, count = %d\n", user, token, start, count);
				
				// 验证登录token，成功返回0，失败-1
				ret = verify_token(user,token);
				if(ret == 0){
					get_search_list(cmd,user,start,count,keyword);	//获取生产记录列表
				}
				else{
					char *out = return_status("111"); 	// token 验证失败错误码
					if(out != NULL){
						printf(out); 	//给前端反馈错误码
						free(out);
					}
				}
			}else if(strcmp(cmd,"ProduceDelete") == 0){
				ret = Produce_delete(buf);
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
			}else if(strcmp(cmd,"ProduceUpdate") == 0){
				ret = Produce_update(buf);
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
			}else if(strcmp(cmd,"ProduceAdd") == 0){
				ret = Produce_add(buf);
				if (ret == 0) //上传成功
				{
					//返回前端添加情况， 020代表成功
					char *out = return_status("020");
					if(out != NULL){
						printf(out); 	//给前端反馈错误码
						free(out);
					}
				}
				else if(ret == -1)
				{
					//返回前端添加情况， 021代表失败
					char *out = return_status("021");
					if(out != NULL){
						printf(out); 	//给前端反馈错误码
						free(out);
					}
				}
			}else{
				query_parse_key_value(cmd,"ProduceSearch",keywordori,NULL);
				//将base64转码的关键字转为原来的文字
				base64_decode(keywordori,keyword);
				LOG(Produce_LOG_MODULE,Produce_LOG_PROC,"keywordori = %s\n",keywordori);
				LOG(Produce_LOG_MODULE,Produce_LOG_PROC,"keyword = %s\n",keyword);
				
				get_count_json_info(buf, user, token); // 通过json包获取用户名, token
				
				// 验证登陆token，成功返回0，失败-1
				ret = verify_token(user, token); // util_cgi.h
				
				get_search_count(ret,keyword);
			}
        }
    }
    return 0;
}
