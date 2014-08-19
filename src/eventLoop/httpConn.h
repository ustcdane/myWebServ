// Copyright (c) 2014 Daniel Wang. All rights reserved.
// https://github.com/ustcdane/myWebServ
// Use of this source code is governed by a BSD-style license
// that can be found in the License file
// Author: Daniel Wang(daneustc at gmail dot com)

#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include "epoller.h"
#include "eventLoop.h"

namespace myWebServ
{

class eventLoop;

class httpConn
{
public:
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;
    // HTTP请求方法
    enum METHOD { GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT, PATCH };
    // 解析HTTP请求时,主状态机所处状态
    enum CHECK_STATE { CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER, CHECK_STATE_CONTENT };
    // 服务器处理HTTP请求的可能状态
    enum HTTP_CODE { NO_REQUEST, GET_REQUEST, BAD_REQUEST, NO_RESOURCE, FORBIDDEN_REQUEST, FILE_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION };
    // 行的读取状态
    enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN };

public:
    httpConn(int sockfd, epoller *pep, eventLoop *lp):m_sockfd(sockfd),pEpoll(pep), pLoop(lp)
    {
    	int error = 0;
    	socklen_t len = sizeof( error );
    	getsockopt( m_sockfd, SOL_SOCKET, SO_ERROR, &error, &len );
    	int reuse = 1;
    	// 关闭TIME_WAIT状态
    	setsockopt( m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof( reuse ) );
	init();
    	pEpoll->add(sockfd);
    }

    ~httpConn()
    {
        close(m_sockfd);
    }

public:
    void process();
    bool read();
    bool write();

private:
    void init();// 初始化连接
    HTTP_CODE process_read();// 解析HTTP请求
    bool process_write( HTTP_CODE ret );// 填充HTTP应答

    // process_read()调用下面函数进行HTTP请求解析
    HTTP_CODE parse_request_line( char* text );
    HTTP_CODE parse_headers( char* text );
    HTTP_CODE parse_content( char* text );
    HTTP_CODE do_request();
    char* get_line() { return m_read_buf + m_start_line; }
    LINE_STATUS parse_line();

    // process_write() 调用下面的函数填充 HTTP应答
    void unmap();
    bool add_response( const char* format, ... );
    bool add_content( const char* content );
    bool add_status_line( int status, const char* title );
    bool add_headers( int content_length );
    bool add_content_length( int content_length );
    bool add_linger();
    bool add_blank_line();

private:
    epoller *pEpoll;// 所有socket事件注册到这个epoll内核事件中
    eventLoop *pLoop;

    int m_sockfd;// 该HTTP连接的socket
    char m_read_buf[ READ_BUFFER_SIZE ];// 度缓冲区
    int m_read_idx;// 标识读缓冲区待读字节的位置
    int m_checked_idx;// 当前正在分析的字符在缓冲区的位置
    int m_start_line;// 当前正在解析的行的起始位置
    char m_write_buf[ WRITE_BUFFER_SIZE ];// 写缓冲区
    int m_write_idx;// 写缓冲区待发送字节数
    CHECK_STATE m_check_state; // 主状态机所处状态
    METHOD m_method;// 请求方法
    char m_real_file[ FILENAME_LEN ];// 客户请求的目标文件的完整路径
    char* m_url;// 客户请求的目标文件的文件名
    char* m_version;// HTTP协议版本
    char* m_host;// 主机名
    int m_content_length;// HTTP请求的消息体的长度
    bool m_linger;// HTTP请求是否保持连接 keeplive
    char* m_file_address;// 客户请求mmap到内存中的起始位置
    struct stat m_file_stat;// 目标文件的状态
    struct iovec m_iv[2];// 采用 writev来执行写操作，所需的变量
    int m_iv_count;
};

}
#endif
