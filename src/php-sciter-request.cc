//
// Created by root on 20-3-15.
//
#include "php_sciter.h"
/**
 * 初始化_GET _POST _REQUEST的运行时变量
 */
BOOL ZendSciterRequest::initRequest(const std::string& request_uri) {
    //initialize $_GET $_POST  $_REQUEST $_SERVER
    if (PG(auto_globals_jit)) {
#if PHP_VERSION_ID >= 70000
        zend_string *request_str = zend_string_init(ZEND_STRL("_REQUEST"), 0);
        zend_is_auto_global(request_str);
        zend_string_release(request_str);
#else
        zend_is_auto_global(ZEND_REQUEST, ZEND_REQUEST_LEN);
#endif
    }

    if (PG(auto_globals_jit)) {
#if PHP_VERSION_ID >= 70000
        zend_string *request_str = zend_string_init(ZEND_STRL("_SERVER"), 0);
        zend_is_auto_global(request_str);
        zend_string_release(request_str);
#else
        zend_is_auto_global(ZEND_SERVER, ZEND_SERVER_LEN);
#endif
    }

    //clear and init global var
    zval* array;
    MAKE_STD_ZVAL(array);
#if PHP_VERSION_ID >= 70000
    zval* post_data = zend_hash_str_find(&EG(symbol_table),ZEND_POST,ZEND_POST_LEN);
    zval* get_data = zend_hash_str_find(&EG(symbol_table),ZEND_GET,ZEND_GET_LEN);
    zval* request_data = zend_hash_str_find(&EG(symbol_table),ZEND_REQUEST,ZEND_REQUEST_LEN);
    zval* server_data = zend_hash_str_find(&EG(symbol_table),ZEND_SERVER,ZEND_SERVER_LEN);
#else
    int res;
    zval** post_data = nullptr;
    zval** get_data = nullptr;
    zval** request_data = nullptr;
    zval** server_data = nullptr;

    res = zend_hash_find(&EG(symbol_table), ZEND_POST, ZEND_POST_LEN, (void**)&post_data);
    if(res == FAILURE)
    {
        post_data = nullptr;
    }
    res = zend_hash_find(&EG(symbol_table), ZEND_GET, ZEND_GET_LEN, (void**)&get_data);
    if(res == FAILURE)
    {
        get_data = nullptr;
    }
    res =zend_hash_find(&EG(symbol_table), ZEND_REQUEST, ZEND_REQUEST_LEN, (void**)&request_data);
    if(res == FAILURE)
    {
        request_data = nullptr;
    }
    res = zend_hash_find(&EG(symbol_table), ZEND_SERVER, ZEND_SERVER_LEN, (void**)&server_data);
    if(res == FAILURE)
    {
        server_data = nullptr;
    }
#endif

    if(get_data)
    {
        PHPSCITER_ZVAL_DTOR(*get_data);
        array_init(*get_data);

    }else{
#if PHP_VERSION_ID >= 70000
        get_data = zend_hash_str_add(&EG(symbol_table),ZEND_GET,ZEND_GET_LEN,&array);
#else
        zend_hash_add(&EG(symbol_table), ZEND_GET,ZEND_GET_LEN, (void *)&array, sizeof(zval*), (void**)&get_data);
#endif
    }
    if(post_data)
    {
        PHPSCITER_ZVAL_DTOR(*post_data);
        array_init(*post_data);
    }else{
#if PHP_VERSION_ID >= 70000
        post_data = zend_hash_str_add(&EG(symbol_table),ZEND_POST,ZEND_POST_LEN,&array);
#else
        zend_hash_add(&EG(symbol_table), ZEND_POST, ZEND_POST_LEN, (void *)&array, sizeof(zval*), (void**)&post_data);
#endif
    }
    if(request_data)
    {
        PHPSCITER_ZVAL_DTOR(*request_data);
        array_init(*request_data);
    }else{
#if PHP_VERSION_ID >= 70000
        request_data = zend_hash_str_add(&EG(symbol_table),ZEND_REQUEST,ZEND_REQUEST_LEN,&array);
#else
        array_init(array);
        zend_hash_add(&EG(symbol_table), ZEND_REQUEST, ZEND_REQUEST_LEN, (void *)&array, sizeof(zval*), (void**)&request_data);
#endif

    }
    if(!server_data)
    {
#if PHP_VERSION_ID >= 70000
        server_data = zend_hash_str_add(&EG(symbol_table),ZEND_SERVER,ZEND_SERVER_LEN,&array);;
#else
        zend_hash_add(&EG(symbol_table), ZEND_SERVER, ZEND_SERVER_LEN, (void *)array, sizeof(zval*), (void**)&server_data);
#endif
    }

    request_storage.request_uri = request_uri;
    request_storage.request_real_uri = request_uri;
    request_storage.request_data = *request_data;
    request_storage.get_data = *get_data;
    request_storage.post_data = *post_data;
    request_storage.server_data = *server_data;
}

BOOL ZendSciterRequest::onRequest(LPSCN_LOAD_DATA load_data) {
    HREQUEST request_id;
    UINT request_param_number;
    uint8_t res;
    UINT request_param_count;
    REQUEST_RQ_TYPE rq_type;

    //parse url info
    std::vector<string> url_vector;
    std::string parse_url(request_storage.request_uri);
    PHPSCITER_G(tool)->splitString(request_storage.request_uri,url_vector,"?");
    if(!url_vector.empty())
    {
        request_storage.request_real_uri = url_vector[0];
        if(url_vector.size() > 1)
        {
            std::vector<string> url_param_list;
            PHPSCITER_G(tool)->splitString(url_vector[1],url_param_list,"&");
            if(!url_param_list.empty())
            {
                std::vector<string> url_param_unit;
                size_t len = url_param_list.size();
                for(int i=0;i<len;i++)
                {
                    PHPSCITER_G(tool)->splitString(url_param_list[i],url_param_unit,"=");
                    if(!url_param_unit.empty())
                    {
                        size_t param_len = url_param_unit.size();
                        switch (param_len)
                        {
                            case 1:
                                request_storage.url_param[url_param_unit[0]] = "";
                                break;
                            case 2:
                                request_storage.url_param[url_param_unit[0]] = url_param_unit[1];
                                break;
                        }
                    }
                }
            }
        }
    }

    //parse request data
    request_id = load_data->requestId;
    res = rapi()->RequestGetRequestType(request_id, &rq_type);
    if (res != REQUEST_OK) {
        return FALSE;
    }
    request_storage.request_type = rq_type;
    res = rapi()->RequestGetNumberOfParameters(request_id, &request_param_number);
    if (res == REQUEST_OK)
    {
        if(request_param_count > 0) {
            LPVOID
            request_ptr = PHPSCITER_G(request).get();
            for (request_param_count = 0; request_param_count < request_param_number; request_param_count++) {
                res = rapi()->RequestGetNthParameterName(request_id, request_param_count, callSciterRequestName,
                                                         request_ptr);
                res = rapi()->RequestGetNthParameterValue(request_id, request_param_count, callSciterRequestValue,
                                                          request_ptr);
                request_storage.request_table[current_request_key] = current_request_value;
            }
        }
    }
}

const std::string& ZendSciterRequest::onComplete() {
    zval* storage_data;
    zval* request_data;
    map<std::string,std::string> request_info;
    map<std::string,std::string>::iterator request_map_iter;
    zval zend_data;
    int res;

    switch(request_storage.request_type)
    {
        case RRT_GET:
            storage_data = request_storage.get_data;
            break;
        case RRT_POST:
            storage_data = request_storage.post_data;
            break;
        default:
            return FALSE;
            break;
    }
    request_data = request_storage.request_data;
    request_info = request_storage.request_table;

    if(!storage_data)
        return FALSE;
    if(!request_data)
        return FALSE;

    //parse url
    zval zend_request_uri;
    PHPSCITER_ZVAL_STRING(&zend_request_uri,request_storage.request_uri.c_str());
#if PHP_VERSION_ID >= 70000
    zend_hash_str_add(HASH_OF(request_storage.server_data),ZEND_SERVER_REQUEST_URI
            ,ZEND_SERVER_REQUEST_URI_LEN,&zend_request_uri);
#else
    res = zend_hash_add(HASH_OF(request_storage.server_data), ZEND_SERVER_REQUEST_URI, ZEND_SERVER_REQUEST_URI_LEN, (void *)&zend_request_uri,
            sizeof(zval*), nullptr);
#endif


    for(request_map_iter = request_storage.url_param.begin();
        request_map_iter!=request_storage.url_param.end();request_map_iter++) {
        std::string key = request_map_iter->first;
        std::string value  = request_map_iter->second;
        PHPSCITER_ZVAL_STRING(&zend_data,key.c_str());
#if PHP_VERSION_ID >= 70000
        zend_hash_str_add(HASH_OF(request_storage.get_data),key.c_str(),key.length(),&zend_data);
#else
        res = zend_hash_add(HASH_OF(request_storage.get_data), key.c_str(), key.length(), (void *)&zend_data,
                      sizeof(zval*), nullptr);
#endif
        PHPSCITER_ZVAL_STRING(&zend_data,value.c_str());
#if PHP_VERSION_ID >= 70000
        zend_hash_str_add(HASH_OF(request_data),key.c_str(),key.length(),&zend_data);
#else
        res = zend_hash_add(HASH_OF(request_data), key.c_str(),key.length(), (void *)&zend_data,
                      sizeof(zval), nullptr);
#endif
    }

    for(request_map_iter = request_info.begin();
    request_map_iter!=request_info.end();request_map_iter++) {
        std::string key = request_map_iter->first;
        std::string value  = request_map_iter->second;
        PHPSCITER_ZVAL_STRING(&zend_data,key.c_str());
#if PHP_VERSION_ID >= 70000
        zend_hash_str_add(HASH_OF(storage_data), key.c_str(),key.length(), &zend_data);
#else
        res = zend_hash_add(HASH_OF(storage_data), key.c_str(),key.length(), (void *)&zend_data,
                      sizeof(zval*), nullptr);
#endif
        PHPSCITER_ZVAL_STRING(&zend_data,value.c_str());
#if PHP_VERSION_ID >= 70000
        zend_hash_str_add(HASH_OF(request_data), value.c_str(),value.length(), &zend_data);
#else
        res = zend_hash_add(HASH_OF(request_data), value.c_str(),value.length(), (void *)&zend_data,
                      sizeof(zval), nullptr);
#endif
    }

    return request_storage.request_real_uri;
}

BOOL ZendSciterRequest::onClose()
{
    current_request_key.clear();
    current_request_value.clear();
    request_storage.request_real_uri.clear();
    request_storage.request_uri.clear();
    request_storage.url_param.clear();
    request_storage.server_data = nullptr;
    request_storage.post_data = nullptr;
    request_storage.get_data = nullptr;
    request_storage.request_data = nullptr;
    request_storage.request_type = RRT_GET;
    request_storage.request_table.clear();
}
